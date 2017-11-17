/*
 * dxpb-grapher.c
 *
 * Heart of dxpb
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include "dxpb.h"
#include "pkgimport_msg.h"
#include "pkggraph_msg.h"
#include "pkgfiles_msg.h"
#include "pkgimport_grapher.h"
#include "pkggraph_grapher.h"
#include "pkgfiler_grapher.h"
#include "bworker_end_status.h"
#include "btranslate.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-grapher-help.inc"
	printf("%.*s\n", ___doc_dxpb_grapher_help_txt_len, ___doc_dxpb_grapher_help_txt);
}

#define DXPB_HANDLE_SOCK(type, sock, msg, importer, retVal) { \
		pkg##type##_msg_recv(msg, sock); \
		retVal = handle_##type##_msg(msg, importer); \
	}

enum ret_codes
handle_graph_msg(pkggraph_msg_t *msg, pkgimport_grapher_t *importer)
{
	switch(pkggraph_msg_id(msg)) {
	case PKGGRAPH_MSG_ICANHELP:
		pkgimport_grapher_worker_available(importer,
				pkggraph_msg_addr(msg),
				pkggraph_msg_check(msg),
				pkggraph_msg_hostarch(msg),
				pkggraph_msg_targetarch(msg),
				pkggraph_msg_iscross(msg),
				pkggraph_msg_cost(msg));
		break;
	case PKGGRAPH_MSG_FORGET_ABOUT_ME:
		pkgimport_grapher_worker_gone(importer,
				pkggraph_msg_addr(msg),
				pkggraph_msg_check(msg)
				);
		break;
	case PKGGRAPH_MSG_JOB_ENDED:
		switch(pkggraph_msg_cause(msg)) {
		case END_STATUS_OK:
			pkgimport_grapher_pkg_now_completed(importer,
					pkggraph_msg_pkgname(msg),
					pkggraph_msg_version(msg),
					pkggraph_msg_arch(msg));
			break;
		case END_STATUS_OBSOLETE: // Self-correcting by other means
			break;
		case END_STATUS_PKGFAIL:
			pkgimport_grapher_pkg_failed_to_build(importer,
					pkggraph_msg_addr(msg),
					pkggraph_msg_check(msg),
					pkggraph_msg_pkgname(msg),
					pkggraph_msg_version(msg),
					pkggraph_msg_arch(msg));
			break;
		case END_STATUS_NODISK:
		case END_STATUS_WRONG_ASSIGNMENT:
		case END_STATUS_NOMEM:
			pkgimport_grapher_worker_not_appropiate(importer,
					pkggraph_msg_addr(msg),
					pkggraph_msg_check(msg)
					);
			break;
		default:
			return ERR_CODE_BAD;
		}
		break;
	default:
		return ERR_CODE_BAD;
	}
	return ERR_CODE_OK;
}

int
handle_files_msg(pkgfiles_msg_t *msg, pkgimport_grapher_t *importer)
{
	switch(pkgfiles_msg_id(msg)) {
	case PKGFILES_MSG_PKGHERE:
		pkgimport_grapher_pkg_here(importer,
				pkgfiles_msg_pkgname(msg),
				pkgfiles_msg_version(msg),
				pkgfiles_msg_arch(msg));
		break;
	case PKGFILES_MSG_PKGNOTHERE:
		pkgimport_grapher_pkg_not_here(importer,
				pkgfiles_msg_pkgname(msg),
				pkgfiles_msg_version(msg),
				pkgfiles_msg_arch(msg));
		break;
	default:
		return ERR_CODE_BAD;
	}
	return ERR_CODE_OK;
}

int
main_loop(pkgimport_grapher_t *importer, pkggraph_grapher_t *grapher,
						pkgfiler_grapher_t *filer)
{
	assert(importer);
	assert(grapher);
	assert(filer);
	enum ret_codes retVal = ERR_CODE_OK;
	zsock_t *in_sock;
	zpoller_t *polling;
	zsock_t *import_sock, *graph_sock, *file_sock;
	int rc;

	pkgimport_msg_t *import_msg;
	pkggraph_msg_t *graph_msg;
	pkgfiles_msg_t *file_msg;

	import_sock = pkgimport_grapher_msgpipe(importer);
	assert(import_sock);
	graph_sock = pkggraph_grapher_msgpipe(grapher);
	assert(graph_sock);
	file_sock = pkgfiler_grapher_msgpipe(filer);
	assert(file_sock);

	import_msg = pkgimport_msg_new();
	graph_msg = pkggraph_msg_new();
	file_msg = pkgfiles_msg_new();

	/* We allow the following flows.
	 * import <-> file_actor
	 * import <-> graph
	 * but we just call the appropiate methods instead of passing messages
	 * in, and we know what to pass because of messages on the msgpipe.
	 */
	polling = zpoller_new(import_sock, graph_sock, file_sock, NULL);
	assert(polling);
	fprintf(stderr, "Now entering main loop\n");
	while (retVal == ERR_CODE_OK &&
				(in_sock = zpoller_wait(polling, -1)) != NULL) {
		if (in_sock == import_sock) {
			zframe_t *frame = zframe_recv(import_sock);
			switch (btranslate_type_of_msg(frame)) {
			case TRANSLATE_FILES:
				rc = pkgfiles_msg_recv(file_msg, import_sock);
				switch (rc) {
				case -2:
					exit(ERR_CODE_BADSOCK);
				case -1:
					exit(ERR_CODE_SADSOCK);
				}
				rc = pkgfiles_msg_send(file_msg, file_sock);
				assert(rc == 0);
				break;
			case TRANSLATE_GRAPH:
				rc = pkggraph_msg_recv(graph_msg, import_sock);
				switch (rc) {
				case -2:
					exit(ERR_CODE_BADSOCK);
				case -1:
					exit(ERR_CODE_SADSOCK);
				}
				rc = pkggraph_msg_send(graph_msg, graph_sock);
				assert(rc == 0);
				break;
			default:
				break;
			}
		} else if (in_sock == graph_sock) {
			DXPB_HANDLE_SOCK(graph, in_sock, graph_msg, importer,
					retVal);
		} else if (in_sock == file_sock) {
			DXPB_HANDLE_SOCK(files, in_sock, file_msg, importer,
					retVal);
		} else /* interrupted, bid everybody farewell and die */
			retVal = ERR_CODE_DONE;
	}
	zpoller_destroy(&polling);
	pkgimport_msg_destroy(&import_msg);
	pkggraph_msg_destroy(&graph_msg);
	pkgfiles_msg_destroy(&file_msg);
	return retVal;
}

int
run(int flags, const char *dbpath, const char *import_endpoint,
		const char *graph_endpoint, const char *file_endpoint,
		const char *pubpoint, const char *ssldir)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	assert(dbpath);
	assert(import_endpoint);
	assert(file_endpoint);
	assert(graph_endpoint);
	assert(ssldir);
	enum ret_codes retVal = ERR_CODE_BAD;
	char *tmp = NULL;
	pkgimport_grapher_t *importer;
	pkggraph_grapher_t *grapher;
	pkgfiler_grapher_t *filer;
	zactor_t *import_actor, *graph_actor, *file_actor;

	/* Init Import */
	importer = pkgimport_grapher_new();
	assert(importer);
	import_actor = pkgimport_grapher_actor(importer);
	assert(import_actor);
	zstr_sendx(import_actor, "SET DB PATH", dbpath, NULL);
	zstr_sendx(import_actor, "SET PUBLISH ENDPOINT", pubpoint, NULL);
	zstr_sendx(import_actor, "CONSTRUCT", import_endpoint, NULL);

	/* Init Graph */
	grapher = pkggraph_grapher_new();
	assert(grapher);
	graph_actor = pkggraph_grapher_actor(grapher);
	assert(graph_actor);
	zstr_sendx(graph_actor, "CONSTRUCT", graph_endpoint, NULL);

	/* Init File */
	filer = pkgfiler_grapher_new();
	assert(filer);
	file_actor = pkgfiler_grapher_actor(filer);
	assert(file_actor);
	zstr_sendx(file_actor, "CONSTRUCT", file_endpoint, NULL);

	/* Finish construction */
	/* For each of them, wait once */
	/* TODO: Replace each of these three with a single #define */
	tmp = zstr_recv(import_actor);
	if (strcmp(tmp, "FAILURE") == 0)
		retVal = ERR_CODE_SADSOCK;
	else if (strcmp(tmp, "SUCCESS") == 0)
		retVal = ERR_CODE_OK;
	else
		return ERR_CODE_BAD;

	if (retVal == ERR_CODE_OK) {
		tmp = zstr_recv(graph_actor);
		if (strcmp(tmp, "FAILURE") == 0)
			retVal = ERR_CODE_SADSOCK;
		else if (strcmp(tmp, "SUCCESS") == 0)
			retVal = ERR_CODE_OK;
		else
			return ERR_CODE_BAD;
	}

	if (retVal == ERR_CODE_OK) {
		tmp = zstr_recv(file_actor);
		if (strcmp(tmp, "FAILURE") == 0)
			retVal = ERR_CODE_SADSOCK;
		else if (strcmp(tmp, "SUCCESS") == 0)
			retVal = ERR_CODE_OK;
		else
			return ERR_CODE_BAD;
	}

	/* And this is where all the real work is done */
	if (flags & VERBOSE_FLAG)
		fprintf(stderr, "Now considering entering main loop\n");
	if (retVal == ERR_CODE_OK)
		main_loop(importer, grapher, filer);

	if (flags & VERBOSE_FLAG)
		fprintf(stderr, "Program complete, destroying actors\n");

	zstr_sendx(import_actor, "$TERM", NULL);
	zstr_sendx(graph_actor, "$TERM", NULL);
	zstr_sendx(file_actor, "$TERM", NULL);
	pkgimport_grapher_destroy(&importer);
	pkggraph_grapher_destroy(&grapher);
	pkgfiler_grapher_destroy(&filer);
	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "vhLd:f:g:i:I:k:";
 	char *default_dbpath = DEFAULT_DBPATH;
 	char *default_import_endpoint = DEFAULT_IMPORT_ENDPOINT;
 	char *default_file_endpoint = DEFAULT_FILE_ENDPOINT;
 	char *default_graph_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_pubpoint = DEFAULT_DXPB_GRAPHER_PUBPOINT;
 	char *default_ssldir = DEFAULT_SSLDIR;
 	char *dbpath = NULL;
 	char *import_endpoint = NULL;
 	char *file_endpoint = NULL;
 	char *graph_endpoint = NULL;
	char *pubpoint = NULL;
 	char *ssldir = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'd':
			dbpath = optarg;
			break;
		case 'f':
			file_endpoint = optarg;
			break;
		case 'g':
			graph_endpoint = optarg;
			break;
		case 'i':
			import_endpoint = optarg;
			break;
		case 'I':
			pubpoint = optarg;
			break;
		case 'k':
			ssldir = optarg;
			break;
		case 'L':
			print_license();
			return 0;
		case 'h':
			help();
			return 0;
		case 'v':
			flags |= VERBOSE_FLAG;
			break;
		case '?':
			flags |= ERR_FLAG;
			break;
		case ':':
			flags |= ERR_FLAG;
			break;
		}
	}

	prologue(argv[0]);

	if (flags & ERR_FLAG) {
		fprintf(stderr, "Exiting due to errors.\n");
		exit(ERR_CODE_BADWORLD);
	}

 	if (!dbpath)
		dbpath = default_dbpath;
 	if (!import_endpoint)
		import_endpoint = default_import_endpoint;
 	if (!file_endpoint)
		file_endpoint = default_file_endpoint;
 	if (!graph_endpoint)
		graph_endpoint = default_graph_endpoint;
 	if (!ssldir)
		ssldir = default_ssldir;
	if (!pubpoint)
		pubpoint = default_pubpoint;

	if (ensure_sock(graph_endpoint) != ERR_CODE_OK ||
			ensure_sock(import_endpoint) != ERR_CODE_OK ||
			ensure_sock(file_endpoint) != ERR_CODE_OK ||
			ensure_sock(pubpoint) != ERR_CODE_OK) {
		fprintf(stderr, "Aborted due to not being able to ensure our endpoint is there");
		exit(ERR_CODE_BAD);
	}

	return run(flags, dbpath, import_endpoint, graph_endpoint,
			file_endpoint, pubpoint, ssldir);
}

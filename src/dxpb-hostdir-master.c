/*
 * dxpb-pkgimport-agent.c
 *
 * Manager for xbps-src readers
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <stdio.h>
#include <czmq.h>
#include "dxpb.h"
#include "pkgfiler.h"
#include "pkggraph_filer.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

#include "dxpb-common.h"

void
help(void)
{
#include "dxpb-hostdir-master-help.inc"
	printf("%.*s\n", ___doc_dxpb_hostdir_master_help_txt_len, ___doc_dxpb_hostdir_master_help_txt);
}

enum ret_codes
run(int flags, const char *ssldir, const char *sdir, const char *rdir,
		const char *ldir, const char *file_endpoint,
		const char *graph_endpoint, const char *file_pubpoint,
		const char *graph_pubpoint)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	zactor_t *file_actor, *log_actor;
	pkggraph_filer_t *log_client;
	enum ret_codes retVal = ERR_CODE_OK;

	file_actor = zactor_new(pkgfiler, "pkgfiler");
	if (flags & VERBOSE_FLAG)
		zstr_sendx(file_actor, "VERBOSE", NULL);

	log_client = pkggraph_filer_new();
	assert(log_client);
	log_actor = pkggraph_filer_actor(log_client);
	if (flags & VERBOSE_FLAG)
		zstr_sendx(log_actor, "VERBOSE", NULL);

	zstr_sendx(file_actor, "SET", "dxpb/stagingdir", sdir, NULL);
	zstr_sendx(file_actor, "SET", "dxpb/repodir", rdir, NULL);
	zstr_sendx(file_actor, "SET", "dxpb/pubpoint", file_pubpoint, NULL);
	zstr_sendx(file_actor, "BIND", file_endpoint, NULL);

	zstr_sendx(log_actor, "SET LOGDIR", ldir, NULL);
	zstr_sendx(log_actor, "SET PUB TO PROVIDED", graph_pubpoint, NULL);
	zstr_sendx(log_actor, "CONSTRUCT", graph_endpoint, NULL);

	zpoller_t *polling = zpoller_new(file_actor, log_actor, NULL);
	assert(polling);

	char *tmp = zstr_recv(log_actor);
	if (!strcmp(tmp, "FAILURE"))
		retVal = ERR_CODE_SADSOCK;
	else if (!strcmp(tmp, "SUCCESS"))
		retVal = ERR_CODE_OK;
	zsock_flush(log_actor);


	zactor_t *sock_to_clear;
	while (retVal == ERR_CODE_OK && (sock_to_clear = zpoller_wait(polling, -1)) != NULL) {
		// We don't have any communication specified.
		flushsock(sock_to_clear, "hostdir-master");
	}

	zstr_sendx(file_actor, "$TERM", NULL);
	zstr_sendx(log_actor, "$TERM", NULL);

	zpoller_destroy(&polling);
	zactor_destroy(&file_actor);
	pkggraph_filer_destroy(&log_client);
	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "Lhvk:s:r:l:f:F:g:G:";
	char *default_stagingdir = DEFAULT_STAGINGDIR;
	char *default_repodir = DEFAULT_REPODIR;
	char *default_logdir = DEFAULT_LOGDIR;
	char *default_file_endpoint = DEFAULT_FILE_ENDPOINT;
	char *default_graph_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *default_graph_pubpoint = DEFAULT_DXPB_HOSTDIR_MASTER_GRAPHER_PUBPOINT;
	char *default_file_pubpoint = DEFAULT_DXPB_HOSTDIR_MASTER_FILE_PUBPOINT;
	char *stagingdir, *repodir, *logdir, *file_endpoint, *graph_endpoint,
	     *ssldir, *graph_pubpoint, *file_pubpoint;
	stagingdir = repodir = logdir = file_pubpoint = graph_pubpoint = NULL;
	file_endpoint = graph_endpoint = ssldir = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'L':
			print_license();
			return 0;
		case 'h':
			help();
			return 0;
		case 'g':
			graph_endpoint = optarg;
			break;
		case 'f':
			file_endpoint = optarg;
			break;
		case 'G':
			graph_pubpoint = optarg;
			break;
		case 'F':
			file_pubpoint = optarg;
			break;
		case 'k':
			ssldir = optarg;
			break;
		case 's':
			stagingdir = optarg;
			break;
		case 'r':
			repodir = optarg;
			break;
		case 'l':
			logdir = optarg;
			break;
		case 'v':
			flags |= VERBOSE_FLAG;
			break;
		case '?':
			flags |= ERR_FLAG;
			break;
		}
	}

	prologue(argv[0]);

	if (flags & ERR_FLAG) {
		fprintf(stderr, "Exiting due to errors.\n");
		help();
		exit(ERR_CODE_BADWORLD);
	}


	if (!ssldir)
		ssldir = default_ssldir;
	if (!stagingdir)
		stagingdir = default_stagingdir;
	if (!repodir)
		repodir = default_repodir;
	if (!logdir)
		logdir = default_logdir;
	if (!file_endpoint)
		file_endpoint = default_file_endpoint;
	if (!graph_endpoint)
		graph_endpoint = default_graph_endpoint;
	if (!file_pubpoint)
		file_pubpoint = default_file_pubpoint;
	if (!graph_pubpoint)
		graph_pubpoint = default_graph_pubpoint;

	if (ensure_sock(graph_endpoint) != ERR_CODE_OK ||
			ensure_sock(graph_pubpoint) != ERR_CODE_OK ||
			ensure_sock(file_endpoint) != ERR_CODE_OK ||
			ensure_sock(file_pubpoint) != ERR_CODE_OK) {
		fprintf(stderr, "Aborted due to not being able to ensure our endpoint is there");
		exit(ERR_CODE_BAD);
	}

	return run(flags, ssldir, stagingdir, repodir, logdir, file_endpoint, graph_endpoint, file_pubpoint, graph_pubpoint);
}

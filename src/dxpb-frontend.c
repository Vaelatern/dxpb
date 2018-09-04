/*
 * dxpb-frontend.c
 *
 * Manager for remote builders, reporting to grapher
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include "dxpb.h"
#include "pkggraph_server.h"
#include "blog.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-frontend-help.inc"
	printf("%.*s\n", ___doc_dxpb_frontend_help_txt_len, ___doc_dxpb_frontend_help_txt);
}

void
run(int flags, const char *endpoint, const char *ssldir, const char *pubpoint)
{
	assert((flags & ERR_FLAG) == 0);
	zactor_t *actor;

	actor = zactor_new(pkggraph_server, "pkggraph server");
	if (flags & VERBOSE_FLAG)
		zstr_sendx(actor, "VERBOSE", NULL);

	do_server_ssl_if_possible(actor, ssldir, "dxpb-frontend");

	zstr_sendx(actor, "SET", "dxpb/ssldir", ssldir, NULL);
	zstr_sendx(actor, "SET", "dxpb/pubpoint", pubpoint, NULL);
	zstr_sendx(actor, "BIND", endpoint, NULL);

	zpoller_t *polling = zpoller_new(actor, NULL);
	assert(polling);
	zactor_t *rc;
	while ((rc = zpoller_wait(polling, -1)) != NULL) {
		// We don't have any communication specified.
		zsock_flush(rc);
	}

	zstr_sendx(actor, "$TERM", NULL);

	zpoller_destroy(&polling);
	zactor_destroy(&actor);
	return;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "vhLg:G:k:Yo:";
	char *default_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *default_pubpoint = DEFAULT_DXPB_FRONTEND_PUBPOINT;
	char *endpoint = NULL;
	char *pubpoint = NULL;
	char *ssldir = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'Y':
			blog_logging_on(1);
			break;
		case 'o':
			blog_logfile(optarg);
			break;
		case 'L':
			print_license();
			return 0;
		case 'h':
			help();
			return 0;
		case 'g':
			endpoint = optarg;
			break;
		case 'G':
			pubpoint = optarg;
			break;
		case 'k':
			ssldir = optarg;
			break;
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
		help();
		exit(ERR_CODE_BADWORLD);
	}

	if (!endpoint)
		endpoint = default_endpoint;
	if (!pubpoint)
		pubpoint = default_pubpoint;
	if (!ssldir)
		ssldir = default_ssldir;

	enum ret_codes rc = ensure_sock_if_ipc(endpoint);
	assert(rc == ERR_CODE_OK);
	rc = ensure_sock_if_ipc(pubpoint);
	assert(rc == ERR_CODE_OK);

	run(flags, endpoint, ssldir, pubpoint);
	return 0;
}

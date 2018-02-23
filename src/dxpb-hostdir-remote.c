/*
 * dxpb_pkgimport_agent.c
 *
 * Runtime for xbps-src reader.
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include "pkgfiler_remote.h"
#include "dxpb.h"
#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-hostdir-remote-help.inc"
	printf("%.*s\n", ___doc_dxpb_hostdir_remote_help_txt_len, ___doc_dxpb_hostdir_remote_help_txt);
}

int
run(int flags, const char *endpoint, const char *hostdir, const char *ssldir)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	enum ret_codes retVal = ERR_CODE_BAD;
	pkgfiler_remote_t *client;
	zactor_t *actor;

	client = pkgfiler_remote_new();
	assert(client);
	actor = pkgfiler_remote_actor(client);
	assert(actor);
	if (flags & VERBOSE_FLAG)
		zstr_sendx(actor, "SET VERBOSE", 1, NULL);
	zstr_sendx(actor, "SET HOSTDIR", hostdir, NULL);
	zstr_sendx(actor, "CONSTRUCT", endpoint, NULL);

	zpoller_t *polling = zpoller_new(actor, NULL);
	assert(polling);
	zactor_t *rc = zpoller_wait(polling, -1);
	char *tmp = zstr_recv(rc);
	if (!strcmp(tmp, "FAILURE"))
		retVal = ERR_CODE_SADSOCK;
	else if (!strcmp(tmp, "SUCCESS"))
		retVal = ERR_CODE_OK;
	zsock_flush(rc);

	while (retVal == ERR_CODE_OK && (rc = zpoller_wait(polling, -1)) != NULL) {
		fprintf(stderr, "pkgimport-agent: %s\n", zstr_recv(rc));
		// We don't have any communication specified.
		zsock_flush(rc);
	}

	zstr_sendx(actor, "$TERM", NULL);

	zpoller_destroy(&polling);
	pkgfiler_remote_destroy(&client);
	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "vhLr:k:f:";
	char *default_hostdir = DEFAULT_REPOPATH;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *default_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *hostdir = NULL;
	char *ssldir = NULL;
	char *endpoint = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'h':
			help();
			return 0;
		case 'L':
			print_license();
			return 0;
		case 'f':
			endpoint = optarg;
			break;
		case 'k':
			ssldir = optarg;
			break;
		case 'r':
			hostdir = optarg;
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
		exit(ERR_CODE_BADWORLD);
	}

	if (!ssldir)
		ssldir = default_ssldir;
	if (!hostdir)
		hostdir = default_hostdir;
	if (!endpoint)
		endpoint = default_endpoint;

	enum ret_codes rc = ensure_sock_if_ipc(endpoint);
	assert(rc == ERR_CODE_OK);

	return run(flags, endpoint, hostdir, ssldir);
}

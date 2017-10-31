/*
 * dxpb-pkgimport-agent.c
 *
 * Manager for xbps-src readers
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include "dxpb.h"
#include "pkgimport_server.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-pkgimport-master-help.inc"
	printf("%.*s\n", ___doc_dxpb_pkgimport_master_help_txt_len, ___doc_dxpb_pkgimport_master_help_txt);
}

void
run(int flags, const char *endpoint, const char *pubpoint,
		const char *repopath, const char *xbps_src)
{
	assert((flags & ERR_FLAG) == 0);
	zactor_t *actor;

	actor = zactor_new(pkgimport_server, "pkgimport master");
	if (flags & VERBOSE_FLAG)
		zstr_sendx(actor, "VERBOSE", NULL);

	zstr_sendx(actor, "SET", "dxpb/repopath", repopath, NULL);
	zstr_sendx(actor, "SET", "dxpb/xbps_src", xbps_src, NULL);
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
	const char *optstring = "vhLr:i:I:x:";
	char *default_endpoint = DEFAULT_IMPORT_ENDPOINT;
	char *default_pubpoint = DEFAULT_DXPB_PKGIMPORT_MASTER_PUBPOINT;
	char *default_repopath = DEFAULT_REPOPATH;
	char *default_xbps_src = DEFAULT_XBPS_SRC;
	char *pubpoint = NULL;
	char *endpoint = NULL;
	char *repopath = NULL;
	char *xbps_src = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'L':
			print_license();
			return 0;
		case 'h':
			help();
			return 0;
		case 'I':
			pubpoint = optarg;
			break;
		case 'i':
			endpoint = optarg;
			break;
		case 'r':
			repopath = optarg;
			break;
		case 'x':
			xbps_src = optarg;
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
	if (!repopath)
		repopath = default_repopath;
	if (!xbps_src)
		xbps_src = default_xbps_src;

	run(flags, endpoint, pubpoint, repopath, xbps_src);
	return 0;
}

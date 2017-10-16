/*
 * dxpb_builder.c
 *
 * Runtime for xbps-src reader.
 */

#include <getopt.h>
#include <czmq.h>
#include "dxpb.h"
#include "bfs.h"
#include "bbuilder.h"
#include "pkggraph_worker.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

#include "dxpb-common.h"

void
help(void)
{
#include "dxpb-builder-help.inc"
	printf("%.*s\n", ___doc_dxpb_builder_help_txt_len, ___doc_dxpb_builder_help_txt);
}

char *
gimme_unix_socket()
{
	char *retVal = bfs_new_tmpsock("/var/run/dxpb/dxpb-XXXXXX", "builder-1");
	if (!retVal)
		exit(ERR_CODE_BAD);
	return retVal;
}

zsock_t *
spawn_child(char *endpoint, char *masterdir, char *hostdir, char *xbps_src)
{
	pid_t pidfirst;
	zsock_t *mypipe;

	switch(pidfirst = fork()) {
	case -1:
		perror("Ran into issue in first fork");
		exit(ERR_CODE_NOFORK);
	case 0: /* I am a child */
		mypipe = zsock_new_pair(endpoint);
		assert(mypipe);
		exit(bbuilder_agent(mypipe, masterdir, hostdir, xbps_src));
	default:
		sched_yield(); /* Give the child space to mature */
		zsock_t *retVal = zsock_new_pair(endpoint);
		assert(retVal);
		return retVal;
	}
}

int
run(int flags, char *masterdir, char *hostdir,  char *ssldir, char *endpoint, char *xbps_src)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	enum ret_codes retVal = ERR_CODE_BAD;
	pkggraph_worker_t *client;
	zactor_t *actor;

	char *child_endpoint = gimme_unix_socket();
	zsock_t *child = spawn_child(child_endpoint, masterdir, hostdir, xbps_src);

	if (flags & VERBOSE_FLAG)
		printf("Spawned a child\n");

	client = pkggraph_worker_new(child);
	assert(client);
	actor = pkggraph_worker_actor(client);
	assert(actor);
	zstr_sendx(actor, "CONSTRUCT", endpoint, NULL);

	zpoller_t *polling = zpoller_new(actor);
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
	pkggraph_worker_destroy(&client);
	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	char *default_masterdir = DEFAULT_MASTERDIR;
	char *default_hostdir = DEFAULT_HOSTDIR;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *default_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_xbps_src = DEFAULT_XBPS_SRC;
	char *masterdir = NULL;
	char *hostdir = NULL;
	char *ssldir = NULL;
	char *endpoint = NULL;
	char *xbps_src = NULL;
	const char *optstring = "hvLg:k:H:m:";

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'h':
			help();
			return 0;
		case 'L':
			print_license();
			return 0;
		case 'g':
			endpoint = optarg;
			break;
		case 'x':
			xbps_src = optarg;
			break;
		case 'k':
			ssldir = optarg;
			break;
		case 'H':
			hostdir = optarg;
			break;
		case 'm':
			masterdir = optarg;
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

	if (!masterdir)
		masterdir = default_masterdir;
	if (!hostdir)
		hostdir = default_hostdir;
	if (!ssldir)
		ssldir = default_ssldir;
	if (!endpoint)
		endpoint = default_endpoint;
	if (!xbps_src)
		xbps_src = default_xbps_src;

	return run(flags, masterdir, hostdir, ssldir, endpoint, xbps_src);
}

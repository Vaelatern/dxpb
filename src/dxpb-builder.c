/*
 * dxpb_builder.c
 *
 * Runtime for xbps-src reader.
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <czmq.h>
#include "bstring.h"
#include "dxpb.h"
#include "bfs.h"
#include "bbuilder.h"
#include "pkggraph_worker.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

#include "dxpb-common.h"

struct workerspec {
	char *hostarch;
	char *targetarch;
	uint8_t iscross;
	uint16_t cost;
};

struct workerspec *
parse_workerspec(const char *argv)
{
	struct workerspec *retVal = malloc(sizeof(struct workerspec));
	char *newargv = strdup(argv);
	if (!retVal || !newargv) {
		perror("Couldn't allocate a struct, or perhaps clone a string");
		exit(ERR_CODE_NOMEM);
	}
	// TODO: Replace with a better config, like:
	// host x86_64 target x86_64 cost 100
	// host x86_64 target armv7hf cross cost 100
	// host x86_64 target armv7hf cost 100 cross
	// I'll need to learn a little yacc for this. That's ok.
	//
	// format: host:target:cost:iscross
	// x86_64:x86_64:100
	// x86_64:armv7hf-musl:100:yes
	char *strtok_val = NULL;
	char *atom = NULL;
	enum fsm_state read_state = FSM_STATE_A;
	atom = strtok_r(newargv, ":", &strtok_val);
	while (atom != NULL) {
		switch(read_state) {
		case FSM_STATE_A:
			retVal->hostarch = atom;
			read_state = FSM_STATE_B;
			break;
		case FSM_STATE_B:
			retVal->targetarch = atom;
			read_state = FSM_STATE_C;
			break;
		case FSM_STATE_C:
			errno = 0;
			retVal->cost = strtol(atom, NULL, 10);
			retVal->iscross = 0;
			if (errno != 0)
				read_state = FSM_STATE_G;
			else
				read_state = FSM_STATE_D;
			break;
		case FSM_STATE_D: // Exists?
			retVal->iscross = 1;
			read_state = FSM_STATE_E;
			break;
		default:
			read_state = FSM_STATE_F;
			break;
		}
		atom = strtok_r(NULL, ":", &strtok_val);
	}
	if (read_state != FSM_STATE_D && read_state != FSM_STATE_E) {
		free(retVal);
		free(newargv);
		newargv = NULL;
		retVal = NULL;
	}
	return retVal;
}

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
		mypipe = zsock_new(ZMQ_PAIR);
		assert(mypipe);
		// rc1 and rc2 else they are allocated on the stack at the same
		// time and they should be allowed to be different values.
		int rc1 = zsock_bind(mypipe, "ipc://%s", endpoint);
		assert(rc1 != -1);
		exit(bbuilder_agent(mypipe, masterdir, hostdir, xbps_src));
	default:
		sched_yield(); /* Give the child space to mature */
		zsock_t *retVal = zsock_new(ZMQ_PAIR);
		assert(retVal);
		int rc2 = zsock_connect(retVal, "ipc://%s", endpoint);
		assert(rc2 != -1);
		return retVal;
	}
}

int
run(int flags, char *masterdir, char *hostdir,  char *ssldir, char *endpoint,
		char *repopath, struct workerspec *wrkr)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	enum ret_codes retVal = ERR_CODE_BAD;
	pkggraph_worker_t *client;
	zactor_t *actor;

	char *xbps_src = bstring_add(bstring_add(NULL, repopath,
					NULL, NULL), "/xbps-src", NULL, NULL);

	char *child_endpoint = gimme_unix_socket();
	zsock_t *child = spawn_child(child_endpoint, masterdir, hostdir, xbps_src);

	if (flags & VERBOSE_FLAG)
		printf("Spawned a child\n");

	client = pkggraph_worker_new(child);
	assert(client);
	actor = pkggraph_worker_actor(client);
	assert(actor);
	if (!pkggraph_worker_set_build_params(client, wrkr->hostarch, wrkr->targetarch, wrkr->iscross, wrkr->cost)) {
		fprintf(stderr, "Invalid arguments for hostarch %s and/or targetarch %s\n", wrkr->hostarch, wrkr->targetarch);
		retVal = ERR_CODE_BAD;
		goto end;
	}
	zstr_sendx(actor, "SET REPOPATH", repopath, NULL);
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

end:
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
	struct workerspec *wrkr = NULL;
	char *default_masterdir = DEFAULT_MASTERDIR;
	char *default_hostdir = DEFAULT_HOSTDIR;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *default_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_repopath = DEFAULT_REPOPATH;
	char *masterdir = NULL;
	char *hostdir = NULL;
	char *ssldir = NULL;
	char *endpoint = NULL;
	char *repopath = NULL;
	const char *optstring = "hvLg:k:H:m:W:r:";

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
		case 'r':
			repopath = optarg;
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
		case 'W':
			wrkr = parse_workerspec(optarg);
			if (!wrkr) {
				fprintf(stderr, "Something wrong with spec\n");
				fprintf(stderr, "\tHostarch:Targetarch:Cost[:CrossBuild]\n");
				exit(ERR_CODE_BAD);
			}
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
	if (!repopath)
		repopath = default_repopath;
	if (!wrkr)
		fprintf(stderr, "Need a worker specification (-W)\n");
	assert(wrkr);

	return run(flags, masterdir, hostdir, ssldir, endpoint, repopath, wrkr);
}

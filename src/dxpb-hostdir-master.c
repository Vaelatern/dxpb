/*
 * dxpb-pkgimport-agent.c
 *
 * Manager for xbps-src readers
 */

#include <getopt.h>
#include <stdio.h>
#include <czmq.h>
#include "dxpb.h"
#include "pkgfiler.h"
#include "pkggraph_filer.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

#define DEFAULT_STAGINGDIR "/var/lib/dxpb/staging/"
#define DEFAULT_REPODIR "/var/lib/dxpb/pkgs/"
#define DEFAULT_LOGDIR "/var/lib/dxpb/logs/"
#define DEFAULT_FILE_ENDPOINT "ipc:///var/run/dxpb/pkgfiler.sock"
#define DEFAULT_GRAPH_ENDPOINT "tcp://127.0.0.1:5195"
#define DEFAULT_SSLDIR "/etc/dxpb/curve/"

#include "dxpb-common.h"

void
help(void)
{
#include "dxpb-hostdir-master-help.inc"
	printf("%.*s\n", ___doc_dxpb_hostdir_master_help_txt_len, ___doc_dxpb_hostdir_master_help_txt);
}

void
run(int flags, const char *ssldir, const char *sdir, const char *rdir,
		const char *ldir, const char *file_endpoint,
		const char *graph_endpoint)
{
	SSLDIR_UNUSED(ssldir);
	assert((flags & ERR_FLAG) == 0);
	zactor_t *file_actor;
	pkggraph_filer_t *log_client;

	file_actor = zactor_new(pkgfiler, "pkgfiler");
	if (flags & VERBOSE_FLAG)
		zstr_sendx(file_actor, "VERBOSE", NULL);

	log_client = pkggraph_filer_new();
	assert(log_client);
	if (flags & VERBOSE_FLAG)
		zstr_sendx(log_client, "VERBOSE", NULL);

	zstr_sendx(file_actor, "BIND", file_endpoint, NULL);
	zstr_sendx(file_actor, "CONSTRUCT", graph_endpoint, NULL);

	zpoller_t *polling = zpoller_new(file_actor, log_client);
	assert(polling);
	zactor_t *rc;
	while ((rc = zpoller_wait(polling, -1)) != NULL) {
		// We don't have any communication specified.
		zsock_flush(rc);
	}

	zstr_sendx(file_actor, "$TERM", NULL);
	zstr_sendx(log_client, "$TERM", NULL);

	zpoller_destroy(&polling);
	zactor_destroy(&file_actor);
	pkggraph_filer_destroy(&log_client);
	return;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "Lhvk:s:r:l:f:g:";
	char *default_stagingdir = DEFAULT_STAGINGDIR;
	char *default_repodir = DEFAULT_REPODIR;
	char *default_logdir = DEFAULT_LOGDIR;
	char *default_file_endpoint = DEFAULT_FILE_ENDPOINT;
	char *default_graph_endpoint = DEFAULT_GRAPH_ENDPOINT;
	char *default_ssldir = DEFAULT_SSLDIR;
	char *stagingdir, *repodir, *logdir, *file_endpoint, *graph_endpoint, *ssldir;
	stagingdir = repodir = logdir = file_endpoint = graph_endpoint = ssldir = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'L':
			print_license();
			return 0;
		case 'g':
			graph_endpoint = optarg;
			break;
		case 'f':
			file_endpoint = optarg;
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
		case 'h':
			help();
			return 0;
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

	run(flags, ssldir, stagingdir, repodir, logdir, file_endpoint, graph_endpoint);
	return 0;
}

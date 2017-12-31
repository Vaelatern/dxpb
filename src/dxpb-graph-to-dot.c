/*
 * dxpb-graph-to-dot.c
 *
 * One-shot to translate from a pkgdb to a .dot file
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include <graphviz/cgraph.h>
#include <sqlite3.h>
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bdb.h"
#include "bdot.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-graph-to-dot-help.inc"
	printf("%.*s\n", ___doc_dxpb_graph_to_dot_help_txt_len, ___doc_dxpb_graph_to_dot_help_txt);
}

int
run(int flags, const char *from, const char *to)
{
	assert(from);
	assert(to);
	int verbose = flags & VERBOSE_FLAG;

	if (verbose)
		printf("Reading from database %s\n", from);
	struct bdb_bound_params *params = bdb_params_init_ro(from);
	assert(!params->DO_NOT_USE_PARAMS);
	if (verbose)
		printf("\tCreating graph\n");
	bgraph grph = bgraph_new();
	if (verbose)
		printf("\tReading pkgs to graph\n");
	int rc = bdb_read_pkgs_to_graph(grph, params);
	assert(rc == ERR_CODE_OK);
	if (verbose)
		printf("\tClosing database\n");
	bdb_params_destroy(&params);
	if (verbose)
		printf("Read from database\n");

	if (verbose)
		printf("Resolving graph\n");
	(void)bgraph_attempt_resolution(grph);
	if (verbose)
		printf("Resolved graph\n");

	if (verbose)
		printf("Creating dot graph...\n");
	Agraph_t *dot = bdot_from_graph(grph);
	if (verbose)
		printf("Saving to file %s ...\n", to);
	bdot_save_graph(dot, to);
	bdot_close(&dot);
	if (verbose)
		printf("Done\n");
	return ERR_CODE_OK;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "hLvt:f:";
	char *default_dbpath = DEFAULT_DBPATH;
	char *dbpath = NULL;
	char *default_graphpath = DEFAULT_DOTGRAPHPATH;
	char *graphpath = NULL;


	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'f':
			dbpath = optarg;
			break;
		case 't':
			graphpath = optarg;
			break;
		case 'h':
			help();
			return 0;
		case 'L':
			print_license();
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

	if (flags & ERR_FLAG) {
		fprintf(stderr, "Exiting due to errors.\n");
		help();
		exit(ERR_CODE_BADWORLD);
	}

	prologue(argv[0]);

	if (!graphpath)
		graphpath = default_graphpath;
	if (!dbpath)
		dbpath = default_dbpath;

	return run(flags, dbpath, graphpath);
}

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
#define TOPLEVEL_ONLY_FLAG 4

void
help(void)
{
#include "dxpb-graph-to-dot-help.inc"
	printf("%.*s\n", ___doc_dxpb_graph_to_dot_help_txt_len, ___doc_dxpb_graph_to_dot_help_txt);
}

int
run(int flags, const char *from, const char *to, const char *arch)
{
	assert(from);
	int verbose = flags & VERBOSE_FLAG;

	if (verbose)
		printf("Reading from database %s\n", from);
	struct bdb_bound_params *params = bdb_params_init_ro(from);
	assert(!params->DO_NOT_USE_PARAMS);
	if (verbose)
		printf("\tCreating graph\n");
	bgraph grph = NULL;
	if (verbose)
		printf("\tReading pkgs to graph\n");
	int rc = bdb_read_pkgs_to_graph(&grph, params);
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
	Agraph_t *dot = NULL;
	if (flags & TOPLEVEL_ONLY_FLAG) {
		if (arch == NULL)
			dot = bdot_toplevel_from_graph(grph);
		else
			dot = bdot_toplevel_from_graph_for_arch(grph, arch);
	} else {
		if (arch == NULL)
			dot = bdot_from_graph(grph);
		else
			dot = bdot_from_graph_for_arch(grph, arch);
	}

	if (to) {
		if (verbose)
			printf("Saving to file %s ...\n", to);
		bdot_save_graph(dot, to);
	} else
		bdot_print_graph(dot);
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
	const char *optstring = "hLva:t:Tf:";
	char *default_dbpath = DEFAULT_DBPATH;
	char *dbpath = NULL;
	char *graphpath = NULL;
	char *arch = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'f':
			dbpath = optarg;
			break;
		case 't':
			graphpath = optarg;
			break;
		case 'a':
			arch = optarg;
			break;
		case 'h':
			prologue(argv[0]);
			help();
			return 0;
		case 'L':
			print_license();
			return 0;
		case 'v':
			flags |= VERBOSE_FLAG;
			break;
		case 'T':
			flags |= TOPLEVEL_ONLY_FLAG;
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

	if (!dbpath)
		dbpath = default_dbpath;

	return run(flags, dbpath, graphpath, arch);
}

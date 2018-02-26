/*
 * bdot.c
 *
 * Wrapper for working with graphviz
 */
#define _POSIX_C_SOURCE 200809L

#include <graphviz/cgraph.h>
#include <czmq.h>
#include "bstring.h"
#include "bwords.h"
#include "bxpkg.h"
#include "pkgimport_msg.h"
#include "bpkg.h"
#include "bgraph.h"

static char *
bdot_pkg_to_nodename(struct pkg *pkg)
{
	char *rV = NULL;
	uint32_t parA = 0, parB = 0;
	rV = bstring_add(rV, pkg->name, &parA, &parB);
	rV = bstring_add(rV, pkg_archs_str[pkg->arch], &parA, &parB);
	return rV;
}

static Agnode_t *
bdot_pkg_to_node(Agraph_t *graph, struct pkg *pkg)
{
	char *name = bdot_pkg_to_nodename(pkg);
	Agnode_t *rV = agnode(graph, name, 1);
	free(name);
	name = NULL;
	assert(rV != NILnode);
	return rV;
}

Agraph_t *
bdot_from_graph(bgraph grph)
{
	char *archname = NULL;
	zhash_t *arch = NULL;
	struct pkg *pkg = NULL;
	struct pkg_need *needpkg = NULL;
	Agraph_t *perarch = NULL;
	Agnode_t *node = NULL;

	Agraph_t *rV = agopen("dxpb-pkgs", Agdirected, NULL);

	/* First pass. Need all the nodes before I can make edges */
	for (arch = zhash_first(grph); arch; arch = zhash_next(grph)) {
		archname = strdup(zhash_cursor(grph));
		perarch = agsubg(rV, archname, 1);
		free(archname);
		archname = NULL;
		for (pkg = zhash_first(arch); pkg; pkg = zhash_next(arch)) {
			node = bdot_pkg_to_node(perarch, pkg);
			assert(node != NILnode);
		}
	}

	/* Second pass */
	Agedge_t *edge;
	for (arch = zhash_first(grph); arch; arch = zhash_next(grph)) {
		for (pkg = zhash_first(arch); pkg; pkg = zhash_next(arch)) {
			node = bdot_pkg_to_node(rV, pkg);
			for (needpkg = zlist_first(pkg->needs); needpkg;
					needpkg = zlist_next(pkg->needs)) {
				edge = agedge(rV,
						bdot_pkg_to_node(rV, needpkg->pkg),
						node,
						NULL,
						1);
				assert(edge != NILedge);
			}
			for (needpkg = zlist_first(pkg->cross_needs); needpkg;
					needpkg = zlist_next(pkg->cross_needs)) {
				edge = agedge(rV,
						bdot_pkg_to_node(rV, needpkg->pkg),
						node,
						NULL,
						1);
				assert(edge != NILedge);
			}
		}
	}
	return rV;
}

void
bdot_save_graph(Agraph_t *grph, const char *path)
{
	FILE *file = fopen(path, "w");
	agwrite(grph, file);
	fclose(file);
}

void
bdot_print_graph(Agraph_t *grph)
{
	agwrite(grph, stdout);
}

void
bdot_close(Agraph_t **grph)
{
	agclose(*grph);
	*grph = NULL;
}

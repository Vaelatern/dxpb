/*
 * bworkermatch.c
 *
 * Module helping workers find packages, and vice versa.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <czmq.h>
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bworker.h"
#include "bworkermatch.h"

int
bworkermatch_pkg(struct bworker *wrkr, bgraph arch, bgraph hostarch, struct pkg *pkg)
{
	assert(wrkr);
	assert(pkg);
	assert(arch);
	assert(hostarch);

	if (wrkr->job.name == NULL) // A fast, simple check
		if (pkg->arch == ARCH_NOARCH ||
				(wrkr->arch == pkg->arch &&
				 (pkg->can_cross || !wrkr->iscross)))
			if (bgraph_pkg_ready_to_build(pkg, arch, hostarch) == ERR_CODE_YES)
				return 1;
	return 0;
}

struct bworkermatch *
bworkermatch_grp_pkg(struct bworkgroup *grp, bgraph grph, struct pkg *pkg)
{
	assert(grp);
	assert(grph);
	assert(pkg);
	struct bworkermatch *retVal = calloc(1, sizeof(struct bworkermatch));

	bgraph pkgarch = zhash_lookup(grph, pkg_archs_str[pkg->arch]);
	assert(pkgarch);
	enum pkg_archs hostarch_num = ARCH_NUM_MAX;
	bgraph hostarch = NULL;

	struct bworker *wrkr = NULL;

	for (size_t i = 0; i < grp->num_workers; i++) {
		wrkr = grp->workers[i];
		if (hostarch_num != wrkr->hostarch) {
			hostarch_num = wrkr->hostarch;
			hostarch = zhash_lookup(grph, pkg_archs_str[hostarch_num]);
		}
		if (bworkermatch_pkg(wrkr, pkgarch, hostarch, pkg)) {
			if (retVal->num + 1 > retVal->alloced ||
					retVal->workers == NULL) {
				retVal->alloced = 2*(1+retVal->num);
				retVal->workers = realloc(retVal->workers,
						sizeof(struct bworker *)*
						retVal->alloced);
			}
			if (retVal->workers == NULL)
				exit(ERR_CODE_NOMEM);
			retVal->workers[retVal->num] = wrkr;
			retVal->num++;
		}
	}

	if (retVal == NULL) // no effect, but makes the return values clear.
		return NULL;
	if (retVal->num == 0)
		bworkermatch_destroy(&retVal); // sets retVal to NULL
	assert(retVal == NULL);
	return retVal;
}

void
bworkermatch_destroy(struct bworkermatch **todie)
{
	struct bworkermatch *die = *todie;
	free(die->workers);
	free(die);
	*todie = NULL;
}

struct bworker *
bworkermatch_group_choose(struct bworkermatch *matches)
{
	struct bworker *retVal = NULL;
	for (size_t i = 0; i < matches->num; i++)
		if (!retVal || retVal->cost > matches->workers[i]->cost)
			retVal = matches->workers[i];
	return retVal;
}

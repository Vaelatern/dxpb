/*
 * bworker.c
 *
 * Module specifying and working with the type struct worker
 */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <czmq.h>
#include <bsd/stdlib.h>
#include <bsd/sys/tree.h>
#include "bwords.h"
#include "bxpkg.h"
#include "dxpb.h"
#include "bworker.h"


static void bworker_subgroup_free(struct bworksubgroup **);
static int bworker_subgroup_assign_slot(struct bworkgroup *, struct bworksubgroup *);
static void bworker_destroy(struct bworker **);
static struct bworker *bworker_reinit(struct bworker *, uint16_t, uint32_t, enum pkg_archs, enum pkg_archs, uint8_t, uint16_t);
static struct bworker *bworker_new(uint16_t, uint32_t, enum pkg_archs, enum pkg_archs, uint8_t, uint16_t);

/* Workgroup */

struct bworkgroup *
bworker_group_new(void)
{
	struct bworkgroup *retVal = calloc(1, sizeof(struct bworkgroup));

	if (retVal == NULL)
		exit(ERR_CODE_NOMEM);

	retVal->direct_use = 1;

	return retVal;
}

struct bworker *
bworker_from_my_addr(struct bworkgroup *group, uint16_t addr, uint32_t check)
{
	if (group->workers[addr] != NULL && group->workers[addr]->check == check)
		return group->workers[addr];
	return NULL;
}

struct bworker *
bworker_from_remote_addr(struct bworkgroup *group, uint16_t addr, uint32_t check)
{
	for (uint16_t i = 0; i < group->num_workers; i++)
		if (group->workers[i]->addr == addr && group->workers[i]->mycheck == check)
			return group->workers[i];
	return NULL;
}

void
bworker_group_destroy(struct bworkgroup **byebye)
{
	struct bworkgroup *bye = *byebye;
	for (size_t i = 0; i < bye->num_subs; i++)
		bworker_subgroup_free(&(bye->subs[i]));
	for (size_t i = 0; i < bye->num_workers; i++) {
		bworker_job_remove(bye->workers[i]);
		bworker_destroy(&(bye->workers[i]));
	}
	free(bye->workers);
	free(bye->owners);
	free(bye->subs);
	free(*byebye);
	*byebye = NULL;
}

/* Ultimately, ensure we can put a worker at grp->workers[grp->num_workers] */
static void
bworker_ensure_can_add_worker(struct bworkgroup *grp)
{
	if (grp->num_workers + 1 < grp->num_workers_alloced)
		return;

	uint16_t newsize = (UINT16_MAX / 2.0) <= grp->num_workers + 1 ?
		UINT16_MAX : (grp->num_workers + 1) * 2;

	if ((grp->workers = realloc(grp->workers, newsize * sizeof(struct bworker *))) == NULL)
		exit(ERR_CODE_NOMEM);

	if (!grp->direct_use)
		if ((grp->owners = realloc(grp->owners, newsize * sizeof(struct bworksubgroup *))) == NULL)
			exit(ERR_CODE_NOMEM);

	grp->num_workers_alloced = newsize;
}

uint16_t
bworker_group_insert(struct bworkgroup *grp, uint16_t addr, uint32_t check,
		enum pkg_archs arch, enum pkg_archs hostarch, uint8_t iscross,
		uint16_t cost)
{
	assert(grp->owners == NULL);
	assert(grp->direct_use);
	struct bworker *myguy = NULL;
	for (uint16_t i = 0; i < grp->num_workers; i++) {
		if (grp->workers[i]->addr == UINT16_MAX) {
			myguy = grp->workers[i];
			bworker_reinit(myguy, addr, check, arch, hostarch, iscross, cost);
			grp->num_workers++;
			return i;
		}
	}
	if (grp->num_workers >= UINT16_MAX - 1)
		return UINT16_MAX;

	bworker_ensure_can_add_worker(grp);

	grp->workers[grp->num_workers++] = bworker_new(addr, check, arch, hostarch, iscross, cost);
	return grp->num_workers - 1;
}


/* Subgroup */

void *
bworker_owner_from_my_addr(struct bworkgroup *group, uint16_t addr, uint32_t check)
{
	if (group->workers[addr] != NULL && group->workers[addr]->check == check)
		return group->owners[addr]->owner;
	return NULL;
}

struct bworksubgroup *
bworker_subgroup_new(struct bworkgroup *grp)
{
	struct bworksubgroup *retVal = calloc(1, sizeof(struct bworksubgroup));

	if (retVal == NULL)
		exit(ERR_CODE_NOMEM);

	grp->direct_use = 0;
	retVal->grp = grp;
	retVal->addrs = zlist_new();

	if (bworker_subgroup_assign_slot(grp, retVal))
		return retVal;

	// else cleanup
	bworker_subgroup_free(&retVal);
	return NULL;
}

/* Worker */

static struct bworker *
bworker_reinit(struct bworker *wrkr, uint16_t addr, uint32_t check,
		enum pkg_archs arch, enum pkg_archs hostarch, uint8_t iscross, uint16_t cost)
{
	wrkr->addr = addr;
	wrkr->check = check;
	wrkr->cost = cost;
	wrkr->arch = arch;
	wrkr->hostarch = hostarch;
	wrkr->iscross = iscross;
	wrkr->job.name = NULL;
	wrkr->job.ver = NULL;
	wrkr->job.arch = ARCH_NUM_MAX;

	wrkr->mycheck = arc4random();

	return wrkr;
}

static struct bworker *
bworker_new(uint16_t addr, uint32_t check, enum pkg_archs arch,
		enum pkg_archs hostarch, uint8_t iscross, uint16_t cost)
{
	struct bworker *retVal = malloc(sizeof(struct bworker));
	if (!retVal) {
		perror("Could not allocate a new worker");
		exit(ERR_CODE_NOMEM);
	}

	return bworker_reinit(retVal, addr, check, arch, hostarch, iscross, cost);
}

static void
bworker_clear(struct bworker *recycle)
{
	recycle->addr = UINT16_MAX;
	recycle->arch = ARCH_NUM_MAX;
	recycle->hostarch = ARCH_NUM_MAX;
	bworker_job_remove(recycle);
}

void
bworker_group_remove(struct bworker *gone)
{
	bworker_clear(gone);
}

static void
bworker_destroy(struct bworker **todie)
{
	free(*todie);
	*todie = NULL;
}

/* This is so a non-fixed-size bworker is possible, since all subgroups
 * are calculated here instead of by users. Users need never know how we
 * arrange stuff in this module.
 */
struct bworksubgroup *
bworker_subgroup_from_addr(struct bworkgroup *grp, size_t addr)
{
	return grp->owners[addr];
}

static int
bworker_subgroup_assign_slot(struct bworkgroup *grp, struct bworksubgroup *sub)
{
	assert(grp);
	assert(sub);

	/* Either we have workers and they have owners, or we have neither. */
	assert((grp->owners == NULL) == (grp->workers == NULL));

	/* Have to be careful from now. This could be an entry point for denial
	 * of service, and it's just easier to be careful and more restrictive
	 * now.
	 * As of time of writing, uint16_t is used for number of subgroups.
	 * I don't believe this would be a plausible DoS attack anyway. But
	 * good to be safe anyway.
	 */

	uint16_t newlen = grp->num_subs + 1;
	uint16_t alloced = grp->num_subs_alloced;

	if (newlen >= alloced) {
		alloced = (UINT16_MAX / 2.0) <= newlen ? UINT16_MAX : newlen * 2;
		void *tmp;
		if ((tmp = realloc(grp->subs, alloced * sizeof(struct bworksubgroup *))) == NULL)
			return 0; // Safe because grp->subs hasn't been changed.
		else {
			grp->subs = tmp;
			grp->num_subs_alloced = alloced;
		}
	}

	grp->subs[grp->num_subs++] = sub;

	return 1;
}


void
bworker_subgroup_destroy(struct bworksubgroup **sub)
{
	struct bworkgroup *grp = (*sub)->grp;
	struct bworksubgroup *newsub = *sub;
	uint16_t *index;
	while ((index = zlist_pop(newsub->addrs))) {
		bworker_clear(grp->workers[*index]);
		assert(grp->owners[*index] == newsub);
		grp->owners[*index] = NULL;
		free(index);
		index = NULL;
	}
	for (size_t i = 0; i < grp->num_subs; i++) {
		if (grp->subs[i] == *sub) {
			bworker_subgroup_free(&(grp->subs[i]));
			*sub = grp->subs[i];
			assert(*sub == NULL);
			grp->subs[i] = grp->subs[grp->num_subs - 1];
			grp->subs[grp->num_subs - 1] = NULL;
			grp->num_subs--;
			break;
		}
	}
	return;
}

static void
bworker_subgroup_free(struct bworksubgroup **sacrifice)
{
	struct bworksubgroup *lamb = *sacrifice;
	zlist_destroy(&(lamb->addrs));
	free(lamb);
	*sacrifice = NULL;
}

/*
 * Use multiple return to simplify execution. First try to reinit
 * a pre-existing memory structure which has fallen into disuse, first for the
 * subgroup, then for all subgroups. Then, if all else fails, make a new worker
 * structure and use that
 */
uint16_t
bworker_subgroup_insert(struct bworksubgroup *grp, uint16_t addr,
		uint32_t check, enum pkg_archs arch, enum pkg_archs hostarch,
		uint8_t iscross, uint16_t cost)
{
	assert(grp->grp->direct_use);
	struct bworkgroup *topgrp = grp->grp;
	struct bworker *wrkr = NULL;
	uint16_t *i = NULL, j = 0;
	for (i = zlist_first(grp->addrs); i; i = zlist_next(grp->addrs))
		if (topgrp->workers[*i] == NULL) {
			wrkr = topgrp->workers[*i];
			break;
		} else
			j = *i;

	if (i) {
		bworker_reinit(wrkr, addr, check, arch, hostarch, iscross, cost);
		return *i;
	}

	// Attempt 1 over

	// Don't read any of the links again, j is already set to latest
	// "failed" *i value
	for (; j < topgrp->num_workers; j++) {
		if (topgrp->owners[j] == NULL) { // time to adopt it
			topgrp->owners[j] = grp;
			if (topgrp->workers[j] == NULL)
				topgrp->workers[j] = bworker_new(addr, check,
						arch, hostarch, iscross, cost);
			else
				bworker_reinit(topgrp->workers[j], addr, check,
						arch, hostarch, iscross, cost);
			return j;
		}
	}

	// Attempt 2 over

	bworker_ensure_can_add_worker(topgrp);

	topgrp->workers[topgrp->num_workers++] = bworker_new(addr, check, arch,
							hostarch, iscross, cost);

	return topgrp->num_workers - 1;
}


/* Worker Job Management */

int
bworker_job_assign(struct bworker *wrkr, const char *name, const char *ver, enum pkg_archs arch)
{
	assert(wrkr);
	assert(name);
	assert(ver);
	assert(arch < ARCH_HOST);

	bworker_job_remove(wrkr);

	wrkr->job.name = strdup(name);
	wrkr->job.ver = strdup(ver);
	wrkr->job.arch = arch;

	if (wrkr->job.name == NULL)
		exit(ERR_CODE_NOMEM);
	if (wrkr->job.ver == NULL)
		exit(ERR_CODE_NOMEM);
	return 1;
}

void
bworker_job_remove(struct bworker *wrkr)
{
	assert(wrkr);
	if (wrkr->job.name != NULL) {
		free(wrkr->job.name);
		wrkr->job.name = NULL;
	}
	if (wrkr->job.ver != NULL) {
		free(wrkr->job.ver);
		wrkr->job.ver = NULL;
	}
	wrkr->job.arch = ARCH_NUM_MAX;
}

int
bworker_job_equal(struct bworker *wrkr, char *name, char *ver, enum pkg_archs arch)
{
	assert(wrkr);
	assert(name);
	assert(ver);

	if (arch >= ARCH_HOST || wrkr->job.arch >= ARCH_HOST)
		return 0;
	if (wrkr->job.arch != arch)
		return 0;
	if (strcmp(wrkr->job.ver, ver) != 0)
		return 0;
	if (strcmp(wrkr->job.name, name) != 0)
		return 0;

	return 1;
}


/* Worker Matching */

int
bworker_pkg_match(struct bworker *wrkr, struct pkg *pkg)
{
	assert(wrkr);
	assert(pkg);
	if (wrkr->job.name == NULL) // A fast, simple check
		if (wrkr->arch == pkg->arch && (pkg->can_cross || !wrkr->iscross))
			return 1;
	return 0;
}

struct bworkermatch *
bworker_grp_pkg_matches(struct bworkgroup *grp, struct pkg *pkg)
{
	assert(grp);
	assert(pkg);
	struct bworkermatch *retVal = calloc(1, sizeof(struct bworkermatch));

	for (size_t i = 0; i < grp->num_workers; i++)
		if (bworker_pkg_match(grp->workers[i], pkg)) {
			if (retVal->num + 1 > retVal->alloced ||
					retVal->workers == NULL) {
				retVal->alloced = 2*(1+retVal->num);
				retVal->workers = realloc(retVal->workers,
						sizeof(struct bworker *)*
						retVal->alloced);
			}
			if (retVal->workers == NULL)
				exit(ERR_CODE_NOMEM);
			retVal->workers[retVal->num] = grp->workers[i];
			retVal->num++;
		}

	if (retVal == NULL) // no effect, but makes the return values clear.
		return NULL;
	return retVal;
}

void
bworker_match_destroy(struct bworkermatch **todie)
{
	struct bworkermatch *die = *todie;
	free(die->workers);
	free(die);
	*todie = NULL;
}

struct bworker *
bworker_group_matches_choose(struct bworkermatch *matches)
{
	struct bworker *retVal = NULL;
	for (size_t i = 0; i < matches->num; i++)
		if (!retVal || retVal->cost > matches->workers[i]->cost)
			retVal = matches->workers[i];
	return retVal;
}

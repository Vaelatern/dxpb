/*
 * bgraph.c
 *
 * Simple module to, given constraints and a package graph, pick the next job
 * to run. This is deliberately NOT threadsafe, depending on the hash
 * implementation used.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "pkgimport_msg.h"
#include "bpkg.h"
#include "bgraph.h"
#include "bxbps.h"

static void bgraph_destroy_need(struct pkg_need **, struct pkg *);

#define BGRAPH_VPKG_STR "virtual?pkg"
#define BGRAPH_BAD_TRIES 3

bgraph
bgraph_new()
{
	zhash_t *retVal = zhash_new();
	enum pkg_archs i = ARCH_NOARCH;

	zhash_insert(retVal, BGRAPH_VPKG_STR, zhash_new());
	while (pkg_archs_str[i] != NULL && i < ARCH_NUM_MAX) {
		if (i != ARCH_TARGET)
			zhash_insert(retVal, pkg_archs_str[i], zhash_new());
		i++;
	}

	return retVal;
}

void
bgraph_set_vpkgs(bgraph grph, zhash_t *vpkgs)
{
	zhash_freefn(vpkgs, BGRAPH_VPKG_STR, free);
	zhash_t *destroy = zhash_lookup(grph, BGRAPH_VPKG_STR);
	assert(destroy);
	zhash_delete(grph, BGRAPH_VPKG_STR);
	zhash_insert(grph, BGRAPH_VPKG_STR, vpkgs);
	zhash_destroy(&destroy);
}

zhash_t *
bgraph_get_vpkgs(bgraph grph)
{
	return zhash_lookup(grph, BGRAPH_VPKG_STR);
}

static void
bgraph_un_needs_me(struct pkg *poor_soul, struct pkg *origin)
{
	struct pkg_need *i;
	zlist_t *cur_list = poor_soul->cross_needs;
	for (i = zlist_first(cur_list); i != NULL; i = zlist_next(cur_list)) {
		if (i->pkg == origin) {
			zlist_remove(cur_list, i);
			bgraph_destroy_need(&i, poor_soul);
			poor_soul->resolved = 0;
		}
	}

	cur_list = poor_soul->needs;
	for (i = zlist_first(cur_list); i != NULL; i = zlist_next(cur_list)) {
		if (i->pkg == origin) {
			zlist_remove(cur_list, i);
			bgraph_destroy_need(&i, poor_soul);
			poor_soul->resolved = 0;
		}
	}
}

static void
bgraph_destroy_pkg(void *sacrifice)
{
	assert(sacrifice);
	struct pkg *lamb = sacrifice;
	struct pkg_need *i = NULL;
	struct pkg *j = NULL;
	assert(lamb->name);
	assert(lamb->ver);
	assert(lamb->cross_needs);
	assert(lamb->needs);
	assert(lamb->needs_me);
	while ((i = zlist_pop(lamb->cross_needs)) != NULL) {
		bgraph_destroy_need(&i, lamb);
	}
	while ((i = zlist_pop(lamb->needs)) != NULL) {
		bgraph_destroy_need(&i, lamb);
	}
	while ((j = zlist_pop(lamb->needs_me)) != NULL) {
		bgraph_un_needs_me(j, lamb);
	}
	bpkg_destroy(sacrifice);
}

void
bgraph_destroy(bgraph *deadmeat)
{
	bgraph goner = *deadmeat;
	struct pkg *child;
	assert(goner);
	for (zhash_t *tmp = zhash_first(goner); tmp; tmp = zhash_next(goner)) {
		assert(tmp);
		for (child = zhash_first(tmp); child != NULL; child = zhash_next(tmp)) {
			zhash_delete(tmp, zhash_cursor(tmp));
		}
		zhash_delete(goner, zhash_cursor(goner));
		zhash_destroy(&tmp);
	}
	zhash_destroy(deadmeat);
}

int
bgraph_insert_pkg(bgraph grph, struct pkg *newguy)
{
	/* One of the things this code doesn't do, but at the same time does 
	 * do, is fix the graph if a package changes from noarch to caring 
	 * about architecture. How does it not do that? There is minimal code
	 * to handle the case. How DOES it do that? Let's walk through the 
	 * code.
	 *  - We open on a sunny beach. The birds are singing. assert(birds);
	 *  - We check if the pkg is of type ARCH_TARGET, and if so, pick the
	 *    noarch bird. We notice the bird is significantly bigger than the
	 *    rest, and that's because this bird contains a copy of every
	 *    single package, no matter if broken or old or new and working
	 *    great on only one specific architecture. This is important.
	 *  - If the package is not ARCH_TARGET or ARCH_NOARCH, our story will 
	 *    continue when you do find a package of those types. It will 
	 *    happen eventually for every package.
	 *  - We don't xray the bird to determine that it contains two types of 
	 *    packages, ARCH_NOARCH and ARCH_TARGET. But we could, and we would 
	 *    discover, in the grapher steady-state (not when starting up) that 
	 *    this would be true.
	 *  - We ask the bird for a current package of it's name. It complies.
	 *  - We investigate the package, if it exists, to confirm that yes, 
	 *    everything is in order. If however, the package has become 
	 *    noarch, then there will be nothing to inform the other birds. So 
	 *    go get them and remove the obsolete packages. This is the only 
	 *    code that deals with the case of going from noarch to arch or 
	 *    arch to noarch.
	 *  - After everything is in order, let the bird go as the old package 
	 *    dies a nice death. It served us well.
	 */
	assert(grph);
	assert(newguy);
	const char *trgtarch;
	bgraph relevant;
	if (newguy->arch == ARCH_TARGET)
		trgtarch = pkg_archs_str[ARCH_NOARCH];
	else
		trgtarch = pkg_archs_str[newguy->arch];
	assert(trgtarch);
	relevant = zhash_lookup(grph, trgtarch);

	if (relevant == NULL)
		return ERR_CODE_BADDOBBY;

	struct pkg *oldguy = zhash_lookup(relevant, newguy->name);
	if (oldguy) /* A pre-existing element */
		/* The only case we don't handle by magic */
		if (oldguy->arch == ARCH_TARGET && newguy->arch == ARCH_NOARCH)
			for (enum pkg_archs i = ARCH_NOARCH + 1; i <= ARCH_HOST; i++)
				zhash_delete(zhash_lookup(grph, pkg_archs_str[i]), newguy->name);
	zhash_update(relevant, newguy->name, newguy);
	oldguy = zhash_freefn(relevant, newguy->name, bgraph_destroy_pkg);
	assert(oldguy);

	return ERR_CODE_OK;
}

inline static struct pkg *
bgraph_find_pkg(zhash_t *arch, zhash_t *noarch, const char *pkgname)
{
	struct pkg *retVal = NULL;
	assert(pkgname);
	if (arch != NULL)
		retVal = zhash_lookup(arch, pkgname);
	if (noarch != NULL && retVal == NULL)
		retVal = zhash_lookup(noarch, pkgname);

	return retVal;
}

static void
bgraph_destroy_need(struct pkg_need **lamb, struct pkg *goner)
{
	zlist_remove((*lamb)->pkg->needs_me, goner);
	(*lamb)->pkg->priority--;
	(*lamb)->pkg = NULL;
	(*lamb)->spec = NULL;
	FREE(*lamb);
}

inline static struct pkg_need *
bgraph_new_need(const char *spec, struct pkg *pkg)
{
	struct pkg_need *tmp;
	if ((tmp = malloc(sizeof(struct pkg_need))) == NULL) {
		perror("Could not allocate memory for needs");
		exit(ERR_CODE_NOMEM);
	}
	tmp->spec = spec;
	tmp->pkg = pkg;
	pkg->priority++;
	return tmp;
}

/* Sends back haystacks */
static void
bgraph_pitchfork(bgraph grph, const char *arch, zhash_t **hay, zhash_t **allhay,
				zhash_t **hosthay)
{
	assert(grph);

	// Primary stack of packages
	if (hay) {
		assert(arch);
		*hay = zhash_lookup(grph, arch);
		assert(*hay);
	}

	// Fallback stack
	if (allhay) {
		*allhay = zhash_lookup(grph, pkg_archs_str[ARCH_NOARCH]);
		assert(*allhay);
	}

	// Host stack
	if (hosthay) {
		*hosthay = zhash_lookup(grph, pkg_archs_str[ARCH_HOST]);
		assert(*hosthay);
	}
}

static int
bgraph_resolve_wneed(bgraph hay, bgraph allhay, zhash_t *virt, bwords curwords, void *ineed, struct pkg *me)
{
	struct pkg *curpkg = NULL;
	size_t i = -1;
	assert(curwords != NULL);// no deps at all! For some reason
	for (i = 0; i < curwords->num_words; i++) {
		char *curpkgname = bxbps_get_pkgname(curwords->words[i], allhay, virt);
		if (curpkgname == NULL)
			goto badwant;
		curpkg = bgraph_find_pkg(hay, allhay, curpkgname);
		if (curpkg == NULL)
			goto badwant;
		FREE(curpkgname); /* mandated by the xbps functions */
		zlist_append(ineed, bgraph_new_need(curwords->words[i], curpkg));
		zlist_append(curpkg->needs_me, me);
	}
	return ERR_CODE_OK;
badwant:
	assert(i >= 0);
	fprintf(stderr, "The following spec is unacceptable: %s\n", curwords->words[i]);
	return ERR_CODE_BADDEP;
}

static int
bgraph_resolve_pkg(bgraph hay, bgraph allhay, bgraph hosthay, zhash_t *virt, struct pkg *subj)
{
	assert(allhay);
	assert(hay);
	assert(hosthay);
	if (subj == NULL)
		return ERR_CODE_BADPKG;

	if (subj->resolved)
		return ERR_CODE_OK;

	int rc;

	rc = bgraph_resolve_wneed(hosthay, allhay, virt, subj->wneeds_cross_host,
			subj->cross_needs, subj);
	if (rc == ERR_CODE_BADDEP)
		goto badwant;

	rc = bgraph_resolve_wneed(hay, allhay, virt, subj->wneeds_cross_target,
			subj->cross_needs, subj);
	if (rc == ERR_CODE_BADDEP)
		goto badwant;

	rc = bgraph_resolve_wneed(hosthay, allhay, virt, subj->wneeds_native_host,
			subj->needs, subj);
	if (rc == ERR_CODE_BADDEP)
		goto badwant;

	rc = bgraph_resolve_wneed(hay, allhay, virt, subj->wneeds_native_target,
			subj->needs, subj);
	if (rc == ERR_CODE_BADDEP)
		goto badwant;

	subj->resolved = 1;
	return ERR_CODE_OK;
badwant:
	subj->bad = BGRAPH_BAD_TRIES;
	return ERR_CODE_BADDEP;
}

int
bgraph_attempt_resolution(bgraph grph)
{
	assert(grph);
	bgraph hay, allhay, hosthay;
	int retVal = ERR_CODE_OK;
	int rc = ERR_CODE_BAD;

	// The zhash functions are not at all thread safe, or I'd parallelize
	// this loop with openmp. Not even the lookup functions are thread
	// safe. But this could be fixed by forking czmq and modifying the
	// function s_item_lookup to not use self-> variables as temporary
	// storage.
	// Food for thought.
	// Vaelatern, 2017-07-10
	allhay = hosthay = NULL;
	bgraph_pitchfork(grph, pkg_archs_str[ARCH_NOARCH], NULL, &allhay, &hosthay);
	zhash_t *virt = bgraph_get_vpkgs(grph);
	for (enum pkg_archs arch = ARCH_NOARCH; arch < ARCH_HOST; arch++) {
		hay = NULL;
		bgraph_pitchfork(grph, pkg_archs_str[arch], &hay, NULL, NULL);
		assert(hay);
		for (struct pkg *needle = zhash_first(hay); needle != NULL;
						needle = zhash_next(hay)) {
			rc = bgraph_resolve_pkg(hay, allhay, hosthay, virt, needle);
			retVal = retVal == ERR_CODE_OK ? rc : retVal;
		}
	}
	return retVal;
}

enum ret_codes
bgraph_pkg_ready_to_build(struct pkg *needle, bgraph hay, bgraph hosthay, int cross)
{
	enum ret_codes rc = ERR_CODE_OK;
	struct pkg *pin;
	if (needle->status != PKG_STATUS_TOBUILD ||
			needle->arch == ARCH_TARGET ||
			needle->arch == ARCH_HOST ||
			(needle->bad >= BGRAPH_BAD_TRIES && !needle->bootstrap))
		return ERR_CODE_NO;
	assert(!bpkg_is_virtual(needle));
	zlist_t *needs = needle->needs;
	if (cross)
		needs = needle->cross_needs;
	for (struct pkg_need *curneed = zlist_first(needs);
			curneed != NULL; curneed = zlist_next(needs)) {
		pin = curneed->pkg;
		assert(pin);
		assert(pin->name);
		rc = bxbps_spec_match(curneed->spec, pin->name, pin->ver);
		if (rc != ERR_CODE_YES)
			return rc;
		if (pin->arch == ARCH_TARGET && hay != NULL) {
			pin = zhash_lookup(hay, pin->name);
		} else if (pin->arch == ARCH_HOST && hosthay != NULL) {
			assert(hosthay != NULL);
			pin = zhash_lookup(hosthay, pin->name);
		} else if (pin->arch == ARCH_HOST || pin->arch == ARCH_TARGET)
			continue;
		if (pin->arch == ARCH_HOST || pin->arch == ARCH_TARGET)
			continue;
		assert(pin); // even if noarch, pin will be not null
		if (pin->status != PKG_STATUS_IN_REPO)
			return ERR_CODE_NO;
	}
	return ERR_CODE_YES;
}

static int
bgraph_zlist_filter_cb(struct pkg *needle, int only_bootstrap)
{
	/* Want to enable restricted packages? Remove part of this condition */
	if (needle->restricted || (only_bootstrap && !needle->bootstrap))
		return 0;
	return 1;
}

static void
bgraph_zlist_filter(zlist_t *list, int (*cb)(struct pkg *, int),
		int found_bootstrap)
{
	assert(list);
	assert(cb);
	for (struct pkg *needle = zlist_first(list); needle != NULL;
			needle = zlist_next(list))
		/* zlist_remove() is safe to use when iterating like this */
		if (!cb(needle, found_bootstrap))
			zlist_remove(list, needle);
}

/* This code will be run a lot.
 * It must be very fast.
 */
zlist_t *
bgraph_what_next_for_arch(bgraph grph, enum pkg_archs arch)
{
	struct pkg *needle;
	bgraph hay;
	int found_bootstrap = 0;
	zlist_t *retVal = zlist_new();

	hay = zhash_lookup(grph, pkg_archs_str[arch]);
	assert(hay != NULL);
	for (needle = zhash_first(hay); needle != NULL; needle = zhash_next(hay))
		if (bgraph_pkg_ready_to_build(needle, NULL, NULL, 0) == ERR_CODE_YES) {
			zlist_append(retVal, needle);
			found_bootstrap = found_bootstrap || (needle->bootstrap && (!needle->broken));
		}

	bgraph_zlist_filter(retVal, bgraph_zlist_filter_cb, found_bootstrap);

	return retVal;
}

enum bgraph_pkg_mark_type {
	BGRAPH_PKG_MARK_TYPE_IN_REPO,
	BGRAPH_PKG_MARK_TYPE_BAD,
	BGRAPH_PKG_MARK_TYPE_IN_PROGRESS,
	BGRAPH_PKG_MARK_TYPE_NUM_MAX
};

struct pkg *
bgraph_get_pkg(const bgraph grph, const char *pkgname, const char *version, enum pkg_archs arch)
{
	assert(grph);
	assert(pkgname);
	assert(version);
	enum pkg_archs tmparch = arch;
	if (arch == ARCH_TARGET)
		tmparch = ARCH_NOARCH;
	assert(arch < ARCH_HOST || arch == ARCH_TARGET);
	bgraph archgraph = zhash_lookup(grph, pkg_archs_str[tmparch]);
	assert(archgraph != NULL);
	struct pkg *pkg = zhash_lookup(archgraph, pkgname);

	assert(pkg->arch == arch);
	if (strcmp(version, pkg->ver) != 0)
		return NULL;
	return pkg;
}

static int
bgraph_mark_pkg(const bgraph grph, const char *pkgname, const char *version, enum pkg_archs arch,
		enum bgraph_pkg_mark_type type, int val)
{
	int rV = ERR_CODE_OK;
	struct pkg *pkg = bgraph_get_pkg(grph, pkgname, version, arch);

	if (pkg == NULL)
		return ERR_CODE_NO;

	enum bgraph_pkg_mark_type tobuild = bpkg_is_virtual(pkg) ? PKG_STATUS_NONE : PKG_STATUS_TOBUILD;

	switch(type) {
	case BGRAPH_PKG_MARK_TYPE_IN_REPO:
		// This logic was written before pkg->status existed, and was
		// instead a complicated bundle of one-bit flags. If there is
		// something to overhaul, this might be it. But the functions
		// may actually represent actual package statuses, so hey.
		pkg->status = val ? PKG_STATUS_IN_REPO : tobuild;
		break;
	case BGRAPH_PKG_MARK_TYPE_BAD:
		pkg->bad += (pkg->bad < BGRAPH_BAD_TRIES) ? 1 : 0;
		if (pkg->bad < BGRAPH_BAD_TRIES)
			rV = ERR_CODE_NO; // No we will NOT mark it bad... yet.
		break;
	case BGRAPH_PKG_MARK_TYPE_IN_PROGRESS:
		// This logic was written before pkg->status existed, and was
		// instead a complicated bundle of one-bit flags. If there is
		// something to overhaul, this might be it. But the functions
		// may actually represent actual package statuses, so hey.
		pkg->status = val ? PKG_STATUS_BUILDING : tobuild;
		break;
	default:
		exit(ERR_CODE_BADDOBBY);
	}
	return rV;
}

int
bgraph_mark_pkg_bad(bgraph grph, char *pkgname, char *version, enum pkg_archs arch)
{
	return bgraph_mark_pkg(grph, pkgname, version, arch, BGRAPH_PKG_MARK_TYPE_BAD, 1);
}

int
bgraph_mark_pkg_present(bgraph grph, char *pkgname, char *version, enum pkg_archs arch)
{
	return bgraph_mark_pkg(grph, pkgname, version, arch, BGRAPH_PKG_MARK_TYPE_IN_REPO, 1);
}

int
bgraph_mark_pkg_absent(bgraph grph, char *pkgname, char *version, enum pkg_archs arch)
{
	return bgraph_mark_pkg(grph, pkgname, version, arch, BGRAPH_PKG_MARK_TYPE_IN_REPO, 0);
}

int
bgraph_mark_pkg_in_progress(bgraph grph, char *pkgname, char *version, enum pkg_archs arch)
{
	return bgraph_mark_pkg(grph, pkgname, version, arch, BGRAPH_PKG_MARK_TYPE_IN_PROGRESS, 1);
}

int
bgraph_mark_pkg_not_in_progress(bgraph grph, char *pkgname, char *version, enum pkg_archs arch)
{
	return bgraph_mark_pkg(grph, pkgname, version, arch, BGRAPH_PKG_MARK_TYPE_IN_PROGRESS, 0);
}

/*
 * bpkg.c
 *
 * An in-memory representation and tools for manipulating packages.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pkgimport_msg.h"
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bxsrc.h"
#include "bstring.h"
#include "bpkg.h"

static void
bpkg_create_allowed_archs(char *allowed_archs, bwords only_for_archs)
{
	int len = only_for_archs->num_words;
	int i, j, num_allowed = 0;
	/* It is obvious that only_for_archs will be shorter than the
	 * exhasutive list of all possible archs, and that if len == 0
	 * everything is allowed */
	for (i = ARCH_NOARCH; i < ARCH_NUM_MAX; i++)
		allowed_archs[i] = (len == 0);
	for (i = ARCH_NOARCH+1; i < ARCH_NUM_MAX && num_allowed < len; i++)
		for (j = 0; j < len; j++)
			if (!strcmp(pkg_archs_str[i], only_for_archs->words[j])) {
				allowed_archs[i] = 1;
				num_allowed++;
				break;
			}
}

void
bpkg_destroy(struct pkg *lamb)
{
	if (lamb == NULL)
		return;
	FREE(lamb->name);
	FREE(lamb->ver);
	bwords_destroy(&(lamb->wneeds_cross_host), 1);
	bwords_destroy(&(lamb->wneeds_cross_target), 1);
	bwords_destroy(&(lamb->wneeds_native_host), 1);
	bwords_destroy(&(lamb->wneeds_native_target), 1);
	zlist_destroy((zlist_t **) &(lamb->cross_needs));
	zlist_destroy((zlist_t **) &(lamb->needs));
	zlist_destroy((zlist_t **) &(lamb->needs_me));
	FREE(lamb);
	return;
}

struct pkg *
bpkg_init_basic()
{
	struct pkg *new_guy;
	if ((new_guy = calloc(1, sizeof(struct pkg))) == NULL) {
		perror("Couldn't allocate new package");
		exit(ERR_CODE_NOMEM);
	}
	new_guy->cross_needs = zlist_new();
	new_guy->needs = zlist_new();
	new_guy->needs_me = zlist_new();
	return new_guy;
}

static struct pkg *
bpkg_init_new(const char *name, const char *ver)
{
	struct pkg *new_guy;
	assert(name);
	assert(ver);
	new_guy = bpkg_init_basic();
	new_guy->name = strdup(name);
	new_guy->ver = strdup(ver);
	if (!new_guy->name) {
		perror("Couldn't allocate new package name");
		exit(ERR_CODE_NOMEM);
	}
	if (!new_guy->ver) {
		perror("Couldn't allocate new package version");
		exit(ERR_CODE_NOMEM);
	}
	return new_guy;
}

struct pkg *
bpkg_init_oneoff(const char *name, const char *ver, enum pkg_archs arch,
		int can_cross, int bootstrap, int restricted)
{
	struct pkg *new_guy = bpkg_init_new(name, ver);
	new_guy->arch = arch;
	new_guy->can_cross = can_cross;
	new_guy->bootstrap = bootstrap;
	new_guy->restricted = restricted;
	return new_guy;
}

static void
bpkg_init_crosses(const char *xbps_src, struct pkg *new_guy, const char *main_name,
		enum pkg_archs target, enum pkg_archs host, char is_subpkg)
{
	assert(new_guy->can_cross);
	int fds[3];
	pid_t c_pid = bxsrc_init_read(xbps_src, new_guy->name, fds, target, host);
	/* This line is for those templates which set broken on CROSS_BUILD */
	new_guy->can_cross = !(bxsrc_q_isset(fds, "broken") && !new_guy->broken);
	assert(new_guy->wneeds_cross_host == NULL);
	assert(new_guy->wneeds_cross_target == NULL);
	if (!is_subpkg && !new_guy->broken) { /* This is the real package */
		new_guy->wneeds_cross_host =
			bwords_merge_words(bxsrc_q_to_words(fds, "hostmakedepends"), NULL);
		new_guy->wneeds_cross_target =
			bwords_merge_words(bxsrc_q_to_words(fds, "makedepends"), NULL);
		assert(new_guy->wneeds_cross_host != NULL);
		assert(new_guy->wneeds_cross_target != NULL);
	}
	if (!new_guy->broken) {
		new_guy->wneeds_cross_target =
			bwords_merge_words(
				new_guy->wneeds_cross_target,
				bwords_append_word(bxsrc_q_to_words(fds, "depends"), main_name, 0)
				);
		assert(new_guy->wneeds_cross_target != NULL);
	}
	bxsrc_close(fds, c_pid);
}

static struct pkg *
bpkg_clone(struct pkg *old_guy)
{
	assert(old_guy);
	assert(old_guy->name);
	assert(old_guy->ver);
	struct pkg *new_guy = bpkg_init_new(old_guy->name, old_guy->ver);
	new_guy->bootstrap = old_guy->bootstrap;
	new_guy->can_cross = old_guy->can_cross;
	new_guy->broken = old_guy->broken;
	new_guy->restricted = old_guy->restricted;
	new_guy->arch = old_guy->arch;
	// TODO: Clone the dependencies too.
	// This is not yet necessary.
	return new_guy;
}

inline static struct pkg *
bpkg_init_generic_target(struct pkg *template)
{
	struct pkg *new_guy = bpkg_init_new(template->name, template->ver);
	new_guy->arch = ARCH_TARGET;
	return new_guy;
}

inline static struct pkg *
bpkg_init_generic_host(struct pkg *template)
{
	struct pkg *new_guy = bpkg_init_new(template->name, template->ver);
	new_guy->arch = ARCH_HOST;
	return new_guy;
}

/* In the following, we make some tests against the current broken
 * understanding, to help determine if additional processing is necessary.
 */
static struct pkg *
bpkg_init_single(const char *xbps_src, struct pkg *template, const char *main_name,
		enum pkg_archs arch_spec, char is_subpkg)
{
	int fds[3];
	struct pkg *new_guy;
	pid_t c_pid = bxsrc_init_read(xbps_src, template->name, fds, arch_spec, ARCH_NUM_MAX);
	new_guy = bpkg_clone(template);
	new_guy->arch = arch_spec;
	new_guy->broken = new_guy->broken || bxsrc_q_isset(fds, "broken");
	assert(new_guy->wneeds_native_host == NULL);
	assert(new_guy->wneeds_native_target == NULL);
	if (!is_subpkg && !new_guy->broken) { /* This is the real package */
		new_guy->wneeds_native_host =
			bwords_merge_words(bxsrc_q_to_words(fds, "hostmakedepends"), NULL);
		new_guy->wneeds_native_target =
			bwords_merge_words(bxsrc_q_to_words(fds, "makedepends"), NULL);
		assert(new_guy->wneeds_native_host != NULL);
		assert(new_guy->wneeds_native_target != NULL);
	}
	if (!new_guy->broken) { /* Never needs build deps, but has its own deps */
		new_guy->wneeds_native_target =
			bwords_merge_words(
				new_guy->wneeds_native_target,
				bwords_append_word(bxsrc_q_to_words(fds, "depends"), main_name, 0)
				);
		assert(new_guy->wneeds_native_target != NULL);
	}
	new_guy->broken = new_guy->broken || bxsrc_did_err(fds);
	bxsrc_close(fds, c_pid);
	if (new_guy->can_cross)
		bpkg_init_crosses(xbps_src, new_guy, main_name, arch_spec,
				ARCH_AARCH64_MUSL, is_subpkg);
	return new_guy;
}

/* This will return a pkg with only general settings pre-set. Nothing that is
 * expected to be architecture or libc dependent will go here. If someone later
 * becomes flexible based on libc or architecture, parts of this must be moved
 * to other functions in the bpkg_init family.
 */
static struct pkg *
bpkg_init_template(const char *xbps_src, const char *name)
{
	struct pkg *retVal;
	bwords tmp;
	char *version;
	int fds[3];
	pid_t c_pid = bxsrc_init_read(xbps_src, name, fds, ARCH_X86_64, ARCH_NUM_MAX);

	/* Get version_revision */
	tmp = bxsrc_q_to_words(fds, "version");
	if (!tmp || !(tmp->words) || !(tmp->words[0]) || tmp->words[1]) {
		fprintf(stderr, "Unable to get version from template in package read!");
		exit(ERR_CODE_BADPKG);
	}
	version = strdup(tmp->words[0]);
	if (version == NULL) {
		perror("Unable to strdup a version");
		exit(ERR_CODE_NOMEM);
	}
	bwords_destroy(&tmp, 1);

	tmp = bxsrc_q_to_words(fds, "revision");
	assert(tmp->words[0] != NULL);
	assert(tmp->words[1] == NULL);
	version = bstring_add(bstring_add(version, "_", NULL, NULL), tmp->words[0], NULL, NULL);
	bwords_destroy(&tmp, 1);

	retVal = bpkg_init_new(name, version);

	/* Finish initialization */
	retVal->can_cross = !bxsrc_q_isset(fds, "nocross");
	retVal->bootstrap = bxsrc_q_isset(fds, "bootstrap");
	retVal->restricted = bxsrc_q_isset(fds, "restricted");

	/* Clean up */
	bxsrc_close(fds, c_pid);
	free(version);

	return retVal;
}

struct pkg *
bpkg_init(const char *name, const char *ver, const char *arch)
{
	struct pkg *retVal;
	retVal = bpkg_init_new(name, ver);
	retVal->arch = bpkg_enum_lookup(arch);
	return retVal;
}

struct pkg *
bpkg_read(pkgimport_msg_t *message)
{
	struct pkg *retVal;
	const char *tmpA, *tmpB;
	tmpA = pkgimport_msg_pkgname(message);
	tmpB = pkgimport_msg_version(message);
	retVal = bpkg_init_new(tmpA, tmpB);
	retVal->arch = bpkg_enum_lookup(pkgimport_msg_arch(message));
	retVal->can_cross = pkgimport_msg_cancross(message);
	retVal->broken = pkgimport_msg_broken(message);
	retVal->bootstrap = pkgimport_msg_bootstrap(message);
	retVal->restricted = pkgimport_msg_restricted(message);
	retVal->wneeds_cross_host =
		bwords_from_units(pkgimport_msg_crosshostneeds(message));
	retVal->wneeds_cross_target =
		bwords_from_units(pkgimport_msg_crosstargetneeds(message));
	retVal->wneeds_native_host =
		bwords_from_units(pkgimport_msg_nativehostneeds(message));
	retVal->wneeds_native_target =
		bwords_from_units(pkgimport_msg_nativetargetneeds(message));
	retVal->status = PKG_STATUS_NONE;
	return retVal;
}

struct pkg_importer *
bpkg_read_begin(const char *xbps_src, const char *name)
{
	assert(name != NULL);
	bwords tmp, only_for_archs;

	struct pkg_importer *retVal = calloc(1, sizeof(struct pkg_importer));
	if (!bxsrc_does_exist(xbps_src, name)) {
		retVal->actual_name = NULL;
		return retVal;
	}
	retVal->template = bpkg_init_template(xbps_src, name);
	retVal->archnow = ARCH_NOARCH;
	retVal->archnext = retVal->archnow + 1;

	/* Get basics */
	int fds[3];
	pid_t c_pid = bxsrc_init_read(xbps_src, name, fds, ARCH_X86_64, ARCH_NUM_MAX);
	retVal->is_noarch = bxsrc_q_isset(fds, "noarch");
	only_for_archs = bxsrc_q_to_words(fds, "only_for_archs");
	bpkg_create_allowed_archs(retVal->allowed_archs, only_for_archs);
	/* Get actual pkgname */
	tmp = bxsrc_q_to_words(fds, "pkgname");
	if (!tmp || !(tmp->words) || !(tmp->words[0]) || tmp->words[1]) {
		fprintf(stderr, "Unable to get packagename from template!");
		exit(ERR_CODE_BADPKG);
	}
	retVal->actual_name = tmp->words[0];
	bwords_destroy(&tmp, 0);
	retVal->is_subpkg = (strcmp(retVal->actual_name, name) != 0);
	bxsrc_close(fds, c_pid);

	return retVal;
}

void
bpkg_read_step(const char *xbps_src, struct pkg_importer *info)
{
	if (info->to_send != NULL) {
		bpkg_destroy(info->to_send);
		info->to_send = NULL;
	}
	/* Base case */
	if (info->archnow > ARCH_HOST)
		goto end;

	assert(!(info->is_noarch && (info->archnow != ARCH_NOARCH)));

	/* Create packages */
	info->template->broken = (!(info->is_noarch) &&
					!(info->allowed_archs[info->archnow]));

	if (info->archnow == ARCH_NOARCH && !(info->is_noarch))
		info->to_send = bpkg_init_generic_target(info->template);
	else if (info->archnow == ARCH_HOST && !(info->is_noarch))
		info->to_send = bpkg_init_generic_host(info->template);
	else
		info->to_send = bpkg_init_single(xbps_src, info->template,
				info->is_subpkg ? info->actual_name : "",
				info->archnow, info->is_subpkg);

end:
	if (info->archnext >= ARCH_HOST)
		info->archnow = ARCH_NUM_MAX;
	else if (info->archnow == ARCH_NOARCH && info->is_noarch)
		info->archnow = ARCH_NUM_MAX;
	else
		info->archnow = (info->archnext++);
}

/*
 * bxbps.c
 *
 * Wrapper for libxbps.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <xbps.h>
#include <czmq.h>
#include "dxpb.h"
#include "bstring.h"
#include "bfs.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bxbps.h"

char *
bxbps_get_pkgname(const char *spec, bgraph graph)
{
	char *retVal = NULL;
	void *tmp;

	/* First step, see if the whole thing is a pkgname */
	if ((tmp = zhash_lookup(graph, spec)) != NULL) {
		retVal = strdup(spec);
		if (retVal == NULL)
			exit(ERR_CODE_NOMEM);
		return retVal;
	}

	/* Second option, is it a pkgname-ver_rev ? Important to note that xbps
	 * has strict rules on what a version may be. It must have a number
	 * between the first '-' and the last '_'.
	 * Vaelatern, 2017-07-06 */
	retVal = xbps_pkg_name(spec);
	if (retVal != NULL && (tmp = zhash_lookup(graph, retVal)) != NULL) {
		return retVal;
	}

	if (retVal != NULL)
		free(retVal);

	/* And then, does it use our special matching? */
	retVal = xbps_pkgpattern_name(spec);
	if (retVal != NULL && (tmp = zhash_lookup(graph, retVal)) != NULL) {
		return retVal;
	}

	fprintf(stderr, "The following spec is unacceptable: %s\n", spec);
	if (retVal != NULL)
		free(retVal);
	retVal = NULL;

	return retVal;
}

int
bxbps_spec_match(const char *spec, const char *pkgname, const char *pkgver)
{
	int rc = xbps_pkgpattern_match(pkgname, spec);
	if (rc == -1)
		return ERR_CODE_BAD;
	else if (rc == 1)
		return ERR_CODE_YES;
	char *newname = bstring_add(strdup(pkgname), "-", NULL, NULL);
	newname = bstring_add(newname, pkgver, NULL, NULL);
	rc = xbps_pkgpattern_match(newname, spec);
	free(newname);
	if (rc == -1)
		return ERR_CODE_BAD;
	else if (rc == 1)
		return ERR_CODE_YES;
	return ERR_CODE_NO;
}

/* This doesn't belong in here. But libxbps doesn't provide the functionality
 * and one might think it should. So it's here.
 */
char *
bxbps_pkg_to_filename(const char *pkgname, const char *version, const char *arch)
{
	assert(pkgname);
	assert(version);
	assert(arch);
	char *filename = NULL;
	uint32_t paramA = 0, paramB = 0;
	filename = bstring_add(filename, pkgname,  &paramA, &paramB);
	filename = bstring_add(filename, "-",      &paramA, &paramB);
	filename = bstring_add(filename, version,  &paramA, &paramB);
	filename = bstring_add(filename, ".",      &paramA, &paramB);
	filename = bstring_add(filename, arch,     &paramA, &paramB);
	filename = bstring_add(filename, ".xbps",  &paramA, &paramB);
	assert(filename);
	return filename;
}

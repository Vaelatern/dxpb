/*
 * bpkg.c
 *
 * An in-memory representation and tools for manipulating packages.
 */
#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <czmq.h>
#include "bwords.h"
#include "bxpkg.h"

const char *pkg_archs_str[] = {
	"noarch",
	"aarch64",
	"aarch64-musl",
	"armv6l",
	"armv6l-musl",
	"armv7l",
	"armv7l-musl",
	"i686",
	"i686-musl",
	"x86_64",
	"x86_64-musl",
	"HOST",
	"TARGET",
	NULL
};

/* ARCH_NUM_MAX is returned in event of no match */
enum pkg_archs
bpkg_enum_lookup(const char *str)
{
	enum pkg_archs retVal = 0;
	while (retVal < ARCH_NUM_MAX) {
		if (strcmp(pkg_archs_str[retVal], str) == 0)
			break;
		else
			retVal++;
	}
	return retVal;
}

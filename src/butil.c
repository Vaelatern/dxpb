/*
 * butil.c
 *
 * Simple module for miscellaneous utility functions/wrappers.
 * Currently not even compiled.
 *
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "dxpb.h"
#include "butil.h"

char *
butil_strdup(const char *in)
{
	char *rV = strdup(in);
	if (rV == NULL) {
		perror("Couldn't strdup a string");
		exit(ERR_CODE_NOMEM);
	}
	return rV;
}

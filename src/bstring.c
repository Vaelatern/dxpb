/*
 * bstring.c
 *
 * Simple module for dealing with C strings.
 *
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "dxpb.h"
#include "bstring.h"

char *
bstring_add(char *str, const char* add, uint32_t *str_alloc_size, uint32_t *len)
{
	if (add == NULL)
		return str;
	assert((!!str_alloc_size) == (!!len));
	uint32_t addlen, newlen, alloced, truelen;
	truelen = (len == NULL ? (str == NULL ? 0 : strlen(str)) : *len);
	alloced = (str_alloc_size == NULL ? (str == NULL ? 0 : truelen + 1) : *str_alloc_size);
	addlen = strlen(add);
	newlen = truelen + addlen;
	errno = 0;
	if (newlen >= alloced) {
		alloced = (newlen * 2) + 1;
		if ((str = realloc(str, alloced * sizeof(char))) == NULL) {
			perror("Out of memory for string");
			exit(ERR_CODE_NOMEM);
		}
	}
	if (add)
		strncpy(str + truelen, add, addlen + 1);
	if (len != NULL)
		*len = newlen;
	if (str_alloc_size != NULL)
		*str_alloc_size = alloced;
	return str;
}

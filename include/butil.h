/*
 * bstring.h
 *
 * Module for small utility wrappers
 */

#ifdef DXPB_BUTIL_H
#error "Clean your dependencies"
#endif
#define DXPB_BUTIL_H

#define FREE(x) do { if (x != NULL) free(x); x = NULL; } while (0)

char	*butil_strdup(const char *);

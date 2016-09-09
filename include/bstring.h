/*
 * bstring.h
 *
 * Module for dealing with strings (char *)
 */

#ifdef DXPB_BSTRING_H
#error "Clean your dependencies"
#endif
#define DXPB_BSTRING_H

char	 *bstring_add(char *, const char *, uint32_t *, uint32_t *);

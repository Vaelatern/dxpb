/*
 * bvirtpkg.h
 *
 * Module for dealing with virtpkgs
 */

#ifdef DXPB_BVIRTPKG_H
#error "Clean your dependencies"
#endif
#define DXPB_BVIRTPKG_H

bwords		 bvirtpkg_read(const char *);
zhash_t		*bvirtpkg_from_words(const bwords);

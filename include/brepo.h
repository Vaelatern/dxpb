/*
 * brepo.h
 *
 * Module for working with xbps repositoriy information.
 */

#ifdef DXPB_BREPO_H
#error "Clean your dependencies"
#endif
#define DXPB_BREPO_H

plist_t	*brepo_get_plists(const char **);

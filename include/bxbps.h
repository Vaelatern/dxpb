/*
 * bxbps.h
 *
 * Wrapper for libxbps
 */
#ifdef DXPB_BXBPS_H
#error "Clean your dependencies"
#endif
#define DXPB_BXBPS_H

char	*bxbps_get_pkgname(const char *, bgraph);
int	 bxbps_spec_match(const char *, const char *, const char *);
char	*bxbps_pkg_to_filename(const char *, const char *, const char *);

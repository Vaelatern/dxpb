/*
 * bpkg.h
 *
 * Entry points for interating with xbps-src pkg readers in the import stage
 * of dxpb
 */

#ifdef DXPB_BPKG_H
#error "Clean your dependencies"
#endif
#define DXPB_BPKG_H

struct pkg_virt_import {
	char *name;
	char *ver;
	enum pkg_archs arch;
	char *depname;
	enum pkg_archs deparch;
	uint8_t to_use : 1;
};

struct pkg_importer {
	struct pkg_virt_import	toread[2];
	struct pkg	*template;
	struct pkg	*to_send;
	enum pkg_archs	 archnow;
	enum pkg_archs	 archnext;
	enum pkg_archs	 arch32bit;
	char	allowed_archs[ARCH_NUM_MAX];
	const	char *actual_name;
	char	is_subpkg : 1;
	char	is_noarch : 1;
	char	need_dbg : 1;
};

int	bpkg_is_virtual(struct pkg *);
void	bpkg_read_step(const char *, struct pkg_importer *);
void	bpkg_destroy(struct pkg *);
struct pkg	*bpkg_init_basic();
struct pkg	*bpkg_read(pkgimport_msg_t *);
struct pkg	*bpkg_virt_read(pkgimport_msg_t *);
struct pkg_importer	*bpkg_read_begin(const char *, const char *);
struct pkg	*bpkg_init(const char *, const char *, const char *);
struct pkg	*bpkg_init_oneoff(const char *, const char *, enum pkg_archs, int, int, int);

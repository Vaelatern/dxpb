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

struct pkg_importer {
	struct pkg	*template;
	struct pkg	*to_send;
	enum pkg_archs	 archnow;
	enum pkg_archs	 archnext;
	char	allowed_archs[ARCH_NUM_MAX];
	const	char *actual_name;
	char	is_subpkg : 1;
	char	is_noarch : 1;
};

void	bpkg_read_step(const char *, struct pkg_importer *);
void	bpkg_destroy(struct pkg *);
struct pkg	*bpkg_init_basic();
struct pkg	*bpkg_read(pkgimport_msg_t *);
struct pkg_importer	*bpkg_read_begin(const char *, const char *);
struct pkg	*bpkg_init(const char *, const char *, const char *);
struct pkg	*bpkg_init_oneoff(const char *, const char *, enum pkg_archs, int, int, int);

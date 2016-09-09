/*
 * bxpkg.h
 *
 * Definitions of packages in memory
 */

#ifdef DXPB_BXPKG_H
#error "Clean your dependencies"
#endif
#define DXPB_BXPKG_H

enum pkg_archs {
	ARCH_NOARCH,
	ARCH_AARCH64,
	ARCH_ARMV6L,
	ARCH_ARMV7L,
	ARCH_I686,
	ARCH_X86_64,
	ARCH_AARCH64_MUSL,
	ARCH_ARMV6L_MUSL,
	ARCH_ARMV7L_MUSL,
	ARCH_I686_MUSL,
	ARCH_X86_64_MUSL,
	ARCH_HOST,
	ARCH_TARGET,
	ARCH_NUM_MAX
};

extern char *pkg_archs_str[];

struct arch {
	enum	pkg_archs spec;
	struct	pkg *pkgs;
};

struct pkg {
	char	*name;
	enum	pkg_archs arch;
	char	*ver;
	void	*needs;
	void	*cross_needs;
	void	*needs_me;
	bwords	wneeds_cross_host;
	bwords	wneeds_cross_target;
	bwords	wneeds_native_host;
	bwords	wneeds_native_target;
	unsigned	long priority;
	enum {
				PKG_STATUS_NONE,
				PKG_STATUS_ASKING,
				PKG_STATUS_TOBUILD,
				PKG_STATUS_BUILDING,
				PKG_STATUS_IN_REPO,
			} status;
	unsigned	int can_cross : 1;
	unsigned	int broken : 1;
	unsigned	int resolved : 1;
	unsigned	int bad : 1;
	unsigned	int bootstrap : 1;
	unsigned	int restricted : 1;
};

struct pkg_need {
	const char 	*spec;
	struct	pkg	*pkg;
};

enum pkg_archs	 bpkg_enum_lookup(const char *);

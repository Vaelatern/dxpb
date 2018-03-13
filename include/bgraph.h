/*
 * bgraph.h
 *
 * Graph utilities.
 */

#ifdef DXPB_BGRAPH_H
#error "Clean your dependencies"
#endif
#define DXPB_BGRAPH_H

typedef zhash_t * bgraph;

struct bgraph_worker {
	unsigned int addr;
	unsigned int cost;
	enum	pkg_archs hostarch;
	enum	pkg_archs targetarch;
	char 	iscross;
};

bgraph		 bgraph_new(void);
void		 bgraph_destroy(bgraph *);
int		 bgraph_insert_pkg(bgraph, struct pkg *);
int		 bgraph_attempt_resolution(bgraph);
zlist_t		*bgraph_what_next_for_arch(bgraph, enum pkg_archs);
int		 bgraph_mark_pkg_absent(bgraph, char *, char *, enum pkg_archs);
int		 bgraph_mark_pkg_present(bgraph, char *, char *, enum pkg_archs);
int		 bgraph_mark_pkg_bad(bgraph, char *, char *, enum pkg_archs);
int		 bgraph_mark_pkg_in_progress(bgraph, char *, char *, enum pkg_archs);
int		 bgraph_mark_pkg_not_in_progress(bgraph, char *, char *, enum pkg_archs);
struct pkg 	*bgraph_get_pkg(const bgraph, const char *, const char *, enum pkg_archs);
void		 bgraph_set_vpkgs(bgraph, zhash_t *);

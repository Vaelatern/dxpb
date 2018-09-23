/*
 * bworkermatch.h
 *
 * The fusion of the package graph and the worker module.
 */

#ifdef DXPB_BWORKERMATCH_H
#error "Clean your dependencies"
#endif
#define DXPB_BWORKERMATCH_H

struct bworkermatch {
	struct pkg	 *pkg;
	struct bworker	**workers;
	uint16_t		  num;
	uint16_t		  alloced;
};

int			 bworkermatch_pkg(struct bworker *, bgraph, bgraph, struct pkg *);
struct bworkermatch	*bworkermatch_grp_pkg(struct bworkgroup *, bgraph, struct pkg *);
void			 bworkermatch_destroy(struct bworkermatch **);
struct bworker		*bworkermatch_group_choose(struct bworkermatch *);

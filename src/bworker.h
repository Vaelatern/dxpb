/*
 * bworker.h
 *
 * Just enough information to specify everything about an end worker,
 * from the server's point of view.
 *
 * Important to note: bworkers have addresses. Those addresses do not cross
 * agent lines, since each agent that deals with workers has another address
 * translation. This allows array indexes to not be synchronized across agents,
 * making addresses not special. Workers can just set their addr fields to 0
 * and everybody can be happy. Workers can also set their addr fields to
 * reference different workers at the same agent, This allows for, in the
 * future, special zproto relays separating the server and workers.
 *
 * If a worker is likely to be destroyed, it MUST have a randomly generated
 * "check" value. 32 bits should be more than adequate. This will help with
 * "upstream" agents assigning work to workers that may have already been
 * destroyed. To avoid this race, the check value acts as a safeguard.
 * No checking to confirm the check values of the old addr and new addr is
 * required, nor should it be necessary.
 */
#ifdef DXPB_BWORKER_H
#error "Clean your dependencies"
#endif
#define DXPB_BWORKER_H

struct bworker {
	/* Array index will be what upstream, aka further *
	 * from the worker, thinks the addr is.           */
	uint16_t	myaddr;
	/* What downstream says the addr is.               *
	 * Almost definitely not what we think the addr is */
	uint16_t	addr;
	/* Check value to be generated on insert and transmitted *
	 * with the addr. Only way to prevent race conditions    */
	uint32_t	check;
	uint32_t	mycheck;
	/* Properties of the worker */
	enum pkg_archs	arch;
	enum pkg_archs	hostarch;
	uint8_t		iscross;
	uint16_t	cost;
	/* Properties of the worker's job */
	struct {
		char		*name;
		char		*ver;
		enum pkg_archs	 arch;
	} job;
};

struct bworkgroup {
	struct bworker		**workers;
	struct bworksubgroup	**owners;
	uint16_t		 num_workers;
	uint16_t		 num_workers_alloced;
	uint8_t			 direct_use : 1;
	struct bworksubgroup	**subs; // in case we need to ditch our main group without the subs ever being used.
	uint16_t		 num_subs; // See how far I'll go for you, my dearest RAM?
	uint16_t		 num_subs_alloced; // Don't worry! I'll free you!
};

struct bworksubgroup {
	void	*owner; // If found, return to...
	struct	 bworkgroup *grp;
	zlist_t	*addrs;
};

struct bworkgroup	*bworker_group_new(void);
struct bworksubgroup	*bworker_subgroup_new(struct bworkgroup *);
void			 bworker_group_destroy(struct bworkgroup **);
uint16_t		 bworker_group_insert(struct bworkgroup *, uint16_t,
				uint32_t, enum pkg_archs, enum pkg_archs,
				uint8_t, uint16_t);
int			 bworker_job_assign(struct bworker *, const char *,
				const char *, enum pkg_archs);
void			 bworker_job_remove(struct bworker *);
int			 bworker_job_equal(struct bworker *, char *, char *,
				enum pkg_archs);
uint16_t		 bworker_subgroup_insert(struct bworksubgroup *, uint16_t, uint32_t, enum pkg_archs, enum pkg_archs, uint8_t, uint16_t);
void			 bworker_subgroup_destroy(struct bworksubgroup **);
struct bworker		*bworker_from_my_addr(struct bworkgroup *, uint16_t, uint32_t);
struct bworker		*bworker_from_remote_addr(struct bworkgroup *, uint16_t, uint32_t);
void			 bworker_group_remove(struct bworker *);
void			*bworker_owner_from_my_addr(struct bworkgroup *, uint16_t, uint32_t);
struct bworker		*bworker_from_sub_remote_addr(struct bworksubgroup *, uint16_t, uint32_t);
void			 bworker_subgroup_destroy_interactive(struct bworksubgroup **, void (*)(struct bworker *, void *), void *);

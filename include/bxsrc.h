/*
 * bxsrc.h
 *
 * Module for dealing with bxsrc instances, which are integer file descriptors
 * desribing one end of a set of pipes, the other side connected to a call to
 * xbps-src, used for reading calls.
 */

#ifdef DXPB_BXSRC_H
#error "Clean your dependencies"
#endif
#define DXPB_BXSRC_H

pid_t	 bxsrc_init_build(const char *, const char *, int *, const char *, const char *, enum pkg_archs);
pid_t	 bxsrc_init_read(const char *, const char *, int *, enum pkg_archs, enum pkg_archs);
void		 bxsrc_ask(int, char *);
void		 bxsrc_close(int[], pid_t);
int		 bxsrc_build_end(const int[], const pid_t);
int		 bxsrc_does_exist(const char *, const char *);
char		 bxsrc_q_isset(int *, char *);
bwords		 bxsrc_q_to_words(int *, char *);
char		 bxsrc_did_err(int *);
bwords		 bxsrc_query_pkgnames(const char *, const char *);
zchunk_t 	*bxsrc_get_log(int[], uint32_t, int *);
char		*bxsrc_pkg_version(const char *, const char *);
int		 bxsrc_run_bootstrap(const char *, const char *, const char *, int);

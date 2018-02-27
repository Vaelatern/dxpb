/*
 * bgit.h
 *
 * Module for working with a git repo.
 */
#ifdef DXPB_BGIT_H
#error "Clean your dependencies"
#endif
#define DXPB_BGIT_H

void	 bgit_proc_changed_pkgs(const char *, const char *, int (*)(void*, char*), void *);
void	 bgit_just_ff(const char *, const char *);
char	*bgit_get_head_hash(const char *, const char *);
int	 bgit_checkout_hash(const char *, const char *);

/*
 * blog.h
 *
 * Module for writing log lines
 */

#ifdef DXPB_BLOG_H
#error "Clean your dependencies"
#endif
#define DXPB_BLOG_H

char	 blog_logging_on(const char);
char	*blog_logfile(const char *);

void	 blog_graphSaved(const char *);
void	 blog_pkgImportedForDeletion(const char *);
void	 blog_graphRead(const char *);

#ifdef __ZLIST_H_INCLUDED__
void	 blog_queueSelected(zlist_t *);
#endif

#ifdef DXPB_BXPKG_H
void	 blog_pkgImported(const char *, const char *, const enum pkg_archs);
void	 blog_pkgAddedToGraph(const char *, const char *, const enum pkg_archs);
void	 blog_logFiled(const char *, const char *, const enum pkg_archs);
void	 blog_pkgFetchStarting(const char *, const char *, const enum pkg_archs);
void	 blog_pkgFetchComplete(const char *, const char *, const enum pkg_archs);
void	 blog_pkgMarkedUnbuildable(const char *, const char *, const enum pkg_archs);

#ifdef DXPB_BWORKER_H
void	 blog_workerAddedToGraphGroup(const struct bworker *);
void	 blog_workerMadeAvailable(const struct bworker *);
void	 blog_workerAssigned(const struct bworker *, const char *, const char *, const enum pkg_archs);
void	 blog_workerAssigning(const struct bworker *, const char *, const char *, const enum pkg_archs);
void	 blog_logReceived(const struct bworker *, const char *, const char *, const enum pkg_archs);
void	 blog_workerAssignmentDone(const struct bworker *, const char *, const char *, const enum pkg_archs, const uint8_t);
#endif
#endif

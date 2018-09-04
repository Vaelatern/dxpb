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
void	 blog_pkgImported(char *, char *, enum pkg_archs);
void	 blog_pkgAddedToGraph(char *, char *, enum pkg_archs);
void	 blog_graphSaved(char *);
void	 blog_pkgImportedForDeletion(char *);
void	 blog_graphRead(char *);
void	 blog_queueSelected(zlist_t *);
void	 blog_workerAddedToGraphGroup(struct bworker *);
void	 blog_workerMadeAvailable(struct bworker *);
void	 blog_workerAssigned(struct bworker *, char *, char *, enum pkg_archs);
void	 blog_workerAssigning(struct bworker *, char *, char *, enum pkg_archs);
void	 blog_logReceived(struct bworker *, char *, char *, enum pkg_archs);
void	 blog_logFiled(char *, char *, enum pkg_archs);
void	 blog_workerAssignmentDone(struct bworker *, char *, char *, enum pkg_archs, uint8_t);
void	 blog_pkgFetchStarting(char *, char *, enum pkg_archs);
void	 blog_pkgFetchComplete(char *, char *, enum pkg_archs);

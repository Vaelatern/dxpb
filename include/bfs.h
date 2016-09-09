/*
 * bfs.c
 *
 * Just a function for dealing with the filesystem directly.
 */

void	 bfs_srcpkgs_to_cb(int (*)(void *, char *), void *);
int	 bfs_file_exists(const char *);
int	 bfs_find_file_in_subdir(const char *, const char *, char **);
char	*bfs_new_tmpsock(char *, char *);
uint64_t bfs_size(int);
int	 bfs_rename(const char *, const char *);
void	 bfs_touch(const char *);

/*
 * bfs.c
 *
 * Just a function for dealing with the filesystem directly.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "dxpb.h"
#include "bstring.h"

struct sockaddr_un {
	unsigned short sun_family;  /* AF_UNIX */
	char sun_path[108]; /* Max length allowed by bind()? */
};

/*
 * To scrape
 * All packages are of type dir or symlink or virtual under that directory.
 * Maximum depth of one.  Parsing is done with readdir(). Symlinks are naively
 * assumed to be in the same dir.
 */
void
bfs_srcpkgs_to_cb(const char *repopath, int (*cb)(void *, void *), void *topass)
{
	DIR *dir = NULL;
	struct dirent *dp = NULL;
	struct stat sinfo = {0};
	int fd_top, rc;

	char *srcpkgspath = strdup(repopath);
	srcpkgspath = bstring_add(srcpkgspath, "/", NULL, NULL);
	srcpkgspath = bstring_add(srcpkgspath, "srcpkgs", NULL, NULL);
	errno = 0;
	if ((fd_top = open(srcpkgspath, O_DIRECTORY | O_RDONLY)) == 0) {
		perror("Can not open fd to dir");
		exit(ERR_CODE_BADDIR);
	}

	errno = 0;
	if ((dir = fdopendir(fd_top)) == NULL) {
		perror("Can not open path");
		exit(ERR_CODE_BADDIR);
	}

	while (errno = 0, (dp = readdir(dir)) != NULL) {
		if (errno != 0) {
			perror("Error while reading dir");
			errno = 0;
		}
		if (dp->d_name[0] == '.')
			continue;
		if (fstatat(fd_top, dp->d_name, &sinfo,
					AT_SYMLINK_NOFOLLOW) != 0) {
			perror("Could not stat dir");
			perror(dp->d_name);
			errno = 0;
			continue;
		}
		rc = (*cb)(topass, dp->d_name);
		if (rc != 0) {
			perror("Could not perform callback action for pkg");
			perror(dp->d_name);
			errno = 0;
			continue;
		}
	}

	closedir(dir);
	close(fd_top);
}

/*
 * Check if file at path exists. Only looks for existance.
 */
int
bfs_file_exists(const char *path)
{
	int retVal = 0;
	struct stat statRet;
	retVal = stat(path, &statRet);
	return (retVal == 0);
}

/* Recursive because we expect very few nested subdirs
 * Useful for finding a package in any subrepo under hostdir*/
int
bfs_find_file_in_subdir(const char *rootdir, const char *filename, char **subdir)
{
	assert(rootdir);
	assert(filename);
	int retVal = 0;
	int fd_top;
	DIR *dir = NULL;
	struct dirent *dp = NULL;
	struct stat sinfo = {0};
	char *subroot = NULL;
	uint32_t parA = 0, parB = 0;
	char *curpath = bstring_add(bstring_add(bstring_add(
					NULL, rootdir, &parA, &parB),
				"/", &parA, &parB),
			filename, &parA, &parB);

	if (!bfs_file_exists(curpath)) {
		if ((fd_top = open(rootdir, O_DIRECTORY | O_RDONLY)) == 0) {
			perror("Can not open fd to dir");
			exit(ERR_CODE_BADDIR);
		}
		if ((dir = fdopendir(fd_top)) == NULL) {
			perror("Can not open path");
			exit(ERR_CODE_BADDIR);
		}

		while (errno = 0, (dp = readdir(dir)) != NULL && !retVal) {
			if (errno != 0) {
				perror("Error while reading dir");
				continue;
			}
			if (dp->d_name[0] == '.')
				continue;
			if (fstatat(fd_top, dp->d_name, &sinfo,
						AT_SYMLINK_NOFOLLOW) != 0) {
				perror("Could not stat dir");
				perror(dp->d_name);
				continue;
			}

			if (S_ISDIR(sinfo.st_mode)) {
				subroot = bstring_add(bstring_add(
							strdup(rootdir), "/",
							NULL, NULL),
						dp->d_name, NULL, NULL);
				retVal = bfs_find_file_in_subdir(subroot,
						filename, subdir);
				free(subroot);
				subroot = NULL;
			} else
				continue;

			if (retVal && !subdir)
				break;
			else if (retVal) {
				parA = parB = 0;
				char *tmpdir = bstring_add(bstring_add(bstring_add(
								NULL, "/", &parA, &parB),
							dp->d_name, &parA, &parB),
						*subdir, &parA, &parB);
				free(*subdir);
				*subdir = tmpdir;
				break;
			}
		}
		close(fd_top);
		closedir(dir);
	} else
		if (*subdir)
			retVal = open(curpath, O_RDONLY | O_NONBLOCK);
		else
			retVal = 1;


	free(curpath);
	curpath = NULL;
	return retVal;
}

int
bfs_setup_sock(const char *path)
{
	struct sockaddr_un sockspec;
	int sd;
	int rc;

	sockspec.sun_family = AF_UNIX;
	memset(&(sockspec.sun_path), 0, sizeof(sockspec.sun_path));
	(void)strncpy(sockspec.sun_path, path, sizeof(sockspec.sun_path)-1);
	sockspec.sun_path[sizeof(sockspec.sun_path)-1] = '\0';

	/* Create socket */
	sd = socket(sockspec.sun_family, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("Couldn't create socket");
		return ERR_CODE_BAD;
	}
	if (access(sockspec.sun_path, 0) == 0) {
		rc = unlink(sockspec.sun_path);
		if (rc != 0) {
			perror("Couldn't unlink temporary socket");
			exit(ERR_CODE_BAD);
		}
	}

	/* bind, so as to create the socket */
	rc = bind(sd, (struct sockaddr *)&sockspec, sizeof(sockspec));
	if (rc == -1) {
		perror("Couldn't bind to socket");
		return ERR_CODE_BAD;
	}

	rc = close(sd);
	if (rc != 0) {
		perror("Can't close the socket after naming it");
		exit(ERR_CODE_BAD);
	}
	return ERR_CODE_OK;
}

char *
bfs_new_tmpsock(const char *dirpattern, const char *sockname)
{
	int rc;
	struct sockaddr_un sockspec;
	int tmpdir = 0;
	char *dirpath = NULL;
	char *dirpat = NULL;
	char *retVal = NULL;

	assert(strlen(dirpattern) + strlen(sockname) + 1 <= sizeof(sockspec.sun_path));
	/* +1 for extra '/' between parts */

	if ((dirpat = strdup(dirpattern)) == NULL) {
		perror("Can't allocate string");
		return NULL;
	}

	dirpath = mkdtemp(dirpat);
	if (dirpath == NULL) { /* dirpat is less of a pattern than a path */
		tmpdir = 0;
		dirpath = dirpat;
		mkdir(dirpath, 0700);
	} else {
		tmpdir = 1;
	}

	retVal = bstring_add(bstring_add(dirpat, "/", NULL, NULL),
					sockname, NULL, NULL);

	rc = bfs_setup_sock(retVal);
	return retVal;
}

enum ret_codes
bfs_delete(char *oldpath)
{
	assert(oldpath);

	enum ret_codes retVal;
	char *destpath = bstring_add(oldpath, ".del", NULL, NULL);
	assert(destpath);

	int rc = rename(oldpath, destpath);

	if (rc == 0)
		retVal = ERR_CODE_OK;
	else
		switch(errno) {
		case ENOENT:
			retVal = ERR_CODE_BADCALLER;
			break;
		default:
			retVal = ERR_CODE_BADCALLER;
			break;
		}

	return retVal;
}

void
bfs_touch(const char *path)
{
	uint16_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	int fd = open(path, O_WRONLY | O_CREAT, mode);
	if (fd == -1 || close(fd)) {
		perror("Couldn't touch file");
	}
}

uint64_t
bfs_size(int fd)
{
	struct stat sinfo = {0};
	int rc = fstat(fd, &sinfo);
	if (sinfo.st_mode != S_IFREG) {
		fprintf(stderr, "Wanted file is not regular\n");
		return 0;
	}
	if (rc == -1 || sinfo.st_size < 0)
		return 0;
	else
		return sinfo.st_size;

}

enum ret_codes
bfs_set_shared_sock_perms(const char *sockpath)
{
	mode_t mode =   S_IRUSR | S_IWUSR | S_IXUSR |
			S_IRGRP | S_IWGRP | S_IXGRP |
			S_IROTH | S_IWOTH | S_IXOTH;
	int rc = chmod(sockpath, mode);
	if (rc != 0) {
		perror("Tried to set the socket to be rwxrwx---");
		return ERR_CODE_BAD;
	}
	return ERR_CODE_OK;
}

// This is short. It is only here so stdio need not be included everywhere.
int
bfs_rename(const char *a, const char *b)
{
	return rename(a, b);
}

enum ret_codes
bfs_ensure_sock_perms(const char *arg)
{
	if (strstr(arg, "ipc://") != arg)
		return ERR_CODE_OK;
	arg = arg + strlen("ipc://");
	enum ret_codes rc = bfs_set_shared_sock_perms(arg);
	return rc;
}

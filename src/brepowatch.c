/*
 * brepowatch.c
 *
 * Agent and helper for keeping track of the binary package repository.
 * Does not have handling for a directory being deleted.
 */

#define _POSIX_C_SOURCE 200809L

#include <sys/inotify.h>
#include <czmq.h>
#include "dxpb.h"
#include "bstring.h"

/* Recursive because we don't expect very deep directories
 */
void
brepowatch_dir_to_hashes(const char *dir, zhash_t *retVal, int inotify_fd, const char *curdir)
{
	assert(dir);
	assert(retVal);
	assert(curdir);
	DIR *dir_s = NULL;
	int fd_dir;
	struct dirent *dp = NULL;
	struct stat sinfo = {0};

	char *fulldir = bstring_add(strdup(dir), curdir, NULL, NULL);
	int rc = inotify_add_watch(inotify_fd, fulldir, IN_CREATE | IN_DELETE);
	if (rc < 0) {
		perror("Failed to add inotify watch");
		fprintf(stderr, "Relevant dir: %s\n", fulldir);
	}

	if ((fd_dir = open(fulldir, O_DIRECTORY | O_RDONLY)) == 0) {
		perror("Failed to open target watch dir");
		exit(ERR_CODE_BADDIR);
	}
	if ((dir_s = fdopendir(fd_dir)) == NULL) {
		perror("Can not open target watch dir");
		exit(ERR_CODE_BADDIR);
	}
	while (errno = 0, (dp = readdir(dir_s)) != NULL) {
		if (errno != 0) {
			perror("Error while reading target dir");
			continue;
		}
		if (dp->d_name[0] == '.')
			continue;

		if (fstatat(fd_dir, dp->d_name, &sinfo, AT_SYMLINK_NOFOLLOW) != 0) {
			perror("Could not stat dir");
			perror(dp->d_name);
			continue;
		}

		if (S_ISDIR(sinfo.st_mode)) {
			char *nextdir = NULL;
			if (curdir[1] == '\0') // else we get '//name' for dir 'name'
				nextdir = bstring_add(strdup("/"), dp->d_name, NULL, NULL);
			else
				nextdir = bstring_add(bstring_add(strdup(curdir),
							"/", NULL, NULL), dp->d_name, NULL, NULL);
			assert(nextdir);
			brepowatch_dir_to_hashes(dir, retVal, inotify_fd, nextdir);
			free(nextdir);
		} else { /* Is not dir */
			char *tmp = strdup(curdir);
			assert(tmp);
			zhash_insert(retVal, dp->d_name, tmp);
			zhash_freefn(retVal, dp->d_name, free);
		}
	}

	free(fulldir);
	fulldir = NULL;
	closedir(dir_s);
	close(fd_dir);
}

zhash_t *
brepowatch_archive_dir(const char *dir, int fd)
{
	assert(dir);
	assert(fd >= 0);
	zhash_t *rV = zhash_new();
	assert(rV);
	brepowatch_dir_to_hashes(dir, rV, fd, "/");
	return rV;
}

void *
brepowatch_filefinder_actor(void *endpoint_in)
{
	assert(endpoint_in);
	const char *endpoint = endpoint_in;
	char *instr = NULL, *outstr = NULL;
	zsock_t *mine = zsock_new(ZMQ_PAIR);
	assert(mine);
	int rc1 = zsock_connect(mine, "inproc://%s", endpoint);
	assert(rc1 != -1);

	int inotify_fd = inotify_init1(IN_NONBLOCK);

	char *rootdir = zstr_recv(mine);
	assert(rootdir);
	zstr_send(mine, "OK");

	zhash_t *files = brepowatch_archive_dir(rootdir, inotify_fd);
	assert(files);

	// Need to have notify first in this argument list, to give it priority.
	zpoller_t *poller = zpoller_new(&inotify_fd, mine, NULL);
	assert(poller);

	void *readIn = NULL;

	while ((readIn = zpoller_wait(poller, -1)) != NULL) {
		if (readIn == &inotify_fd) {
			char *buf[80]; // Discard result
			while(read(inotify_fd, buf, 79) != -1) {
				;
			}
			assert(files);
			zhash_destroy(&files);
			files = brepowatch_archive_dir(rootdir, inotify_fd);
			assert(files);
		} else if (readIn == mine) {
			instr = zstr_recv(mine);
			if (strlen(instr) == 0) {
				free(instr);
				break;
			}
			assert(files);
			outstr = zhash_lookup(files, instr);
			// outstr may be NULL at this point. This is okay.
			zstr_send(mine, outstr);
			free(instr);
		} else {
			perror("Nothing read, yet zpoller quit - exiting");
			break;
		}
	}

	close(inotify_fd);

	free(rootdir);
	rootdir = NULL;
	zsock_destroy(&mine);
	assert(files);
	zhash_destroy(&files);
	zpoller_destroy(&poller);
	return NULL;
}

pthread_t *
brepowatch_filefinder_prepare(zsock_t **sock, const char *rootdir, const char *inprocpoint)
{
	char *endpoint = strdup(inprocpoint);
	assert(endpoint);
	pthread_t *thread = calloc(1, sizeof(pthread_t));
	*sock = zsock_new(ZMQ_PAIR);
	int rc1 = zsock_bind(*sock, "inproc://%s", endpoint);
	assert(rc1 != -1);

	pthread_create(thread, NULL, brepowatch_filefinder_actor, endpoint);

	zstr_send(*sock, rootdir);
	char *back = zstr_recv(*sock);
	assert(strcmp(back, "OK") == 0);
	free(endpoint);
	endpoint = NULL;
	free(back);
	back = NULL;
	return thread;
}

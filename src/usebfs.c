#define _POSIX_C_SOURCE 200809L
// The above line is part of the tests used. Please do not alter.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "../include/dxpb.h"
#include "../include/bfs.h"

int main(void)
{
	char *retVal = bfs_new_tmpsock("/tmp/dxpb-XXXXXX", "default");
	if (!retVal) {
		printf("Nope, didn't work\n");
	}
	free(retVal);
	retVal = NULL;
	assert(bfs_find_file_in_subdir("./", "usebfs.c", NULL));
	assert(bfs_find_file_in_subdir("../../../", "usebfs.c", NULL));
	assert(!bfs_find_file_in_subdir("./", "doesnotexist.c", NULL));
	assert(!bfs_find_file_in_subdir("../../../", "doesnotexist.c", NULL));

	char *end = NULL;
	char goal[32];
	int fd = bfs_find_file_in_subdir("./", "usebfs.c", &end);
	assert(end == NULL);
	free(end);
	end = NULL;
	ssize_t rdin = read(fd, goal, 32);
	assert(rdin == 32);
	assert(strcmp("#define _POSIX_C_SOURCE 200809L\n", goal) == 0);
	close(fd);

	fd = bfs_find_file_in_subdir("../", "usebfs.c", &end);
	assert(strcmp(end, "/src") == 0);
	free(end);
	end = NULL;
	assert(fd > 2);
	rdin = read(fd, goal, 32);
	assert(rdin == 32);
	assert(strcmp("#define _POSIX_C_SOURCE 200809L\n", goal) == 0);
	close(fd);
}

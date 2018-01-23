#define _POSIX_C_SOURCE 200809L

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
}

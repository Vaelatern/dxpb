#include "../include/bfs.h"
#include <stdio.h>

int main(void)
{
	char *retVal = bfs_new_tmpsock("/tmp/dxpb-XXXXXX", "default");
	if (!retVal) {
		printf("Nope, didn't work\n");
	}
}

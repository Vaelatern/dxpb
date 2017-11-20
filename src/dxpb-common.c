/*
 * dxpb-common.c
 *
 * Used to provide common functions to the dxpb-* family of user-facing
 * binaries
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <czmq.h>
#include "dxpb.h"
#include "bfs.h"
#include "dxpb-common.h"

#define VERSION "0.0.1"

void
prologue(const char *argv0)
{
	char *arg = strdup(argv0);
	assert(arg);
	printf("DXPB: Distributed XBPS Package Builder\n"
			"%s %s\n"
			"Copyright 2016-2017, Toyam Cox\n"
			"This is free software with ABSOLUTELY NO WARRANTY.\n"
			"For license details, pass the -L flag\n"
			"Compiled: %s %s\n\n",
			basename(arg), VERSION, __DATE__, __TIME__);
	free(arg);
}

void
print_license(void)
{
#include "license.inc"
	printf("%.*s", ___COPYING_len, ___COPYING);
}

void
flushsock(void *sock, const char *prefix)
{
	char *tmp = zstr_recv(sock);
	fprintf(stderr, "%s: %s\n", prefix, tmp);
	free(tmp);
	zsock_flush(sock);
}

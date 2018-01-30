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
#include "dxpb-version.h"
#include "dxpb-client.h"

void
prologue(const char *argv0)
{
	char *arg = strdup(argv0);
	assert(arg);
	fprintf(stderr, "DXPB: Distributed XBPS Package Builder\n"
			"%s %s\n"
			"Copyright 2016-2018, Toyam Cox\n"
			"This is free software with ABSOLUTELY NO WARRANTY.\n"
			"For license details, pass the -L flag\n"
			"Compiled: %s %s\n\n",
			basename(arg), DXPB_VERSION, __DATE__, __TIME__);
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

enum ret_codes
ensure_sock_if_ipc(const char *arg)
{
       if (strstr(arg, "ipc://") != arg)
               return ERR_CODE_OK;
       enum ret_codes rc = bfs_setup_sock(arg + strlen("ipc://"));
       if (rc != ERR_CODE_OK)
               return rc;
       rc = bfs_ensure_sock_perms(arg);
       return rc;
}

/*
 * dxpb-client.c
 *
 * Used to provide common functions to the dxpb-* family of user-facing
 * binaries, when they act as clients.
 * Mostly exists just for pkgall_client_ssl_setcurve().
 */

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <czmq.h>
#include "dxpb.h"
#include "bfs.h"
#include "bstring.h"

// Duplicated from pkgall_client_ssl.xml
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

int
setup_ssl(void *agent, const char *argv0, const char *servername, const char *ssldir)
{
	int rV = 0;
	zcert_t *server, *mine;
	char *ssldir_actual = bstring_add(strdup(ssldir), "/", NULL, NULL);
	char *servercert = bstring_add(strdup(ssldir_actual), servername, NULL, NULL);
	char *mycert = bstring_add(bstring_add(ssldir_actual, argv0, NULL, NULL),
			"_secret", NULL, NULL);
	ssldir_actual = NULL; // we just reassigned it two lines ago

	if (bfs_file_exists(servercert))
		server = zcert_load(servercert);
	else {
		fprintf(stderr, "*****************************************\n"
				"*****************************************\n"
				"*****************************************\n"
				"*****> !!CURVE encryption disabled!! <***\n"
				"*****> !!Setup not appropiate for !! <***\n"
				"*****> !!    any public networks  !! <***\n"
				"*****************************************\n"
				"*****************************************\n"
				"*****************************************\n");
		rV = -1;
		goto abort;
	}

	if (bfs_file_exists(mycert))
		mine = zcert_load(mycert);
	else
		mine = zcert_new();

	pkgall_client_ssl_setcurve(agent, zcert_secret_txt(mine),
			zcert_public_txt(mine), zcert_public_txt(server));

abort:
	zcert_destroy(&server);
	zcert_destroy(&mine);
	FREE(servercert);
	FREE(mycert);
	return rV;
}


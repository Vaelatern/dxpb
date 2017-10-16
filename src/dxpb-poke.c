/*
 * dxpb-poke.c
 *
 * Notification one-shot to tell pkgimport-master to check git
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include "dxpb.h"
#include "pkgimport_poke.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-poke-help.inc"
	printf("%.*s\n", ___doc_dxpb_poke_help_txt_len, ___doc_dxpb_poke_help_txt);
}

int
run(int flags, const char *endpoint)
{
	assert((flags & ERR_FLAG) == 0);
	pkgimport_poke_t *client;
	int rc;

	client = pkgimport_poke_new();
	assert(client);

	if (flags & VERBOSE_FLAG)
		zstr_sendx(client, "VERBOSE", NULL);

	rc = pkgimport_poke_connect_now(client, endpoint);
	if (rc != 0) {
		perror("dxpb_pkgimport_agent: could not connect");
		return ERR_CODE_ZMQ;
	}

	zpoller_t *polling = zpoller_new(pkgimport_poke_actor(client));
	assert(polling);
	zactor_t *pollret;
	char *str;
	while ((pollret = zpoller_wait(polling, -1)) != NULL) {
		str = zstr_recv(pollret);
		if (strcmp(str, "KTHNKSBYE") == 0) {
			zstr_free(&str);
			break;
		}
	}

	pkgimport_poke_destroy(&client);
	return ERR_CODE_OK;
}

int
main(int argc, char * const *argv)
{
	int c;
	int flags = 0;
	const char *optstring = "hLvi:";
	char *default_endpoint = DEFAULT_IMPORT_ENDPOINT;
	char *endpoint = NULL;


	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'i':
			endpoint = optarg;
			break;
		case 'h':
			help();
			return 0;
		case 'L':
			print_license();
			return 0;
		case 'v':
			flags |= VERBOSE_FLAG;
			break;
		case '?':
			flags |= ERR_FLAG;
			break;
		case ':':
			flags |= ERR_FLAG;
			break;
		}
	}

	if (flags & ERR_FLAG) {
		fprintf(stderr, "Exiting due to errors.\n");
		help();
		exit(ERR_CODE_BADWORLD);
	}

	prologue(argv[0]);

	if (!endpoint)
		endpoint = default_endpoint;

	return run(flags, endpoint);
}

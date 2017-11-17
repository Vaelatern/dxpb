/*
 * dxpb-certs-server.c
 *
 * One-shot program to run at dxpb master setup or key rotation
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <czmq.h>
#include "dxpb.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1<<0
#define ERR_FLAG 1<<1

void
help(void)
{
#include "dxpb-certs-remote-help.inc"
	printf("%.*s\n", ___doc_dxpb_certs_remote_help_txt_len, ___doc_dxpb_certs_remote_help_txt);
}

int
makecert(int verbose, const char *name)
{
	zcert_t *cert = zcert_new();
	char *tmp;
	zcert_set_meta(cert, "created-by", "DXPB Remote-Worker Tooling");
	zcert_set_meta(cert, "created-for-user", "%s", getlogin());
	zcert_set_meta(cert, "created-for", "%s", name);
	zcert_set_meta(cert, "date-created", "%s", (tmp = zclock_timestr()));
	assert(tmp);
	free(tmp);
	tmp = NULL;
	int rc = zcert_save(cert, name);
	if (rc != 0) {
		char buf[80];
		fprintf(stderr, "Couldn't save %s/%s\n"
				"\tPerhaps permissions should be fixed?\n",
				getcwd(buf, 79),
				name);
		return -1;
	}
	if (verbose)
		printf("Created cert for %s\n", name);
	zcert_destroy(&cert);
	return 0;
}

int
run(uint8_t flags, const char *name, const char *ssldir)
{
	assert(name);
	assert(ssldir);

	int verbose = flags & VERBOSE_FLAG;
	int rc = chdir(ssldir);
	if (rc != 0) {
		fprintf(stderr, "Couldn't chdir to %s\n"
				"\tMaybe it doesn't exist?\n",
				ssldir);
		return -1;
	}

	int retVal = 0;

	rc = makecert(verbose, name);
	if (rc)
		retVal = rc;

	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	uint8_t flags = 0;
	const char *optstring = "hLvk:n:";
	char *default_ssldir = DEFAULT_SSLDIR;
	char *ssldir = NULL;
	char *name = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'k':
			ssldir = optarg;
			break;
		case 'n':
			name = optarg;
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

	if (!name) {
		flags &= ERR_FLAG;
		fprintf(stderr, "Need to specify a name!\n");
	}

	if (flags & ERR_FLAG) {
		fprintf(stderr, "Exiting due to errors.\n");
		help();
		exit(ERR_CODE_BADWORLD);
	}

	prologue(argv[0]);

	if (!ssldir)
		ssldir = default_ssldir;

	return run(flags, name, ssldir);
}

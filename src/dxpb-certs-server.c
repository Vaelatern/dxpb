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
#define DOGRAPHER_FLAG 1<<2
#define DOFILER_FLAG 1<<3
#define DOFRONTEND_FLAG 1<<4

void
help(void)
{
#include "dxpb-certs-server-help.inc"
	printf("%.*s\n", ___doc_dxpb_certs_server_help_txt_len, ___doc_dxpb_certs_server_help_txt);
}

int
makecert(int verbose, const char *name)
{
	zcert_t *cert = zcert_new();
	char *tmp;
	zcert_set_meta(cert, "created-by", "DXPB Server Tooling");
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

void
chuser(int verbose, const char *name)
{
	if (verbose)
		printf("Setting user: %s\n", name);
	struct passwd *passwd = getpwnam(name);
	assert(passwd);
	int uid = passwd->pw_uid;

	struct group *grp = getgrnam("_dxpb");
	assert(grp);
	int gid = grp->gr_gid;

	int rc;

	errno = 0;
	rc = setegid(gid);
	if (rc != 0) {
		perror("setegid failed");
		exit(-1);
	}

	errno = 0;
	rc = seteuid(uid);
	if (rc != 0) {
		perror("seteuid failed");
		exit(-1);
	}

	return;
}
int
run(uint8_t flags, const char *ssldir)
{
	int verbose = flags & VERBOSE_FLAG;
	int rc = chdir(ssldir);
	if (rc != 0) {
		fprintf(stderr, "Couldn't chdir to %s\n"
				"\tMaybe it doesn't exist?\n",
				ssldir);
		return -1;
	}

	int retVal = 0;

	char *me = getpwuid(geteuid())->pw_name;
	assert(me);

	if (flags & DOGRAPHER_FLAG) {
		chuser(verbose, "_dxpb_grapher");
		rc = makecert(verbose, "dxpb-grapher");
		chuser(verbose, me);
		if (rc)
			retVal = rc;
	}
	if (flags & DOFRONTEND_FLAG) {
		chuser(verbose, "_dxpb_frontend");
		rc = makecert(verbose, "dxpb-frontend");
		chuser(verbose, me);
		if (rc)
			retVal = rc;
	}
	if (flags & DOFILER_FLAG) {
		chuser(verbose, "_dxpb_filer");
		rc = makecert(verbose, "dxpb-filer");
		chuser(verbose, me);
		if (rc)
			retVal = rc;
	}
	return retVal;
}

int
main(int argc, char * const *argv)
{
	int c;
	uint8_t flags = DOGRAPHER_FLAG | DOFILER_FLAG | DOFRONTEND_FLAG;
	const char *optstring = "hLvRFGk:";
	char *default_ssldir = DEFAULT_SSLDIR;
	char *ssldir = NULL;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'k':
			ssldir = optarg;
			break;
		case 'h':
			help();
			return 0;
		case 'L':
			print_license();
			return 0;
		case 'R':
			flags ^= DOFRONTEND_FLAG;
			break;
		case 'F':
			flags ^= DOFILER_FLAG;
			break;
		case 'G':
			flags ^= DOGRAPHER_FLAG;
			break;
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

	if (!ssldir)
		ssldir = default_ssldir;

	return run(flags, ssldir);
}

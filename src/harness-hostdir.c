/*
 * harness-hostdir.c
 *
 * DXPB repository file transfer - test harness
 */

#define _POSIX_C_SOURCE 200809L

#include <bsd/stdlib.h>
#include <czmq.h>
#include <sys/stat.h>
#include "dxpb.h"
#include "bfs.h"
#include "bstring.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bxbps.h"
#include "pkggraph_msg.h"
#include "pkgfiles_msg.h"
#include "pkggraph_filer.h"
#include "pkgfiler.h"

#include "dxpb-common.h"
#include "dxpb-version.h"

// This is the largest number of 4-byte integers we can expect in a file.
// The lucky package that is this massive is `ceph-dbg`. Since this is 2.5 GB,
// this may not be desireable for tests, but is necessary just to be sure.
// But we have now shrunk the size to something we can put under a 4 GB tmp
// 510000000
// Now shrunk by 2
#define LARGEST_SIZE 51000000

void
forkoff_master(const char *ssldir, const char *sdir, const char *rdir,
		const char *ldir, const char *file_end,
		const char *graph_end, const char *file_pub,
		const char *graph_pub)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-hostdir-master", "dxpb-hostdir-master",
				"-r", rdir, "-l", ldir,
				"-s", sdir, "-g", graph_end, "-G", graph_pub,
				"-f", file_end, "-F", file_pub, "-k", ssldir,
				NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

void
forkoff_remote(const char *ssldir, const char *hostdir, const char *file_end)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-hostdir-remote", "dxpb-hostdir-remote",
				"-H", hostdir, "-f", file_end, "-k", ssldir,
				NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

int
run(const char *ssldir, const char *sdir, const char *rdir, const char *ldir,
		const char *remdir1, const char *remdir2,
		const char *file_end, const char *graph_end,
		const char *file_pub, const char *graph_pub)
{
	SSLDIR_UNUSED(ssldir);
	(void) file_pub;
	(void) graph_pub;
	assert(sdir);
	assert(rdir);
	assert(ldir);
	assert(remdir1);
	assert(remdir2);
	assert(file_end);
	assert(graph_end);
	assert(file_pub);
	assert(graph_pub);
	assert(ssldir);

	char *repopath1 = bstring_add(bstring_add(NULL, remdir1, NULL, NULL),
			"/", NULL, NULL);
	char *repopath2 = bstring_add(bstring_add(NULL, remdir2, NULL, NULL),
			"/", NULL, NULL);
	char *pkgfile1 = bxbps_pkg_to_filename("foo", DXPB_VERSION, "noarch");
	char *pkgfile2 = bxbps_pkg_to_filename("bar", DXPB_VERSION, "x86_64");
	char *pkgfile3 = bxbps_pkg_to_filename("bar", DXPB_VERSION, "x86_64-musl");
	char *pkgfile4 = bxbps_pkg_to_filename("bar", DXPB_VERSION, "aarch64");
	char *pkgpath1 = bstring_add(strdup(repopath1), pkgfile1, NULL, NULL);
	char *pkgpath2 = bstring_add(strdup(repopath2), pkgfile2, NULL, NULL);
	char *pkgpath3 = bstring_add(strdup(repopath1), pkgfile3, NULL, NULL);
	char *pkgpath4 = bstring_add(strdup(repopath2), pkgfile4, NULL, NULL);

	int rc;

	pkgfiles_msg_t *file_msg = pkgfiles_msg_new();

	zsock_t *file  = zsock_new (ZMQ_DEALER);
	assert(file);

	zsock_attach(file, file_end, false);

#define SEND(this, msg, sock)        { \
					pkgfiles_msg_set_id(msg, this); \
					rc = pkgfiles_msg_send(msg, sock); \
					assert(rc == 0); \
				}

#define GET(mymsg, sock)        { \
					zpoller_t *p = zpoller_new(sock, NULL); \
					(void) zpoller_wait(p, 10*1000); \
					assert(!zpoller_expired(p)); \
					if (zpoller_terminated(p)) \
					exit(-1); \
					rc = pkgfiles_msg_recv(mymsg, sock); \
					assert(rc == 0); \
					zpoller_destroy(&p); \
				}

#define DOPING(msg,sock)	{ \
					for (int i = 0; i < 2; i++) { \
						sleep(10); \
						SEND(TOMSG(PING), msg, sock); \
						GET(msg, sock); \
						ASSERTMSG(id, msg, TOMSG(ROGER)); \
					} \
				}

#define DOPINGRACE(msg,sock)	{ \
					for (int i = 0; i < 2; i++) { \
						sleep(10); \
						SEND(TOMSG(PING), msg, sock); \
						GET(msg, sock); \
						ASSERTMSGOR(id, msg, TOMSG(ROGER), TOMSG(PKGHERE)); \
					} \
				}

#define WRITEFILE(path, iterations)	{ \
						PRINTUP; \
						printf("Writing to path: %s\n", path); \
						FILE *fp = fopen(path, "wb"); \
						for (uint64_t i = 0; i < iterations; i++) { \
							fprintf(fp, "%uld", arc4random()); \
						} \
						fclose(fp); \
					}

#define PRINTUP			printf("****************************\n*****> WRITE FILE <*********\n****************************\n");
#define TOMSG(str)                   PKGFILES_MSG_##str
#define SETMSG(what, msg, to)        pkgfiles_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkgfiles_msg_##what(msg) == eq)
#define ASSERTMSGOR(what, msg, eq, eq2)     assert(pkgfiles_msg_##what(msg) == eq || pkgfiles_msg_##what(msg) == eq2)
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkgfiles_msg_##what(msg), eq) == 0)
#define ASSERTMSGSTROR(what, msg, eq, eq2)     assert(strcmp(pkgfiles_msg_##what(msg), eq) == 0 || strcmp(pkgfiles_msg_##what(msg), eq2) == 0)

	/* And now we get to work */

	SEND(TOMSG(HELLO), file_msg, file);
	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));
	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "noarch");
	SEND(TOMSG(ISPKGHERE), file_msg, file);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "noarch");

	DOPING(file_msg, file);

	WRITEFILE(pkgpath1, 1000);
	SETMSG(pkgname, file_msg, "");
	SETMSG(version, file_msg, "");
	SETMSG(arch, file_msg, "");

	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "noarch");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	// test if we send multiple of these messages
	SEND(TOMSG(ISPKGHERE), file_msg, file);

	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "noarch");

	WRITEFILE(pkgpath3, 10000000);
	WRITEFILE(pkgpath2, 10);
	WRITEFILE(pkgpath4, LARGEST_SIZE);
	SEND(TOMSG(HELLO), file_msg, file);
	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));

	SETMSG(pkgname, file_msg, "");
	SETMSG(version, file_msg, "");
	SETMSG(arch, file_msg, "");

	SETMSG(pkgname, file_msg, "bar");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "x86_64");
	SEND(TOMSG(ISPKGHERE), file_msg, file);

	SETMSG(pkgname, file_msg, "");
	SETMSG(version, file_msg, "");
	SETMSG(arch, file_msg, "");

	SETMSG(pkgname, file_msg, "bar");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "x86_64-musl");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(pkgname, file_msg, "");
	SETMSG(version, file_msg, "");
	SETMSG(arch, file_msg, "");

	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));
	ASSERTMSGSTR(pkgname, file_msg, "bar");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTROR(arch, file_msg, "x86_64", "x86_64-musl");

	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));
	ASSERTMSGSTR(pkgname, file_msg, "bar");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTROR(arch, file_msg, "x86_64-musl", "x86_64");

	SETMSG(pkgname, file_msg, "bar");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "aarch64");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(pkgname, file_msg, "");
	SETMSG(version, file_msg, "");
	SETMSG(arch, file_msg, "");

	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);
	DOPINGRACE(file_msg, file);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));
	ASSERTMSGSTR(pkgname, file_msg, "bar");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "aarch64");

	/* Work over, let's clean up. */

	pkgfiles_msg_destroy(&file_msg);

	zsock_destroy(&file);

	return 0;
}

int
main(int argc, char * const *argv)
{
	(void) argc;
	char *tmppath = strdup("/tmp/dxpb-harness-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	assert(ourdir);
	char *logpath = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/logs/", NULL, NULL);
	char *stagepath = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/staging/", NULL, NULL);
	char *repopath = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/repo/", NULL, NULL);
	char *remotepath1 = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/remote1/", NULL, NULL);
	char *remotepath2 = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/remote2/", NULL, NULL);
	char *ssldir = "/var/empty/";

	char *file_endpoint = "tcp://127.0.0.1:95953";
	char *file_pubpoint = "tcp://127.0.0.1:95954";
	char *graph_endpoint = "tcp://127.0.0.1:95955";
	char *graph_pubpoint = "tcp://127.0.0.1:95956";

	int rc = mkdir(logpath, S_IRWXU);
	rc = mkdir(stagepath, S_IRWXU);
	rc = mkdir(repopath, S_IRWXU);
	rc = mkdir(remotepath1, S_IRWXU);
	rc = mkdir(remotepath2, S_IRWXU);

	prologue(argv[0]);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff_master(ssldir, stagepath, repopath, logpath,
		file_endpoint, graph_endpoint, file_pubpoint, graph_pubpoint);
	sleep(1);
	forkoff_remote(ssldir, remotepath1, file_endpoint);
	sleep(1);
	forkoff_remote(ssldir, remotepath2, file_endpoint);
	sleep(5);
	return run(ssldir, stagepath, repopath, logpath, remotepath1, remotepath2,
		file_endpoint, graph_endpoint, file_pubpoint, graph_pubpoint);
}

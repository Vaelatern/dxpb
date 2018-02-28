/*
 * harness-builder.c
 *
 * DXPB package worker - test harness
 */

#define _POSIX_C_SOURCE 200809L

#define HARNESS_BUILD_PKGNAME "ucl"
#define HARNESS_BUILD_VERSION "1.03_5"
#define HARNESS_BUILD_PKGNAME2 "zsync"
#define HARNESS_BUILD_VERSION2 "0.6.2_3"
#define HARNESS_BUILD_ARCH "armv7l-musl"
#define HARNESS_BUILD_HOSTARCH "x86_64"

#include <czmq.h>
#include <sys/stat.h>
#include "dxpb.h"
#include "bstring.h"
#include "bfs.h"
#include "pkggraph_msg.h"
#include "pkggraph_worker.h"

#include "dxpb-common.h"

void
forkoff(const char *endpoint, const char *hostdir, const char *masterdir,
		const char *repopath, const char *workspec, const char *ssldir,
		const char *varrundir)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-builder", "dxpb-builder",
				"-g", endpoint, "-T", varrundir,
				"-H", hostdir, "-m", masterdir, "-p", repopath,
				"-W", workspec, "-k", ssldir, NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

int
run(const char *endpoint, const char *hostdir, const char *masterdir,
		const char *repopath, const char *workspec, const char *ssldir,
		const char *varrundir,
		const char *pkgname, const char *version, const char *arch,
		const char *pkgname2, const char *version2)
{
	(void)varrundir;
	assert(hostdir);
	assert(masterdir);
	assert(repopath);
	assert(workspec);
	assert(ssldir);

	int rc;

	pkggraph_msg_t *msg = pkggraph_msg_new();

	zsock_t *graph = zsock_new (ZMQ_ROUTER);
	assert(graph);

	rc = zsock_attach(graph, endpoint, true);
	assert(rc == 0);

#define SEND(this, msg, sock)        { \
					pkggraph_msg_set_id(msg, TOMSG(this)); \
					rc = pkggraph_msg_send(msg, sock); \
					assert(rc == 0); \
				}

#define GET(mymsg, sock)        { \
					zpoller_t *p = zpoller_new(sock, NULL); \
					(void) zpoller_wait(p, 5*60*1000); \
					assert(!zpoller_expired(p)); \
					if (zpoller_terminated(p)) \
					exit(-1); \
					rc = pkggraph_msg_recv(mymsg, sock); \
					assert(rc == 0); \
					zpoller_destroy(&p); \
				}

#define TOMSG(str)                   PKGGRAPH_MSG_##str
#define SETMSG(what, msg, to)        pkggraph_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkggraph_msg_##what(msg) == eq)
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkggraph_msg_##what(msg), eq) == 0)

	/* And now we get to work */

	GET(msg, graph);
	ASSERTMSG(id, msg, TOMSG(HELLO));
	SEND(ROGER, msg, graph);

	GET(msg, graph);
	ASSERTMSG(id, msg, TOMSG(IMAWORKER));

	do {
		SEND(ROGER, msg, graph);
		GET(msg, graph);
	} while (pkggraph_msg_id(msg) == TOMSG(PING));
	fprintf(stderr, "Broke out of listening loop\n");
	ASSERTMSG(id, msg, TOMSG(ICANHELP));
	ASSERTMSGSTR(hostarch, msg, HARNESS_BUILD_HOSTARCH );
	ASSERTMSGSTR(targetarch, msg, arch);
	//int addr = pkggraph_msg_addr(msg);
	//int check = pkggraph_msg_check(msg);
	//int cost = pkggraph_msg_cost(msg);

	SETMSG(pkgname, msg, pkgname);
	SETMSG(version, msg, version);
	SETMSG(arch, msg, arch);
	SEND(NEEDPKG, msg, graph);

	do {
		SEND(ROGER, msg, graph);
		GET(msg, graph);
		fprintf(stderr, (char *)zchunk_data(pkggraph_msg_logs(msg)));
	} while (pkggraph_msg_id(msg) == TOMSG(LOGHERE));

	assert(pkggraph_msg_id(msg) == TOMSG(JOB_ENDED));
	ASSERTMSGSTR(pkgname, msg, pkgname);
	ASSERTMSGSTR(version, msg, version);
	ASSERTMSGSTR(arch, msg, arch);

	do {
		SEND(ROGER, msg, graph);
		GET(msg, graph);
	} while (pkggraph_msg_id(msg) == TOMSG(PING));
	ASSERTMSG(id, msg, TOMSG(ICANHELP));
	ASSERTMSGSTR(hostarch, msg, HARNESS_BUILD_HOSTARCH);
	ASSERTMSGSTR(targetarch, msg, arch);
	//int addr = pkggraph_msg_addr(msg);
	//int check = pkggraph_msg_check(msg);
	//int cost = pkggraph_msg_cost(msg);

	SETMSG(pkgname, msg, pkgname2);
	SETMSG(version, msg, version2);
	SETMSG(arch, msg, arch);
	SEND(NEEDPKG, msg, graph);

	do {
		SEND(ROGER, msg, graph);
		GET(msg, graph);
		fprintf(stderr, (char *)zchunk_data(pkggraph_msg_logs(msg)));
	} while (pkggraph_msg_id(msg) == TOMSG(LOGHERE));

	assert(pkggraph_msg_id(msg) == TOMSG(JOB_ENDED));
	ASSERTMSGSTR(pkgname, msg, pkgname2);
	ASSERTMSGSTR(version, msg, version2);
	ASSERTMSGSTR(arch, msg, arch);

	/* Work over, let's clean up. */

	pkggraph_msg_destroy(&msg);

	zsock_destroy(&graph);

	return 0;
}

int
main(int argc, char * const *argv)
{
	(void) argc;
	char *tmppath = strdup("/tmp/dxpb-harness-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	assert(ourdir);
	char *repopath = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/repo/", NULL, NULL);
	char *varrundir = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/varrun/", NULL, NULL);
	char *ssldir = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/ssl/", NULL, NULL);

	char *masterdir = bstring_add(bstring_add(NULL, repopath, NULL, NULL), "/masterdir/", NULL, NULL);
	char *hostdir = bstring_add(bstring_add(NULL, repopath, NULL, NULL), "/hostdir/", NULL, NULL);

	char *endpoint = "tcp://127.0.0.1:95912";

	char *workspec = "x86_64:armv7l-musl:100:yescross";

	int rc;

	rc = mkdir(varrundir, S_IRWXU);
	assert(rc == 0);
	rc = mkdir(repopath, S_IRWXU);
	assert(rc == 0);
	rc = mkdir(ssldir, S_IRWXU);
	assert(rc == 0);

	prologue(argv[0]);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff(endpoint, hostdir, masterdir, repopath, workspec, ssldir, varrundir);

	char *pkgname = HARNESS_BUILD_PKGNAME;
	char *version = HARNESS_BUILD_VERSION;
	char *arch = HARNESS_BUILD_ARCH;
	char *pkgname2 = HARNESS_BUILD_PKGNAME2;
	char *version2 = HARNESS_BUILD_VERSION2;
	return run(endpoint, hostdir, masterdir, repopath, workspec, ssldir, varrundir, pkgname, version, arch, pkgname2, version2);
}

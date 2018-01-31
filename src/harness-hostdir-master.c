/*
 * harness-hostdir-master.c
 *
 * DXPB repository manager - test harness
 */

#define _POSIX_C_SOURCE 200809L

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

void
forkoff(const char *ssldir, const char *sdir, const char *rdir,
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

int
run(const char *ssldir, const char *sdir, const char *rdir,
		const char *ldir, const char *file_end, const char *graph_end,
		const char *file_pub, const char *graph_pub)
{
	(void) file_pub;
	(void) graph_pub;
	assert(sdir);
	assert(rdir);
	assert(ldir);
	assert(file_end);
	assert(graph_end);
	assert(file_pub);
	assert(graph_pub);
	assert(ssldir);

	char *repopath = bstring_add(bstring_add(NULL, rdir, NULL, NULL),
	     "/", NULL, NULL);
	char *pkgfile = bxbps_pkg_to_filename("foo", DXPB_VERSION, "noarch");
	char *pkgpath = bstring_add(strdup(repopath), pkgfile, NULL, NULL);

	int rc;

	pkggraph_msg_t *graph_msg = pkggraph_msg_new();
	pkgfiles_msg_t *file_msg = pkgfiles_msg_new();

	zsock_t *graph = zsock_new (ZMQ_ROUTER);
	zsock_t *file  = zsock_new (ZMQ_DEALER);
	zsock_t *file2  = zsock_new (ZMQ_DEALER);
	assert(graph);
	assert(file);
	assert(file2);

	zsock_attach(graph, graph_end, true);
	zsock_attach(file, file_end, false);
	zsock_attach(file2, file_end, false);

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


#define TOMSG(str)                   PKGFILES_MSG_##str
#define SETMSG(what, msg, to)        pkgfiles_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkgfiles_msg_##what(msg) == eq)
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkgfiles_msg_##what(msg), eq) == 0)

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

	bfs_touch(pkgpath);

	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "noarch");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(PKGHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "noarch");
	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "x86_64");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(arch, file_msg, "x86_64-musl");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(arch, file_msg, "i686");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	for (int i = 0; i < 3; i++) {
		GET(file_msg, file);
		ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
		ASSERTMSGSTR(pkgname, file_msg, "foo");
		ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
		assert((strcmp(pkgfiles_msg_arch(file_msg), "x86_64") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "x86_64-musl") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "i686") == 0));
	}

	SEND(TOMSG(HELLO), file_msg, file2);
	GET(file_msg, file2);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));

	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "armv6hf");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	GET(file_msg, file2);
	ASSERTMSG(id, file_msg, TOMSG(ISPKGHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "armv6hf");
	SEND(TOMSG(PKGNOTHERE), file_msg, file2);
	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "armv7hf-musl");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	GET(file_msg, file2);
	ASSERTMSG(id, file_msg, TOMSG(ISPKGHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "armv7hf-musl");
	SEND(TOMSG(PKGNOTHERE), file_msg, file2);
	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "aarch64");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	GET(file_msg, file2);
	ASSERTMSG(id, file_msg, TOMSG(ISPKGHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "aarch64");
	SEND(TOMSG(PKGNOTHERE), file_msg, file2);

	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "armv6hf");
	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "armv7hf-musl");
	GET(file_msg, file);
	ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
	ASSERTMSGSTR(pkgname, file_msg, "foo");
	ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
	ASSERTMSGSTR(arch, file_msg, "aarch64");

	for (int i = 0; i < 6; i++) {
		sleep(10);
		SEND(TOMSG(PING), file_msg, file);
		GET(file_msg, file);
		ASSERTMSG(id, file_msg, TOMSG(ROGER));
	}

	SETMSG(pkgname, file_msg, "foo");
	SETMSG(version, file_msg, DXPB_VERSION);
	SETMSG(arch, file_msg, "x86_64");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(arch, file_msg, "x86_64-musl");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	SETMSG(arch, file_msg, "i686");
	SEND(TOMSG(ISPKGHERE), file_msg, file);
	for (int i = 0; i < 3; i++) {
		GET(file_msg, file);
		ASSERTMSG(id, file_msg, TOMSG(PKGNOTHERE));
		ASSERTMSGSTR(pkgname, file_msg, "foo");
		ASSERTMSGSTR(version, file_msg, DXPB_VERSION);
		assert((strcmp(pkgfiles_msg_arch(file_msg), "x86_64") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "x86_64-musl") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "i686") == 0));
	}

	SEND(TOMSG(HELLO), file_msg, file2);
	GET(file_msg, file2);
	ASSERTMSG(id, file_msg, TOMSG(ROGER));


	/* Work over, let's clean up. */

	pkggraph_msg_destroy(&graph_msg);
	pkgfiles_msg_destroy(&file_msg);

	zsock_destroy(&graph);
	zsock_destroy(&file2);
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
	char *ssldir = "/var/empty/";

	char *file_endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "file"), NULL, NULL);
	char *file_pubpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pubFile"), NULL, NULL);
 	char *graph_endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "graph"), NULL, NULL);
	char *graph_pubpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pubGraph"), NULL, NULL);

	int rc;
	rc = bfs_ensure_sock_perms(file_endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(graph_endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(file_pubpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(graph_pubpoint);
	assert(rc == ERR_CODE_OK);

	rc = mkdir(logpath, S_IRWXU);
	rc = mkdir(stagepath, S_IRWXU);
	rc = mkdir(repopath, S_IRWXU);

	prologue(argv[0]);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff(ssldir, stagepath, repopath, logpath,
		file_endpoint, graph_endpoint, file_pubpoint, graph_pubpoint);
	return run(ssldir, stagepath, repopath, logpath,
		file_endpoint, graph_endpoint, file_pubpoint, graph_pubpoint);
}

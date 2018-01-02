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

void
forkoff(const char *ssldir, const char *sdir, const char *rdir,
		const char *ldir, const char *file_end, const char *graph_end,
		const char *file_pub, const char *graph_pub)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-hostdir-master", "dxpb-hostdir-master",
				"-r", rdir, "-l", ldir, "-s", sdir,
				"-g", graph_end, "-G", graph_pub,
				"-f", file_end, "-F", file_pub,
				"-k", ssldir,
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
	SSLDIR_UNUSED(ssldir);
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
	assert(graph);
	assert(file);

	zsock_attach(graph, graph_end, true);
	zsock_attach(file, file_end, false);

	/* And now we get to work */

	{
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_HELLO);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(file, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgfiles_msg_recv(file_msg, file);
		assert(rc == 0);
		assert(pkgfiles_msg_id(file_msg) == PKGFILES_MSG_ROGER);
		zpoller_destroy(&p);
	}
	{
		pkgfiles_msg_set_pkgname(file_msg, "foo");
		pkgfiles_msg_set_version(file_msg, DXPB_VERSION);
		pkgfiles_msg_set_arch(file_msg, "noarch");
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_ISPKGHERE);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(file, NULL);
		(void) zpoller_wait(p, 120*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgfiles_msg_recv(file_msg, file);
		assert(rc == 0);
		assert(pkgfiles_msg_id(file_msg) == PKGFILES_MSG_PKGNOTHERE);
		assert(strcmp(pkgfiles_msg_pkgname(file_msg), "foo") == 0);
		assert(strcmp(pkgfiles_msg_version(file_msg), DXPB_VERSION) == 0);
		assert(strcmp(pkgfiles_msg_arch(file_msg), "noarch") == 0);
		zpoller_destroy(&p);
	}
	bfs_touch(pkgpath);
	{
		pkgfiles_msg_set_pkgname(file_msg, "foo");
		pkgfiles_msg_set_version(file_msg, DXPB_VERSION);
		pkgfiles_msg_set_arch(file_msg, "noarch");
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_ISPKGHERE);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(file, NULL);
		(void) zpoller_wait(p, 120*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p)) {
			fprintf(stderr, "oops\n");
			exit(-1);
		}
		rc = pkgfiles_msg_recv(file_msg, file);
		assert(rc == 0);
		assert(pkgfiles_msg_id(file_msg) == PKGFILES_MSG_PKGHERE);
		assert(strcmp(pkgfiles_msg_pkgname(file_msg), "foo") == 0);
		assert(strcmp(pkgfiles_msg_version(file_msg), DXPB_VERSION) == 0);
		assert(strcmp(pkgfiles_msg_arch(file_msg), "noarch") == 0);
		zpoller_destroy(&p);
	}
	{
		pkgfiles_msg_set_pkgname(file_msg, "foo");
		pkgfiles_msg_set_version(file_msg, DXPB_VERSION);
		pkgfiles_msg_set_arch(file_msg, "x86_64");
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_ISPKGHERE);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	{
		pkgfiles_msg_set_pkgname(file_msg, "foo");
		pkgfiles_msg_set_version(file_msg, DXPB_VERSION);
		pkgfiles_msg_set_arch(file_msg, "x86_64-musl");
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_ISPKGHERE);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	{
		pkgfiles_msg_set_pkgname(file_msg, "foo");
		pkgfiles_msg_set_version(file_msg, DXPB_VERSION);
		pkgfiles_msg_set_arch(file_msg, "i686");
		pkgfiles_msg_set_id(file_msg, PKGFILES_MSG_ISPKGHERE);
		rc = pkgfiles_msg_send(file_msg, file);
		assert(rc == 0);
	}
	for (int i = 0; i < 3; i++) {
		zpoller_t *p = zpoller_new(file, NULL);
		(void) zpoller_wait(p, 120*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgfiles_msg_recv(file_msg, file);
		assert(rc == 0);
		assert(pkgfiles_msg_id(file_msg) == PKGFILES_MSG_PKGNOTHERE);
		assert(strcmp(pkgfiles_msg_pkgname(file_msg), "foo") == 0);
		assert(strcmp(pkgfiles_msg_version(file_msg), DXPB_VERSION) == 0);
		assert((strcmp(pkgfiles_msg_arch(file_msg), "x86_64") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "x86_64-musl") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "i686") == 0));
		zpoller_destroy(&p);
	}

	/* Work over, let's clean up. */

	pkggraph_msg_destroy(&graph_msg);
	pkgfiles_msg_destroy(&file_msg);

	zsock_destroy(&graph);
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

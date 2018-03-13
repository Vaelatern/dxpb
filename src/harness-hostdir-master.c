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
	zsock_t *file2 = zsock_new (ZMQ_DEALER);
	assert(graph);
	assert(file);
	assert(file2);

	zsock_attach(graph, graph_end, true);
	zsock_attach(file, file_end, false);
	zsock_attach(file2, file_end, false);

	zchunk_t *chunk = NULL;

#include "harness-macros.inc"

	/* And now we get to work */

	GGET();
	GSRTID(HELLO);
	GSEND(ROGER);

	GGET();
	GSRTID(IAMSTORAGE);
	GSEND(ROGER);

	GSET(pkgname, "abc");
	GSET(version, "0.efg");
	GSET(arch, "noarch");
	GSEND(RESETLOG);
	GGET();
	GSRTID(ROGER);

	GSET(pkgname, "abc");
	GSET(version, "0.efg");
	GSET(arch, "noarch");
	GSEND(RESETLOG);
	GGET();
	GSRTID(ROGER);

	GSET(pkgname, "def");
	GSET(version, "0.ghi");
	GSET(arch, "x86_64");
	GSEND(RESETLOG);
	GGET();
	GSRTID(ROGER);

	chunk = zchunk_slurp("/usr/share/licenses/BSD", 2000);
	GSET(pkgname, "abc");
	GSET(version, "0.efg");
	GSET(arch, "noarch");
	GSET(logs, &chunk);
	GSEND(LOGHERE);
	GGET();
	GSRTID(ROGER);

	FILE *fp = fopen("/usr/share/licenses/Apache-2.0", "r");
	while (fp != NULL) {
		chunk = zchunk_read(fp, 3);
		if (zchunk_size(chunk) == 0) {
			fclose(fp);
			fp = NULL;
		}
		GSET(pkgname, "def");
		GSET(version, "0.ghi");
		GSET(arch, "x86_64");
		GSET(logs, &chunk);
		GSEND(LOGHERE);
		GGET();
		GSRTID(ROGER);
	}

	FSEND(HELLO);
	FGET();
	FSRTID(ROGER);
	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "noarch");
	FSEND(ISPKGHERE);
	FGET();
	FSRTID(PKGNOTHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "noarch");

	bfs_touch(pkgpath);

	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "noarch");
	FSEND(ISPKGHERE);
	FGET();
	FSRTID(PKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "noarch");
	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "x86_64");
	FSEND(ISPKGHERE);
	FSET(arch, "x86_64-musl");
	FSEND(ISPKGHERE);
	FSET(arch, "i686");
	FSEND(ISPKGHERE);
	for (int i = 0; i < 3; i++) {
		FGET();
		FSRTID(PKGNOTHERE);
		FSRTS(pkgname, "foo");
		FSRTS(version, DXPB_VERSION);
		assert((strcmp(pkgfiles_msg_arch(file_msg), "x86_64") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "x86_64-musl") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "i686") == 0));
	}

	FWSEND(HELLO, file2);
	FWGET(file2);
	FSRTID(ROGER);

	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "armv6hf");
	FSEND(ISPKGHERE);
	FWGET(file2);
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "armv6hf");
	FWSEND(PKGNOTHERE, file2);
	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "armv7hf-musl");
	FSEND(ISPKGHERE);
	FWGET(file2);
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "armv7hf-musl");
	FWSEND(PKGNOTHERE, file2);
	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "aarch64");
	FSEND(ISPKGHERE);
	FWGET(file2);
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "aarch64");
	FWSEND(PKGNOTHERE, file2);

	FGET();
	FSRTID(PKGNOTHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "armv6hf");
	FGET();
	FSRTID(PKGNOTHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "armv7hf-musl");
	FGET();
	FSRTID(PKGNOTHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, DXPB_VERSION);
	FSRTS(arch, "aarch64");

	for (int i = 0; i < 6; i++) {
		sleep(10);
		FSEND(PING);
		FGET();
		FSRTID(ROGER);

		GGET();
		GSRTID(PING);
		GSEND(ROGER);
	}

	FSET(pkgname, "foo");
	FSET(version, DXPB_VERSION);
	FSET(arch, "x86_64");
	FSEND(ISPKGHERE);
	FSET(arch, "x86_64-musl");
	FSEND(ISPKGHERE);
	FSET(arch, "i686");
	FSEND(ISPKGHERE);
	for (int i = 0; i < 3; i++) {
		FGET();
		FSRTID(PKGNOTHERE);
		FSRTS(pkgname, "foo");
		FSRTS(version, DXPB_VERSION);
		assert((strcmp(pkgfiles_msg_arch(file_msg), "x86_64") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "x86_64-musl") == 0) ||
			(strcmp(pkgfiles_msg_arch(file_msg), "i686") == 0));
	}

	FWSEND(HELLO, file2);
	FWGET(file2);
	FSRTID(ROGER);

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

	char *file_endpoint = bstring_add(bstring_add(NULL,  "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "file"), NULL, NULL);
	char *file_pubpoint = bstring_add(bstring_add(NULL,  "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pubFile"), NULL, NULL);
 	char *graph_endpoint = "tcp://127.0.0.1:6015";
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

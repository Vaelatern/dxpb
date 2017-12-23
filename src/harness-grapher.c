/*
 * harness-grapher.c
 *
 * Heart of dxpb - test harness
 */

#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <czmq.h>
#include "dxpb.h"
#include "bfs.h"
#include "bstring.h"
#include "bwords.h"
#include "bxpkg.h"
#include "pkgimport_msg.h"
#include "pkggraph_msg.h"
#include "pkgfiles_msg.h"
#include "pkgimport_grapher.h"
#include "pkggraph_grapher.h"
#include "pkgfiler_grapher.h"

#include "dxpb-common.h"

#define VERBOSE_FLAG 1
#define ERR_FLAG 2

void
help(void)
{
#include "dxpb-grapher-help.inc"
	printf("%.*s\n", ___doc_dxpb_grapher_help_txt_len, ___doc_dxpb_grapher_help_txt);
}

#define DXPB_HANDLE_SOCK(type, sock, msg, outbound, direction, retVal) { \
		pkg##type##_msg_recv(msg, sock); \
		retVal = handle_##type##_##direction##_msg(msg, outbound); \
	}

void
forkoff(const char *dbpath, const char *import_endpoint,
		const char *graph_endpoint, const char *file_endpoint,
		const char *pubpoint, const char *ssldir)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-grapher", "dxpb-grapher", "-d", dbpath,
				"-f", file_endpoint, "-g", graph_endpoint,
				"-i", import_endpoint, "-I", pubpoint,
				"-k", ssldir, NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

int
run(const char *dbpath, const char *import_endpoint,
		const char *graph_endpoint, const char *file_endpoint,
		const char *pubpoint, const char *ssldir)
{
	SSLDIR_UNUSED(ssldir);
	(void) pubpoint;
	assert(dbpath);
	assert(import_endpoint);
	assert(file_endpoint);
	assert(graph_endpoint);
	assert(ssldir);
	unlink(dbpath);

	int rc;

	pkgimport_msg_t *import_msg = pkgimport_msg_new();
	pkggraph_msg_t *graph_msg = pkggraph_msg_new();
	pkgfiles_msg_t *file_msg = pkgfiles_msg_new();

	zsock_t *import = zsock_new (ZMQ_ROUTER);
	zsock_t *graph  = zsock_new (ZMQ_ROUTER);
	zsock_t *file   = zsock_new (ZMQ_ROUTER);
	assert(import);
	assert(graph);
	assert(file);

	zsock_attach(import, import_endpoint, true);
	zsock_attach(graph, graph_endpoint, true);
	zsock_attach(file, file_endpoint, true);

	/* And now we get to work */

	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_HELLO);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_ROGER);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_IAMTHEGRAPHER);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_ROGER);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_PLZREADALL);
		zpoller_destroy(&p);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_STABLESTATUSPLZ);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_UNSTABLE);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGDEL);
		pkgimport_msg_set_pkgname(import_msg, "rmme");
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	for (int i = 1; i < ARCH_HOST; i++) {
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGINFO);
		pkgimport_msg_set_pkgname(import_msg, "foo");
		pkgimport_msg_set_version(import_msg, "0.0.1");
		pkgimport_msg_set_arch(import_msg, pkg_archs_str[i]);
		pkgimport_msg_set_nativehostneeds(import_msg, "");
		pkgimport_msg_set_nativetargetneeds(import_msg, "");
		pkgimport_msg_set_crosshostneeds(import_msg, "");
		pkgimport_msg_set_crosstargetneeds(import_msg, "");
		pkgimport_msg_set_cancross(import_msg, 0);
		pkgimport_msg_set_broken(import_msg, 0);
		pkgimport_msg_set_bootstrap(import_msg, 0);
		pkgimport_msg_set_restricted(import_msg, 0);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGINFO);
		pkgimport_msg_set_pkgname(import_msg, "foo");
		pkgimport_msg_set_version(import_msg, "0.0.1");
		pkgimport_msg_set_arch(import_msg, pkg_archs_str[ARCH_TARGET]);
		pkgimport_msg_set_nativehostneeds(import_msg, "");
		pkgimport_msg_set_nativetargetneeds(import_msg, "");
		pkgimport_msg_set_crosshostneeds(import_msg, "");
		pkgimport_msg_set_crosstargetneeds(import_msg, "");
		pkgimport_msg_set_cancross(import_msg, 0);
		pkgimport_msg_set_broken(import_msg, 0);
		pkgimport_msg_set_bootstrap(import_msg, 0);
		pkgimport_msg_set_restricted(import_msg, 0);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	for (int i = 1; i <= ARCH_HOST; i++) {
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_ROGER);
		zpoller_destroy(&p);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_STABLESTATUSPLZ);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_STABLE);
		pkgimport_msg_set_commithash(import_msg, "aabdbababdbabababdbabababdbdbabaabdbdbab");
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	for (int i = 1; i < ARCH_HOST; i++) {
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGINFO);
		pkgimport_msg_set_pkgname(import_msg, "bar");
		pkgimport_msg_set_version(import_msg, "0.1.0");
		pkgimport_msg_set_arch(import_msg, pkg_archs_str[i]);
		pkgimport_msg_set_nativehostneeds(import_msg, "foo");
		pkgimport_msg_set_nativetargetneeds(import_msg, "foo");
		pkgimport_msg_set_crosshostneeds(import_msg, "foo");
		pkgimport_msg_set_crosstargetneeds(import_msg, "foo");
		pkgimport_msg_set_cancross(import_msg, 0);
		pkgimport_msg_set_broken(import_msg, 0);
		pkgimport_msg_set_bootstrap(import_msg, 1);
		pkgimport_msg_set_restricted(import_msg, 0);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGINFO);
		pkgimport_msg_set_pkgname(import_msg, "bar");
		pkgimport_msg_set_version(import_msg, "0.1.0");
		pkgimport_msg_set_arch(import_msg, pkg_archs_str[ARCH_TARGET]);
		pkgimport_msg_set_nativehostneeds(import_msg, "");
		pkgimport_msg_set_nativetargetneeds(import_msg, "");
		pkgimport_msg_set_crosshostneeds(import_msg, "");
		pkgimport_msg_set_crosstargetneeds(import_msg, "");
		pkgimport_msg_set_cancross(import_msg, 0);
		pkgimport_msg_set_broken(import_msg, 0);
		pkgimport_msg_set_bootstrap(import_msg, 0);
		pkgimport_msg_set_restricted(import_msg, 0);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	for (int i = 1; i <= ARCH_HOST; i++) {
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_ROGER);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGINFO);
		pkgimport_msg_set_pkgname(import_msg, "baz");
		pkgimport_msg_set_version(import_msg, "91230");
		pkgimport_msg_set_arch(import_msg, pkg_archs_str[ARCH_NOARCH]);
		pkgimport_msg_set_nativehostneeds(import_msg, "bar");
		pkgimport_msg_set_nativetargetneeds(import_msg, "bar");
		pkgimport_msg_set_crosshostneeds(import_msg, "foo");
		pkgimport_msg_set_crosstargetneeds(import_msg, "foo");
		pkgimport_msg_set_cancross(import_msg, 0);
		pkgimport_msg_set_broken(import_msg, 0);
		pkgimport_msg_set_bootstrap(import_msg, 0);
		pkgimport_msg_set_restricted(import_msg, 0);
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_ROGER);
		zpoller_destroy(&p);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_STABLESTATUSPLZ);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_STABLE);
		pkgimport_msg_set_commithash(import_msg, "aabdbababdbabababdbabababdbdbabaabdbdbac");
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_PKGDEL);
		pkgimport_msg_set_pkgname(import_msg, "alsormme");
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	{
		zpoller_t *p = zpoller_new(import, NULL);
		(void) zpoller_wait(p, 10*1000);
		assert(!zpoller_expired(p));
		if (zpoller_terminated(p))
			exit(-1);
		rc = pkgimport_msg_recv(import_msg, import);
		assert(rc == 0);
		assert(pkgimport_msg_id(import_msg) == PKGIMPORT_MSG_STABLESTATUSPLZ);
		zpoller_destroy(&p);
	}
	{
		pkgimport_msg_set_id(import_msg, PKGIMPORT_MSG_STABLE);
		pkgimport_msg_set_commithash(import_msg, "aabdbababdbabababdbabababdbdbabaabdbdbad");
		rc = pkgimport_msg_send(import_msg, import);
		assert(rc == 0);
	}
	/* Work over, let's clean up. */

	pkgimport_msg_destroy(&import_msg);
	pkggraph_msg_destroy(&graph_msg);
	pkgfiles_msg_destroy(&file_msg);

	zsock_destroy(&import);
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
 	char *dbpath = bstring_add(bstring_add(NULL, ourdir, NULL, NULL), "/test.db", NULL, NULL);
 	char *import_endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "import"), NULL, NULL);
 	char *file_endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "file"), NULL, NULL);
 	char *graph_endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "graph"), NULL, NULL);
	char *pubpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pub"), NULL, NULL);
 	char *ssldir = "/var/empty/";

	int rc = bfs_ensure_sock_perms(import_endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(file_endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(graph_endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(pubpoint);
	assert(rc == ERR_CODE_OK);

	prologue(argv[0]);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff(dbpath, import_endpoint, graph_endpoint, file_endpoint,
			pubpoint, ssldir);
	return run(dbpath, import_endpoint, graph_endpoint, file_endpoint,
			pubpoint, ssldir);
}

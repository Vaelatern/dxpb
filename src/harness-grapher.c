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

static void
append_pkgkey_to_list(zlist_t *list, const char *name, const char *ver, const char *arch)
{
	assert(list);
	zlist_comparefn(list, (zlist_compare_fn *)strcmp);
	zlist_autofree(list);
	char *key = bstring_add(bstring_add(strdup(name), ver, NULL, NULL), arch, NULL, NULL);
	zlist_append(list, key);
	FREE(key);
}

static void
remove_pkgkey_from_list(zlist_t *list, int deltype, const char *name, const char *ver, const char *arch)
{
	assert(list);
	zlist_comparefn(list, (zlist_compare_fn *)strcmp);
	char *key = strdup(name);
	assert(key);
	if (!deltype)
		key = bstring_add(bstring_add(key, ver, NULL, NULL), arch, NULL, NULL);
	if (!zlist_exists(list, key)) {
		printf("Couldn't find: %s\n", key);
		assert(false);
	}
	zlist_remove(list, key);
	FREE(key);
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

	zlist_t *list = zlist_new();

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

#define TOMSG(type, what) PKG##type##_MSG_##what

#define SEND(type, TYPE, WHAT, msg, target) { \
		pkg##type##_msg_set_id(msg, TOMSG(TYPE, WHAT)); \
		rc = pkg##type##_msg_send(msg, target); \
		assert(rc == 0 && "message sending didn't work"); \
	}
#define ISEND(WHAT) SEND(import, IMPORT, WHAT, import_msg, import)
#define GSEND(WHAT) SEND(graph, GRAPH, WHAT, graph_msg, graph)
#define FSEND(WHAT) SEND(files, FILES, WHAT, file_msg, file)

#define SET(type, what, msg, to) pkg##type##_msg_set_##what(msg, to)
#define ISET(what, to) SET(import, what, import_msg, to)
#define GSET(what, to) SET(graph, what, graph_msg, to)
#define FSET(what, to) SET(files, what, file_msg, to)

#define GET(type, mymsg, sock) { \
		zpoller_t *p = zpoller_new(sock, NULL); \
		(void) zpoller_wait(p, 30*1000); \
		assert(!zpoller_expired(p) && "Timeout while waiting for any message"); \
		if (zpoller_terminated(p)) \
			exit(-1); \
		rc = pkg##type##_msg_recv(mymsg, sock); \
		assert(rc == 0 && "We thought there was a message. We were wrong"); \
		zpoller_destroy(&p); \
	}
#define IGET()	GET(import, import_msg, import)
#define GGET()	GET(graph, graph_msg, graph)
#define FGET()	GET(files, file_msg, file)

#define IF(type, msg, field, goal) if(pkg##type##_msg_##field(msg) == goal)
#define IIF(field, goal) IF(import, import_msg, field, goal)
#define GIF(field, goal) IF(graph, graph_msg, field, goal)
#define FIF(field, goal) IF(files, file_msg, field, goal)
#define IIFID(goal) IIF(id, TOMSG(IMPORT, goal))
#define GIFID(goal) GIF(id, TOMSG(GRAPH, goal))
#define FIFID(goal) FIF(id, TOMSG(FILES, goal))

// SRT is shorthand for assert(), and STRS, for assert(strcmp())
#define SRT(type, msg, field, goal) assert(pkg##type##_msg_##field(msg) == goal)
#define ISRT(field, goal) SRT(import, import_msg, ##field, ##goal)
#define GSRT(field, goal) SRT(graph, graph_msg, ##field, ##goal)
#define FSRT(field, goal) SRT(files, file_msg, ##field, ##goal)
#define ISRTID(goal) SRT(import, import_msg, id, TOMSG(IMPORT, goal))
#define GSRTID(goal) SRT(graph, graph_msg, id, TOMSG(GRAPH, goal))
#define FSRTID(goal) SRT(files, file_msg, id, TOMSG(FILES, goal))

#define SRTS(type, msg, field, goal) assert(strcmp(pkg##type##_msg_##field(msg), goal) == 0)
#define ISRTS(field, goal) SRTS(import, import_msg,  field , goal)
#define GSRTS(field, goal) SRTS(graph, graph_msg,  field , goal)
#define FSRTS(field, goal) SRTS(files, file_msg,  field , goal)

	/* And now we get to work */

	IGET();
	ISRTID(HELLO);
	ISEND(ROGER);
	IGET();
	ISRTID(IAMTHEGRAPHER);
	ISEND(ROGER);
	IGET();
	ISRTID(PLZREADALL);
	IGET();
	ISRTID(STABLESTATUSPLZ);
	ISEND(UNSTABLE);
	ISET(pkgname, "rmme");
	ISEND(PKGDEL);
	for (int i = 1; i < ARCH_HOST; i++) {
		ISET(pkgname, "foo");
		ISET(version, "0.0.1");
		ISET(arch, pkg_archs_str[i]);
		ISET(nativehostneeds, "");
		ISET(nativetargetneeds, "giggity");
		ISET(crosshostneeds, "");
		ISET(crosstargetneeds, "giggity");
		ISET(cancross, 0);
		ISET(broken, 0);
		ISET(bootstrap, 0);
		ISET(restricted, 0);
		ISEND(PKGINFO);
	}
	ISET(pkgname, "foo");
	ISET(version, "0.0.1");
	ISET(arch, pkg_archs_str[ARCH_TARGET]);
	ISET(nativehostneeds, "");
	ISET(nativetargetneeds, "");
	ISET(crosshostneeds, "");
	ISET(crosstargetneeds, "");
	ISET(cancross, 0);
	ISET(broken, 0);
	ISET(bootstrap, 0);
	ISET(restricted, 0);
	ISEND(PKGINFO);
	ISET(arch, pkg_archs_str[ARCH_HOST]);
	ISEND(PKGINFO);

	for (int i = 0; i < ARCH_TARGET; i++) {
		IGET();
		ISRTID(ROGER);
	}
	ISET(pkgname, "giggity");
	ISET(version, "1.5.0");
	ISET(arch, pkg_archs_str[ARCH_NOARCH]);
	ISET(nativehostneeds, "");
	ISET(nativetargetneeds, "");
	ISET(crosshostneeds, "");
	ISET(crosstargetneeds, "");
	ISET(cancross, 0);
	ISET(broken, 0);
	ISET(bootstrap, 1);
	ISET(restricted, 0);
	ISEND(PKGINFO);
	ISET(arch, pkg_archs_str[ARCH_HOST]);
	ISEND(PKGINFO);
	IGET();
	ISRTID(ROGER);
	IGET();
	ISRTID(ROGER);
	IGET();
	ISRTID(STABLESTATUSPLZ);
	ISET(commithash, "aabdbababdbabababdbabababdbdbabaabdbdbab");
	ISEND(STABLE);
	for (int i = 1; i < ARCH_HOST; i++) {
		ISET(pkgname, "bar");
		ISET(version, "0.1.0");
		ISET(arch, pkg_archs_str[i]);
		ISET(nativehostneeds, "foo");
		ISET(nativetargetneeds, "foo");
		ISET(crosshostneeds, "foo");
		ISET(crosstargetneeds, "foo");
		ISET(cancross, 0);
		ISET(broken, 0);
		ISET(bootstrap, 1);
		ISET(restricted, 0);
		ISEND(PKGINFO);
	}
	ISET(pkgname, "bar");
	ISET(version, "0.1.0");
	ISET(arch, pkg_archs_str[ARCH_TARGET]);
	ISET(nativehostneeds, "");
	ISET(nativetargetneeds, "");
	ISET(crosshostneeds, "");
	ISET(crosstargetneeds, "");
	ISET(cancross, 0);
	ISET(broken, 0);
	ISET(bootstrap, 0);
	ISET(restricted, 0);
	ISEND(PKGINFO);
	ISET(arch, pkg_archs_str[ARCH_HOST]);
	ISEND(PKGINFO);
	for (int i = 0; i < ARCH_TARGET; i++) {
		IGET();
		ISRTID(ROGER);
	}
	ISET(pkgname, "baz");
	ISET(version, "91230");
	ISET(arch, pkg_archs_str[ARCH_NOARCH]);
	ISET(nativehostneeds, "bar");
	ISET(nativetargetneeds, "bar");
	ISET(crosshostneeds, "foo");
	ISET(crosstargetneeds, "foo");
	ISET(cancross, 0);
	ISET(broken, 0);
	ISET(bootstrap, 0);
	ISET(restricted, 0);
	ISEND(PKGINFO);
	ISET(arch, pkg_archs_str[ARCH_HOST]);
	ISEND(PKGINFO);
	IGET();
	ISRTID(ROGER);
	IGET();
	ISRTID(ROGER);
	IGET();
	ISRTID(STABLESTATUSPLZ);
	ISET(commithash, "aabdbababdbabababdbabababdbdbabaabdbdbac");
	ISEND(STABLE);
	ISET(pkgname, "alsormme");
	ISEND(PKGDEL);
	IGET();
	ISRTID(STABLESTATUSPLZ);
	ISET(commithash, "aabdbababdbabababdbabababdbdbabaabdbdbad");
	ISEND(STABLE);

	/* Now we set up a graph, let's find out if files are there */
	FGET();
	FSRTID(HELLO);
	FSEND(ROGER);
	for (int i = 1; i < ARCH_HOST; i++) {
		append_pkgkey_to_list(list, "foo", "0.0.1", pkg_archs_str[i]);
	}
	append_pkgkey_to_list(list, "rmme", NULL, NULL);
	append_pkgkey_to_list(list, "alsormme", NULL, NULL);
	append_pkgkey_to_list(list, "giggity", "1.5.0", pkg_archs_str[ARCH_NOARCH]);
	{
		int max = zlist_size(list);
		for (int i = 0; i < max; i++) {
			FGET();
			int is_del;
			FIFID(ISPKGHERE)
				is_del = 0;
			else FIFID(PKGDEL)
				is_del = 1;
			else
				assert(false);
			remove_pkgkey_from_list(list, is_del,
					pkgfiles_msg_pkgname(file_msg),
					pkgfiles_msg_version(file_msg),
					pkgfiles_msg_arch(file_msg));
		}
	}
	assert(zlist_size(list) == 0);

	FSET(pkgname, "foo");
	FSET(version, "0.0.1");
	for (int i = 1; i < ARCH_HOST; i++) {
		FSET(arch, pkg_archs_str[i]);
		if (i != ARCH_AARCH64_MUSL && i != ARCH_X86_64) {
			append_pkgkey_to_list(list, "bar", "0.1.0", pkg_archs_str[i]);
			FSEND(PKGHERE);
		} else {
			FSEND(PKGNOTHERE);
		}
	}
	{
		int max = zlist_size(list);
		for (int i = 0; i < max; i++) {
			FGET();
			int is_del;
			FIFID(ISPKGHERE)
				is_del = 0;
			else FIFID(PKGDEL)
				is_del = 1;
			else
				assert(false);
			remove_pkgkey_from_list(list, is_del,
					pkgfiles_msg_pkgname(file_msg),
					pkgfiles_msg_version(file_msg),
					pkgfiles_msg_arch(file_msg));
		}
	}
	assert(zlist_size(list) == 0);

	/* Work over, let's clean up. */

	pkgimport_msg_destroy(&import_msg);
	pkggraph_msg_destroy(&graph_msg);
	pkgfiles_msg_destroy(&file_msg);

	zsock_destroy(&import);
	zsock_destroy(&graph);
	zsock_destroy(&file);

	zlist_destroy(&list);

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
 	char *import_endpoint = "tcp://127.0.0.1:6012";
 	char *file_endpoint = "tcp://127.0.0.1:6013";
 	char *graph_endpoint = "tcp://127.0.0.1:6014";
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

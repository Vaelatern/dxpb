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
#include "bworker_end_status.h"

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

#include "harness-macros.inc"

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
		ISET(nativehostneeds, "");
		ISET(nativetargetneeds, "baz\37foo");
		ISET(crosshostneeds, "");
		ISET(crosstargetneeds, "baz\37foo");
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
	ISET(nativehostneeds, "");
	ISET(nativetargetneeds, "");
	ISET(crosshostneeds, "foo");
	ISET(crosstargetneeds, "foo");
	ISET(cancross, 0);
	ISET(broken, 0);
	ISET(bootstrap, 0);
	ISET(restricted, 0);
	ISEND(PKGINFO);
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
	append_pkgkey_to_list(list, "rmme", NULL, NULL);
	append_pkgkey_to_list(list, "alsormme", NULL, NULL);
	append_pkgkey_to_list(list, "giggity", "1.5.0", pkg_archs_str[ARCH_NOARCH]);
	append_pkgkey_to_list(list, "baz", "91230", pkg_archs_str[ARCH_NOARCH]);
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

	FSET(pkgname, "rmme");
	FSEND(PKGISDEL);
	FSET(pkgname, "alsormme");
	FSEND(PKGISDEL);

	FSET(pkgname, "giggity");
	FSET(version, "1.5.0");
	FSET(arch, pkg_archs_str[ARCH_NOARCH]);
	FSEND(PKGNOTHERE);

	/* And now let's offer to build a package. */
	GGET();
	GSRTID(HELLO);
	GSEND(ROGER);
	GGET();
	GSRTID(IMTHEGRAPHER);
	GSEND(ROGER);
	GSET(hostarch, pkg_archs_str[ARCH_ARMV6L]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV6L]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 0);
	GSET(check, 0);
	GSEND(ICANHELP);
	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "giggity");
	GSRTS(version, "1.5.0");
	GSRTS(arch, pkg_archs_str[ARCH_NOARCH]);
	GSEND(FORGET_ABOUT_ME);
	GSET(check, 6000);
	GSEND(ICANHELP);
	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "giggity");
	GSRTS(version, "1.5.0");
	GSRTS(arch, pkg_archs_str[ARCH_NOARCH]);
	GSET(cause, END_STATUS_NODISK);
	GSEND(JOB_ENDED);
	GSEND(ICANHELP);
	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "giggity");
	GSRTS(version, "1.5.0");
	GSRTS(arch, pkg_archs_str[ARCH_NOARCH]);
	GSET(cause, END_STATUS_OK);
	GSEND(JOB_ENDED);

	GSET(hostarch, pkg_archs_str[ARCH_ARMV6L]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV6L]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 5);
	GSET(check, 1);
	GSEND(ICANHELP);

	FGET();
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "giggity");
	FSRTS(version, "1.5.0");
	FSRTS(arch, pkg_archs_str[ARCH_NOARCH]);
	FSEND(PKGHERE);

	for (int i = 1; i < ARCH_HOST; i++) {
		append_pkgkey_to_list(list, "foo", "0.0.1", pkg_archs_str[i]);
	}
	FSET(pkgname, "foo");
	FSET(version, "0.0.1");
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
			assert(!is_del);
			remove_pkgkey_from_list(list, is_del,
					pkgfiles_msg_pkgname(file_msg),
					pkgfiles_msg_version(file_msg),
					pkgfiles_msg_arch(file_msg));
			if (bpkg_enum_lookup(pkgfiles_msg_arch(file_msg)) == ARCH_ARMV6L_MUSL ||
					bpkg_enum_lookup(pkgfiles_msg_arch(file_msg)) == ARCH_ARMV7L) {
				FSEND(PKGNOTHERE);
			} else {
				FSEND(PKGHERE);
			}
		}
	}
	assert(zlist_size(list) == 0);

	GSET(hostarch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 1);
	GSET(check, 501);
	GSEND(ICANHELP);

	GSET(hostarch, pkg_archs_str[ARCH_ARMV7L]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV7L]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 2);
	GSET(check, 123123);
	GSEND(ICANHELP);

	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "foo");
	GSRTS(version, "0.0.1");
	GSRTS(arch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSRT(addr, 1);
	GSRT(check, 501);

	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "foo");
	GSRTS(version, "0.0.1");
	GSRTS(arch, pkg_archs_str[ARCH_ARMV7L]);
	GSRT(addr, 2);
	GSRT(check, 123123);

	GSEND(FORGET_ABOUT_ME);

	GSET(pkgname, "foo");
	GSET(version, "0.0.1");
	GSET(arch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSET(addr, 1);
	GSET(check, 501);
	GSET(cause, END_STATUS_OK);
	GSEND(JOB_ENDED);

	GSET(hostarch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 1);
	GSET(check, 502);
	GSEND(ICANHELP);

	GSET(hostarch, pkg_archs_str[ARCH_ARMV7L]);
	GSET(targetarch, pkg_archs_str[ARCH_ARMV7L]);
	GSET(iscross, 0);
	GSET(cost, 100);
	GSET(addr, 2);
	GSET(check, 123123);
	GSEND(ICANHELP);

	FGET();
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, "0.0.1");
	FSRTS(arch, pkg_archs_str[ARCH_ARMV6L_MUSL]);
	FSEND(PKGHERE);

	GGET();
	GSRTID(WORKERCANHELP);
	GSRTS(pkgname, "foo");
	GSRTS(version, "0.0.1");
	GSRTS(arch, pkg_archs_str[ARCH_ARMV7L]);
	GSRT(addr, 2);
	GSRT(check, 123123);
	GSEND(JOB_ENDED);

	FGET();
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "foo");
	FSRTS(version, "0.0.1");
	FSRTS(arch, pkg_archs_str[ARCH_ARMV7L]);
	FSEND(PKGHERE);

	// Now we make sure that dropped messages get picked up.
	for (int i = 0; i < 4; i++) {
		IGET();
		ISRTID(PING);
		ISEND(ROGER);
		GGET();
		GSRTID(PING);
		GSEND(ROGER);
		FGET();
		FSRTID(PING);
		FSEND(ROGER);
	}

	FGET();
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "baz");
	FSRTS(version, "91230");
	FSRTS(arch, pkg_archs_str[ARCH_NOARCH]);

	FSEND(PKGHERE);

	for (int i = 1; i < ARCH_HOST; i++) {
		append_pkgkey_to_list(list, "bar", "0.1.0", pkg_archs_str[i]);
	}
	FSET(pkgname, "bar");
	FSET(version, "0.1.0");
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
			assert(!is_del);
			remove_pkgkey_from_list(list, is_del,
					pkgfiles_msg_pkgname(file_msg),
					pkgfiles_msg_version(file_msg),
					pkgfiles_msg_arch(file_msg));
			if (bpkg_enum_lookup(pkgfiles_msg_arch(file_msg)) == ARCH_ARMV6L) {
				FSEND(PKGNOTHERE);
				GGET();
				GSRTID(WORKERCANHELP);
				GSRT(addr, 5);
				GSRT(check, 1);
				GSRTS(pkgname, "bar");
				GSRTS(version, "0.1.0");
				GSRTS(arch, pkg_archs_str[ARCH_ARMV6L]);
			} else {
				FSEND(PKGHERE);
			}
		}
	}

	GSET(pkgname, "bar");
	GSET(version, "0.1.0");
	GSET(arch, pkg_archs_str[ARCH_ARMV6L]);
	GSET(addr, 5);
	GSET(check, 1);
	GSET(cause, END_STATUS_OK);
	GSEND(JOB_ENDED);

	FGET();
	FSRTID(ISPKGHERE);
	FSRTS(pkgname, "bar");
	FSRTS(version, "0.1.0");
	FSRTS(arch, pkg_archs_str[ARCH_ARMV6L]);
	FSEND(PKGHERE);

	IGET();
	ISRTID(PING);
	ISEND(ROGER);
	GGET();
	GSRTID(PING);
	GSEND(ROGER);
	FGET();
	FSRTID(PING);
	FSEND(ROGER);

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

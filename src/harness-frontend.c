/*
 * harness-pkgimport-master.c
 *
 * DXPB import agent - test harness
 */

#define _POSIX_C_SOURCE 200809L

#include <czmq.h>
#include <sys/stat.h>
#include "dxpb.h"
#include "bstring.h"
#include "bfs.h"
#include "pkggraph_msg.h"
#include "bwords.h"
#include "bxpkg.h"
#include <bsd/stdlib.h>

#include "dxpb-common.h"

void
forkoff(const char *endpoint, const char *pubpoint, const char *ssldir)
{
	switch(fork()) {
	case 0:
		execlp("./dxpb-frontend", "dxpb-frontend",
				"-g", endpoint, "-G", pubpoint,
				"-k", ssldir, NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

int
run(const char *endpoint, const char *pubpoint, const char *ssldir)
{
	(void) ssldir;
	assert(endpoint);
	assert(pubpoint);
	int rc, i;
	uint32_t checks[2];

	pkggraph_msg_t *msg = pkggraph_msg_new();

	zsock_t *wrkr[2];
	zsock_t *storage  = zsock_new(ZMQ_DEALER);
	zsock_t *grphr  = zsock_new(ZMQ_DEALER);
	wrkr[0]  = zsock_new(ZMQ_DEALER);
	wrkr[1]  = zsock_new(ZMQ_DEALER);
	assert(storage);
	assert(grphr);
	assert(wrkr[0]);
	assert(wrkr[1]);

	zchunk_t *chunk;

	rc = zsock_attach(storage, endpoint, false);
	assert(rc == 0);
	rc = zsock_attach(grphr, endpoint, false);
	assert(rc == 0);
	rc = zsock_attach(wrkr[0], endpoint, false);
	assert(rc == 0);
	rc = zsock_attach(wrkr[1], endpoint, false);
	assert(rc == 0);

#define SEND(this, sock)	{ \
					pkggraph_msg_set_id(msg, this); \
					rc = pkggraph_msg_send(msg, sock); \
					assert(rc == 0); \
				}

#define GET(mymsg, sock)	{ \
					zpoller_t *p = zpoller_new(sock, NULL); \
					(void) zpoller_wait(p, 10*1000); \
					assert(!zpoller_expired(p)); \
					if (zpoller_terminated(p)) \
						exit(-1); \
					rc = pkggraph_msg_recv(mymsg, sock); \
					assert(rc == 0); \
					zpoller_destroy(&p); \
				}

#define TOMSG(str)		     PKGGRAPH_MSG_##str
#define SETMSG(what, msg, to) 	     pkggraph_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkggraph_msg_##what(msg) == eq)
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkggraph_msg_##what(msg), eq) == 0)

	/* And now we get to work */

	SEND(TOMSG(PING), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(IFORGOTU));

	SEND(TOMSG(HELLO), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IAMSTORAGE), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(PING), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(HELLO), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IAMSTORAGE), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(PING), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(HELLO), wrkr[0]);
	GET(msg, wrkr[0]);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IMAWORKER), wrkr[0]);
	GET(msg, wrkr[0]);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(HELLO), wrkr[1]);
	GET(msg, wrkr[1]);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IMAWORKER), wrkr[1]);
	GET(msg, wrkr[1]);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SETMSG(targetarch, msg, pkg_archs_str[ARCH_X86_64]);
	SETMSG(hostarch, msg, pkg_archs_str[ARCH_X86_64]);
	SETMSG(iscross, msg, 0);
	SETMSG(cost, msg, 100);
	SETMSG(addr, msg, 0);
	SETMSG(check, msg, 0);
	SEND(TOMSG(ICANHELP), wrkr[0]);
	SETMSG(targetarch, msg, pkg_archs_str[ARCH_X86_64_MUSL]);
	SEND(TOMSG(ICANHELP), wrkr[1]);

	SEND(TOMSG(HELLO), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IMTHEGRAPHER), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	for (i = 0; i < 2; i++) {
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(ICANHELP));
		ASSERTMSGSTR(targetarch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		checks[i] = pkggraph_msg_check(msg);
	}

	SEND(TOMSG(PING), storage);
	GET(msg, storage);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SETMSG(pkgname, msg, "foo");
	SETMSG(version, msg, "0.0.1");
	SETMSG(arch, msg, pkg_archs_str[ARCH_X86_64_MUSL]);
	SETMSG(check, msg, checks[0]);
	SETMSG(addr, msg, 1);
	SEND(TOMSG(WORKERCANHELP), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(FORGET_ABOUT_ME));
	ASSERTMSG(check, msg, checks[0]);
	ASSERTMSG(addr, msg, 1);

	SETMSG(pkgname, msg, "foo");
	SETMSG(version, msg, "0.0.1");
	SETMSG(arch, msg, pkg_archs_str[ARCH_X86_64_MUSL]);
	SETMSG(check, msg, checks[0]);
	SETMSG(addr, msg, 100);
	SEND(TOMSG(WORKERCANHELP), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(FORGET_ABOUT_ME));
	ASSERTMSG(check, msg, checks[0]);
	ASSERTMSG(addr, msg, 100);

	for (i = 1; i >= 0; i--) {
		SETMSG(pkgname, msg, "foo");
		SETMSG(version, msg, "0.0.1");
		SETMSG(arch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		SETMSG(check, msg, checks[i]);
		SETMSG(addr, msg, i);
		SEND(TOMSG(WORKERCANHELP), grphr);
		GET(msg, wrkr[i]);
		ASSERTMSG(id, msg, TOMSG(WORKERCANHELP));
		ASSERTMSGSTR(pkgname, msg, "foo");
		ASSERTMSGSTR(version, msg, "0.0.1");
		ASSERTMSGSTR(arch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		ASSERTMSG(check, msg, 0);
		ASSERTMSG(addr, msg, 0);
		GET(msg, storage);
		ASSERTMSG(id, msg, TOMSG(RESETLOG));
		ASSERTMSGSTR(pkgname, msg, "foo");
		ASSERTMSGSTR(version, msg, "0.0.1");
		ASSERTMSGSTR(arch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		while (arc4random() % 100 != 0) {
			uint32_t chunkdata = arc4random();
			chunk = zchunk_new(&chunkdata, sizeof(chunkdata));
			zchunk_t *tmpchunk = zchunk_dup(chunk);
			SETMSG(logs, msg, &tmpchunk);
			SEND(TOMSG(LOGHERE), wrkr[i]);
			GET(msg, storage);
			assert(*(zchunk_data(pkggraph_msg_logs(msg))) ==
					*(zchunk_data(chunk)));
			GET(msg, wrkr[i]);
			ASSERTMSG(id, msg, TOMSG(ROGER));
		}
	}

	for (i = 0; i < 2; i++) {
		SETMSG(pkgname, msg, "foo");
		SETMSG(version, msg, "0.0.1");
		SETMSG(arch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		SETMSG(check, msg, 0);
		SETMSG(addr, msg, 0);
		SETMSG(cause, msg, 0);
		SEND(TOMSG(JOB_ENDED), wrkr[i]);
	}

	for (i = 0; i < 2; i++) {
		GET(msg, grphr);
		ASSERTMSGSTR(pkgname, msg, "foo");
		ASSERTMSGSTR(version, msg, "0.0.1");
		ASSERTMSGSTR(arch, msg, pkg_archs_str[ARCH_X86_64 + i]);
		ASSERTMSG(check, msg, checks[i]);
		ASSERTMSG(addr, msg, i);
		ASSERTMSG(cause, msg, 0);
	}

	/* Work over, let's clean up. */

	pkggraph_msg_destroy(&msg);

	zsock_destroy(&storage);
	zsock_destroy(&grphr);
	zsock_destroy(&wrkr[0]);
	zsock_destroy(&wrkr[1]);

	return 0;
}

int
main(int argc, char * const *argv)
{
	(void) argc;
	char *tmppath = strdup("/tmp/dxpb-harness-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	assert(ourdir);

	const char *endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "sock"), NULL, NULL);
	const char *pubpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pub"), NULL, NULL);
	const char *ssldir = "/var/empty/";

	int rc;
	rc = bfs_ensure_sock_perms(endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(pubpoint);
	assert(rc == ERR_CODE_OK);

	prologue(argv[0]);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff(endpoint, pubpoint, ssldir);
	return run(endpoint, pubpoint, ssldir);
}

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
#include "pkgimport_msg.h"

#include "dxpb-common.h"

void
forkoff(const char *path, const char *endpoint, const char *pubpoint)
{
	switch(fork()) {
	case 0:
		execlp(path, "dxpb-pkgimport-master", "-P",
				"https://github.com/dxpb/packages.git",
				"-i", endpoint, "-I", pubpoint, NULL);
		exit(0);
	case -1:
		exit(ERR_CODE_BAD);
	default:
		return;
	}
}

int
run(const char *endpoint, const char *pubpoint)
{
	assert(endpoint);
	assert(pubpoint);
	int rc;

	pkgimport_msg_t *msg = pkgimport_msg_new();

	zsock_t *grphr  = zsock_new(ZMQ_DEALER);
	zsock_t *wrkr1  = zsock_new(ZMQ_DEALER);
	zsock_t *wrkr2  = zsock_new(ZMQ_DEALER);
	assert(grphr);
	assert(wrkr1);
	assert(wrkr2);

	rc = zsock_attach(grphr, endpoint, false);
	assert(rc == 0);
	rc = zsock_attach(wrkr1, endpoint, false);
	assert(rc == 0);
	rc = zsock_attach(wrkr2, endpoint, false);
	assert(rc == 0);

#define SEND(this, sock)	{ \
					pkgimport_msg_set_id(msg, this); \
					rc = pkgimport_msg_send(msg, sock); \
					assert(rc == 0); \
				}

#define GET(mymsg, sock)	{ \
					zpoller_t *p = zpoller_new(sock, NULL); \
					(void) zpoller_wait(p, 5*60*1000); \
					assert(!zpoller_expired(p)); \
					if (zpoller_terminated(p)) \
						exit(-1); \
					rc = pkgimport_msg_recv(mymsg, sock); \
					assert(rc == 0); \
					zpoller_destroy(&p); \
				}


#define TOMSG(str)		     PKGIMPORT_MSG_##str
#define SETMSG(what, msg, to) 	     pkgimport_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkgimport_msg_##what(msg) == eq)
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkgimport_msg_##what(msg), eq) == 0)

	/* And now we get to work */

	{ /* Say hi */
		SEND(TOMSG(HELLO), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(ROGER));

		SEND(TOMSG(IAMTHEGRAPHER), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(ROGER));

		SEND(TOMSG(HELLO), wrkr1);
		SEND(TOMSG(HELLO), wrkr2);
		GET(msg, wrkr1);
		ASSERTMSG(id, msg, TOMSG(ROGER));
		GET(msg, wrkr2);
		ASSERTMSG(id, msg, TOMSG(ROGER));
	}

	{ /* Say I'm ready to work */
		SEND(TOMSG(RDY2WRK), wrkr1);
		GET(msg, wrkr1);
		ASSERTMSG(id, msg, TOMSG(ROGER));
	}

	{ /* Confirm stable status, then mess it up */
		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(STABLE));

		SETMSG(commithash, msg, "8b25df976e413843a354fdf3cc9946fef1008816");
		SEND(TOMSG(WESEEHASH), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(ROGER));

		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(UNSTABLE));
	}

	{ /* Get work. Then be unreliable */
		GET(msg, wrkr1);
		ASSERTMSG(id, msg, TOMSG(PLZREAD));
		ASSERTMSGSTR(pkgname, msg, "ipfs-cluster");
		SEND(TOMSG(RUREADY), wrkr1);
		GET(msg, wrkr1);
		ASSERTMSG(id, msg, TOMSG(READY));
	}

	{ /* Wait for at least two minutes, avoid timeouts */
		for (int i = 0; i < 200; i += 20) {
			sleep(20);
			SEND(TOMSG(STABLESTATUSPLZ), grphr);
			GET(msg, grphr);
			ASSERTMSG(id, msg, TOMSG(UNSTABLE));
			SEND(TOMSG(PING), wrkr2);
			GET(msg, wrkr2);
			ASSERTMSG(id, msg, TOMSG(ROGER));
		}
	}

	{ /* Get work, we'll do it. Ensure stability is correct during work. */
		SEND(TOMSG(RDY2WRK), wrkr2);
		GET(msg, wrkr2);
		ASSERTMSG(id, msg, TOMSG(ROGER));

		/* Stable? No. */
		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(UNSTABLE));

		GET(msg, wrkr2);
		ASSERTMSG(id, msg, TOMSG(PLZREAD));
		ASSERTMSGSTR(pkgname, msg, "ipfs-cluster");
		/* This following call-response broken up by stability check */
		SEND(TOMSG(RUREADY), wrkr2);

		/* Stable? No. */
		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(UNSTABLE));

		GET(msg, wrkr2);
		ASSERTMSG(id, msg, TOMSG(READY));
		/* Call-response done */

		/* Stable? No. */
		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(UNSTABLE));
	}

	{ /* Do the work */
		SETMSG(pkgname, msg, "ipfs-cluster");
		SEND(TOMSG(PKGDEL), wrkr2);

		/* Graphr gets told according to protocol */
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(PKGDEL));
		ASSERTMSGSTR(pkgname, msg, "ipfs-cluster");

		/* Stable? No. */
		SEND(TOMSG(STABLESTATUSPLZ), grphr);
		GET(msg, grphr);
		ASSERTMSG(id, msg, TOMSG(UNSTABLE));

		/* Now I'm actually done with the work */
		SEND(TOMSG(DIDREAD), wrkr2);
		GET(msg, wrkr2);
		ASSERTMSG(id, msg, TOMSG(ROGER));
	}

	/* Stable? Yes! */
	SEND(TOMSG(STABLESTATUSPLZ), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(STABLE));
	ASSERTMSGSTR(commithash, msg, "1f52b9d972598b40ec1e024f22169cde7ea29ad8");


	/* Work over, let's clean up. */

	pkgimport_msg_destroy(&msg);

	zsock_destroy(&grphr);
	zsock_destroy(&wrkr1);
	zsock_destroy(&wrkr2);

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

	char *endpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "sock"), NULL, NULL);
	char *pubpoint = bstring_add(bstring_add(NULL, "ipc://", NULL, NULL), bfs_new_tmpsock(ourdir, "pub"), NULL, NULL);

	char *execpath = NULL;
	char cwd[1024];
	char *tmp;
	tmp = getcwd(cwd, 1023);
	assert(tmp == cwd);
	execpath = bstring_add(bstring_add(NULL, cwd, NULL, NULL), "/dxpb-pkgimport-master", NULL, NULL);
	printf("Path: %s\n", execpath);

	int rc;
	rc = bfs_ensure_sock_perms(endpoint);
	assert(rc == ERR_CODE_OK);
	rc = bfs_ensure_sock_perms(pubpoint);
	assert(rc == ERR_CODE_OK);

	rc = mkdir(repopath, S_IRWXU);

	prologue(argv[0]);

	rc = chdir(repopath);
	assert(rc == 0);

	puts("\n\nThis is a test harness.\nConducting tests....\n\n");

	forkoff(execpath, endpoint, pubpoint);
	return run(endpoint, pubpoint);
}

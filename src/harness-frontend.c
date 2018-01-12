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
	int rc;

	pkggraph_msg_t *msg = pkggraph_msg_new();

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

	SEND(TOMSG(PING), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(IFORGOTU));

	SEND(TOMSG(HELLO), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IAMSTORAGE), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(PING), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(HELLO), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(IAMSTORAGE), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	SEND(TOMSG(PING), grphr);
	GET(msg, grphr);
	ASSERTMSG(id, msg, TOMSG(ROGER));

	/* Work over, let's clean up. */

	pkggraph_msg_destroy(&msg);

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

/*
 * shim-builder.c
 *
 * DXPB package worker - test shim for interactive use
 */

#define _POSIX_C_SOURCE 200809L

#include <czmq.h>
#include <sys/stat.h>
#include "dxpb.h"
#include "bstring.h"
#include "bfs.h"
#include "pkggraph_msg.h"
#include "pkggraph_worker.h"

#include "dxpb-common.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *
iotime(void *insock)
{
	zsock_t *sock = insock;
	char pkgname[80], version[80], arch[80];
	pthread_mutex_lock(&mutex);
	pthread_mutex_unlock(&mutex);
	while(1) {
		puts("*****************");
		printf("Enter a pkgname: ");
		(void)scanf("%s", pkgname);
		printf("Enter a version: ");
		(void)scanf("%s", version);
		printf("Enter an arch: ");
		(void)scanf("%s", arch);
		puts("*****************");
		zsock_send(sock, "sss", pkgname, version, arch);
	}
}

void
forkoff(const char *endpoint, const char *hostdir, const char *masterdir,
		const char *repopath, const char *workspec, const char *ssldir,
		const char *varrundir, zsock_t *sock)
{
	pthread_t *t = calloc(1, sizeof(pthread_t));
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
		pthread_create(t, NULL, iotime, sock);
		return;
	}
}

int
run(zsock_t *graph, const char *hostdir, const char *masterdir,
		const char *repopath, const char *ssldir,
		const char *varrundir, zsock_t *othersock)
{
	(void)varrundir;
	assert(hostdir);
	assert(masterdir);
	assert(repopath);
	assert(ssldir);

	char *pkgname = NULL;
	char *version = NULL;
	char *arch = NULL;

	int rc;

	pkggraph_msg_t *msg = pkggraph_msg_new();

#define SEND(this, msg, sock)        { \
					pkggraph_msg_set_id(msg, TOMSG(this)); \
					rc = pkggraph_msg_send(msg, sock); \
					assert(rc == 0); \
				}

#define TOMSG(str)                   PKGGRAPH_MSG_##str
#define SETMSG(what, msg, to)        pkggraph_msg_set_##what(msg, to)
#define ASSERTMSG(what, msg, eq)     assert(pkggraph_msg_##what(msg) == eq)
#define ASSERTMSGID(msg, eq)		printf("msg_id: %d\n", pkggraph_msg_id(msg)); ASSERTMSG(id, msg, TOMSG(eq));
#define ASSERTMSGSTR(what, msg, eq)     assert(strcmp(pkggraph_msg_##what(msg), eq) == 0)

#define GET(mymsg, sock, othersock) GETTO(mymsg, sock, othersock, 30*1000);
#define GETTO(mymsg, sock, othersock, timeout)        { \
					zpoller_t *p = zpoller_new(sock, othersock, NULL); \
					zsock_t *tmpsock = zpoller_wait(p, timeout); \
					assert(!zpoller_expired(p)); \
					if (zpoller_terminated(p)) \
						exit(-1); \
					if (tmpsock == sock) rc = pkggraph_msg_recv(mymsg, sock); \
					if (tmpsock == othersock) { \
						(void)zsock_recv(othersock, "sss", &pkgname, &version, &arch); \
						puts("GETTO!"); \
						SETMSG(pkgname, mymsg, pkgname); \
						SETMSG(version, mymsg, version); \
						SETMSG(arch, mymsg, arch); \
						SEND(WORKERCANHELP, mymsg, sock); \
						char a = 0; \
						zsock_send(othersock, "1", a);\
					} \
					assert(rc == 0); \
					zpoller_destroy(&p); \
				}

	/* And now we get to work */

	puts("First GET");
	GETTO(msg, graph, othersock, -1);
	puts("First ASSERTMSG");
	ASSERTMSGID(msg, HELLO);
	puts("First SEND");
	SEND(ROGER, msg, graph);

	puts("Second GET");
	GET(msg, graph, othersock);
	ASSERTMSGID(msg, IMAWORKER);

	puts("After second GET");

	do {
		SEND(ROGER, msg, graph);
		puts("Sent a roger!");
		GET(msg, graph, othersock);
	} while (pkggraph_msg_id(msg) == TOMSG(PING));
	fprintf(stderr, "Now you can use the builder, it's bootstrapped.\n");
	ASSERTMSGID(msg, ICANHELP);
	SEND(ROGER, msg, graph);

	pthread_mutex_unlock(&mutex);

	do {
		GET(msg, graph, othersock);
		if (pkggraph_msg_id(msg) == TOMSG(PING))
			SEND(ROGER, msg, graph);
	} while (1);

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

	char *endpoint = "tcp://127.0.0.1:43512";

	if (argc < 2 || argv[1][0] == '-') {
		printf("Usage: one argument which is the -W argument to the builder\n");
		exit(1);
	}

	char *workspec = argv[1];

	zsock_t *sock1, *sock2;
	sock1 = zsock_new_push("inproc://builder_tasks");
	sock2 = zsock_new_pull("inproc://builder_tasks");

	pthread_mutex_lock(&mutex);

	int rc;

	rc = mkdir(varrundir, S_IRWXU);
	assert(rc == 0);
	rc = mkdir(repopath, S_IRWXU);
	assert(rc == 0);
	rc = mkdir(ssldir, S_IRWXU);
	assert(rc == 0);

	prologue(argv[0]);

	zsock_t *graph = zsock_new_router(endpoint);
	assert(graph);

	forkoff(endpoint, hostdir, masterdir, repopath, workspec, ssldir, varrundir, sock1);

	return run(graph, hostdir, masterdir, repopath, ssldir, varrundir, sock2);
}

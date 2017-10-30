/*
 * bbuilder.c
 *
 * Build worker and process.
 * Don't mix with threads.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <czmq.h>
#include "bwords.h"
#include "bxpkg.h"
#include "bxsrc.h"
#include "bbuilder.h"
#include "dxpb.h"

// I use this variable only to enforce a single point of lookup for what gets
// sent over the socket. It gives no technical advantage over listing in-place.
// Ideally this would be optimized out and provide compile time errors
const char *bbuilder_actions_picture[] = {
	"1", // status
	"1", // status
	"1", // 0
	"1", // 0
	"sss1", // pkg ver arch iscross
	"1", // 0 until future usage
	"1", // 0 until future usage
	"sssc1", // pkg ver arch log more
	"1", // 0 until future usage
	"1", // failure_reason
	"1", // failure_reason
	NULL
};

/* in header file */
extern inline int bbuilder_send(zsock_t *, enum bbuilder_actions, uint8_t);

struct builder {
	int fds[2];
	char *name;
	char *ver;
	char *arch;
};

#define LOG_SIZE 1024*16
#define MAX_LOG_SIZE 1024*63

static enum ret_codes
bbuilder_handle_log_request(zsock_t *pipe, struct builder *bd)
{
	zframe_t *frame;
	int rc;
	uint8_t more = 0;
	int done = 0;
	zchunk_t *log;
	uint32_t log_size, total_size = 0;
	enum bbuilder_actions action = BBUILDER_LOG;
	do {
		log_size = MAX_LOG_SIZE - LOG_SIZE < total_size ?
				MAX_LOG_SIZE - total_size : LOG_SIZE;
		log = bxsrc_get_log(bd->fds, log_size, &done);
		more = zchunk_size(log) == log_size;
		assert(!(more && done));
		total_size += zchunk_size(log);
		frame = zframe_new(&action, sizeof(action));
		zframe_send(&frame, pipe, ZMQ_MORE);
		rc = zsock_bsend(pipe, bbuilder_actions_picture[action],
				log, &more);
		zchunk_destroy(&log);
		if (rc != 0)
			return ERR_CODE_SADSOCK;
	} while (more); // Done is automatically confirmed.

	if (done)
		return ERR_CODE_DONE;
	return ERR_CODE_OK;
}

static pid_t
bbuilder_handle_build(zsock_t *pipe, struct builder *bd, const char *masterdir,
		const char *hostdir, const char *xbps_src)
{
	pid_t retVal = 0;
	char *pkgname = NULL;
	char *version = NULL;
	char *checkver = NULL;
	enum pkg_archs arch;
	int rc;
	int iscross;

	rc = zsock_brecv(pipe, bbuilder_actions_picture[BBUILDER_BUILD],
			&pkgname, &version, &arch, &iscross);
	if (rc != 0)
		goto abort;

	/* Because git upstream changes sometimes, and we don't live
	 * in a lag-proof world
	 */
	checkver = bxsrc_pkg_version(xbps_src, pkgname);
	if (strcmp(version, checkver) != 0)
		goto abort;

	retVal = bxsrc_init_build(xbps_src, pkgname, bd->fds, masterdir,
			hostdir, iscross? arch : ARCH_NUM_MAX);

	goto end;
abort:
	if (pkgname)
		free(pkgname);
	if (version)
		free(version);
	if (checkver)
		free(checkver);
end:
	return retVal;
}

int
bbuilder_agent(zsock_t *pipe, char *masterdir, char *hostdir, char *xbps_src)
{
	zframe_t *frame;
	int rc;
	struct builder bd;
	pid_t srcinstance = 0;
	int quit = 0;

	while (!quit && (frame = zframe_recv(pipe)) && /* that is the blocking call */
			zframe_size(frame) == sizeof(enum bbuilder_actions) &&
			zsock_rcvmore(pipe)) {
		switch(*(zframe_data(frame))) {
		case BBUILDER_PING:
			bbuilder_send(pipe, BBUILDER_ROGER, 0);
			break;
		case BBUILDER_QUIT:
			quit = 1;
		case BBUILDER_STOP:
			if (srcinstance != 0) {
				(void)bxsrc_build_end(bd.fds, srcinstance);
				bbuilder_send(pipe, BBUILDER_ROGER, 0);
			}
			srcinstance = 0;
			break;
		case BBUILDER_BUILD:
			srcinstance = bbuilder_handle_build(pipe, &bd, masterdir,
					hostdir, xbps_src);
			if (srcinstance == 0)
				bbuilder_send(pipe, BBUILDER_NOT_BUILDING, 0);
			else
				bbuilder_send(pipe, BBUILDER_BUILDING, 0);
			break;
		case BBUILDER_GIVE_LOG:
			assert(srcinstance != 0);
			rc = bbuilder_handle_log_request(pipe, &bd);
			switch(rc) {
			case ERR_CODE_DONE:
				rc = bxsrc_build_end(bd.fds, srcinstance);
				if (rc == 0)
					bbuilder_send(pipe, BBUILDER_BUILT, 0);
				else
					bbuilder_send(pipe, BBUILDER_BUILD_FAIL, rc);
				break;
			default:
				break;
			}
			break;
		case BBUILDER_NOT_BUILDING:
		case BBUILDER_BUILD_FAIL:
		case BBUILDER_BUILT:
		case BBUILDER_LOG:
		case BBUILDER_BUILDING:
		default:
			fprintf(stderr, "Invalid message to worker\n");
			exit(ERR_CODE_BAD);
			break;
		}
		zframe_destroy(&frame);
	}
	return ERR_CODE_OK;
}

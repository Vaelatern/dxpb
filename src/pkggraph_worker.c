/*  =========================================================================
    pkggraph_worker - Package Builder - Where Work Gets Done

    =========================================================================
    */

/*
   @header
   Description of class for man page.
   @discuss
   Detailed discussion of the class, if any.
   @end
   */

#define _POSIX_C_SOURCE 200809L

#include "pkggraph_worker.h"
//  TODO: Change these to match your project's needs
#include "./pkggraph_msg.h"
#include "./pkggraph_worker.h"

#include "bbuilder.h"
#include "bworker_end_status.h"
#include "bgit.h"
#include "bwords.h"
#include "bxpkg.h"
#include "dxpb.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
	//  These properties must always be present in the client_t
	//  and are set by the generated engine. The cmdpipe gets
	//  messages sent to the actor; the msgpipe may be used for
	//  faster asynchronous message flows.
	zsock_t *cmdpipe;           //  Command pipe to/from caller API
	zsock_t *msgpipe;           //  Message pipe to/from caller API
	zsock_t *provided_pipe;     //  Do whatever the caller defines
	zsock_t *dealer;            //  Socket to talk to server
	pkggraph_msg_t *message;    //  Message to/from server
	client_args_t *args;        //  Arguments from methods

	//  Add specific properties for your application
	char *repopath;
	char *repourl;
	enum pkg_archs hostarch;
	enum pkg_archs targetarch;
	uint16_t cost;
	uint8_t iscross;
	int bootstrap_wanted : 1;
} client_t;

//  Include the generated client engine
#include "pkggraph_worker_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->bootstrap_wanted = 1; // Also set with flag_bootstrap_wanted
	self->repopath = NULL;
	self->repourl = NULL;
	self->hostarch = ARCH_NUM_MAX;
	self->targetarch = ARCH_NUM_MAX;
	self->iscross = 0;
	self->cost = 100;
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	FREE(self->repopath);
	FREE(self->repourl);
}

static int
handle_bootstrap_resp(zloop_t *loop, zsock_t *sock, void *arg)
{
	(void) loop;
	engine_handle_socket(arg, sock, NULL);
	zframe_t *frame = zframe_recv(sock);
	zsock_flush(sock);

	switch(*(zframe_data(frame))) {
	case BBUILDER_BOOTSTRAP_DONE:
		s_client_execute(arg, bootstrap_done_event);
		break;
	default:
		return -1;
	}
	zframe_destroy(&frame);
	return 0;
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkggraph_worker_test (bool verbose)
{
	printf (" * pkggraph_worker: ");
	if (verbose)
		printf ("\n");

	//  @selftest
	// TODO: fill this out
	//  pkggraph_worker_t *client = pkggraph_worker_new (NULL);
	//  pkggraph_worker_set_verbose(client, verbose);
	//  pkggraph_worker_destroy (&client);
	//  @end
	printf ("OK\n");
}

//  ---------------------------------------------------------------------------
//  connect_to_server
//

static void
connect_to_server (client_t *self)
{
	if (zsock_connect(self->dealer, "%s", self->args->endpoint) != 0) {
		engine_set_exception(self, connect_error_event);
		zsys_warning("could not connect to %s", self->args->endpoint);
		zsock_send(self->cmdpipe, "si", "FAILURE", -1, NULL);
	} else {
		zsys_debug("connected to %s", self->args->endpoint);
		zsock_send(self->cmdpipe, "si", "SUCCESS", 0, NULL);
		engine_set_connected(self, true);
	}
}

//  ---------------------------------------------------------------------------
//  complain_about_connection_error
//

static void
complain_about_connection_error (client_t *self)
{
	(void) self;
	fprintf(stderr, "Could not connect to pkggraph server\n");
}

//  ---------------------------------------------------------------------------
//  set_timeout_high
//

static void
set_timeout_high (client_t *self)
{
	engine_set_expiry(self, 10000);
}

//  ---------------------------------------------------------------------------
//  act_if_bootstrap_is_wanted
//

static void
act_if_bootstrap_is_wanted (client_t *self)
{
	if (self->bootstrap_wanted)
		do_bootstrap_update(self);
	else
		engine_set_next_event(self, bootstrap_done_event);
}

//  ---------------------------------------------------------------------------
//  begin_building_pkg
//

static void
begin_building_pkg (client_t *self)
{
	enum bbuilder_actions action = BBUILDER_BUILD;
	zframe_t *frame = zframe_new(&action, sizeof(action));
	zframe_send(&frame, self->provided_pipe, ZMQ_MORE);
	zsock_bsend(self->provided_pipe, bbuilder_actions_picture[action],
			pkggraph_msg_pkgname(self->message),
			pkggraph_msg_version(self->message),
			pkggraph_msg_arch(self->message),
			pkggraph_msg_iscross(self->message));

	frame = zframe_recv(self->provided_pipe);
	assert(zframe_size(frame) == sizeof(enum bbuilder_actions));
	assert(zsock_rcvmore(self->provided_pipe));
	zsock_flush(self->provided_pipe);
	switch(*(zframe_data(frame))) {
	case BBUILDER_NOT_BUILDING:
		pkggraph_msg_set_cause(self->message, END_STATUS_OBSOLETE);
		engine_set_next_event(self, job_ended_event);
		break;
	case BBUILDER_BUILDING:
		break;
	default:
		exit(ERR_CODE_BAD);
	}
}

//  ---------------------------------------------------------------------------
//  flag_bootstrap_wanted
//

static void
flag_bootstrap_wanted (client_t *self)
{
	self->bootstrap_wanted = 1;
}

//  ---------------------------------------------------------------------------
//  get_log_data
//

static void
get_log_data (client_t *self)
{
	enum bbuilder_actions action = BBUILDER_GIVE_LOG;
	zframe_t *frame = zframe_new(&action, sizeof(action));
	zframe_send(&frame, self->provided_pipe, ZMQ_MORE);
	zsock_bsend(self->provided_pipe, bbuilder_actions_picture[action], 0);
	uint8_t more = 0, reason = 0;
	zchunk_t *logs, *tmplogs;
	char *tmppkgname, *tmpversion, *tmparch;
	tmppkgname = tmpversion = tmparch = NULL;
	logs = zchunk_new(NULL, 0);

	do {
		frame = zframe_recv(self->provided_pipe);
		assert(zframe_size(frame) == sizeof(enum bbuilder_actions));
		assert(zsock_rcvmore(self->provided_pipe));

		action = *(zframe_data(frame));
		switch(*(zframe_data(frame))) {
		case BBUILDER_LOG:
			action = BBUILDER_GIVE_LOG;
			zsock_brecv(self->provided_pipe, "sssc1", &tmppkgname,
					&tmpversion, &tmparch, &tmplogs, &more);
			// must not free tmp* as they belong to zsock_brecv()
			zchunk_extend(logs, zchunk_data(tmplogs), zchunk_size(tmplogs));
			break;
		case BBUILDER_BUILD_FAIL:
			goto fallA;
		case BBUILDER_BUILT:
fallA:			zsock_brecv(self->provided_pipe, "sss1", &tmppkgname,
					&tmpversion, &tmparch, &reason);
			more = 0;
			pkggraph_msg_set_cause(self->message, reason);
			engine_set_next_event(self, job_ended_event);
			break;
		default:
			exit(ERR_CODE_BAD);
		}
	} while (more);

	pkggraph_msg_set_pkgname(self->message, tmppkgname);
	pkggraph_msg_set_version(self->message, tmpversion);
	pkggraph_msg_set_arch(self->message, tmparch);
	pkggraph_msg_set_logs(self->message, &logs);
}

//  ---------------------------------------------------------------------------
//  set_timeout_low
//

static void
set_timeout_low (client_t *self)
{
	// 2 seconds
	engine_set_expiry(self, 2000);
}

//  ---------------------------------------------------------------------------
//  do_git_ff
//

static void
do_git_ff (client_t *self)
{
	assert(self->repourl);
	assert(self->repopath);
	bgit_just_ff(self->repourl, self->repopath);
}

//  ---------------------------------------------------------------------------
//  cease_all_operations
//

static void
cease_all_operations (client_t *self)
{
	(void) self;
}

//  ---------------------------------------------------------------------------
//  do_bootstrap_update
//

static void
do_bootstrap_update (client_t *self)
{
	enum bbuilder_actions action = BBUILDER_BOOTSTRAP;
	zframe_t *frame = zframe_new(&action, sizeof(action));
	zframe_send(&frame, self->provided_pipe, ZMQ_MORE);
	zsock_bsend(self->provided_pipe, bbuilder_actions_picture[action],
			pkg_archs_str[self->hostarch]);

	engine_handle_socket(self, self->provided_pipe, handle_bootstrap_resp);

	self->bootstrap_wanted = 0;
}

//  ---------------------------------------------------------------------------
//  set_repopath_to_provided
//

static void
set_repopath_to_provided (client_t *self)
{
	assert(self);
	assert(self->args->repopath);
	self->repopath = strdup(self->args->repopath);
	assert(self->repopath);
	assert(self->args->repourl);
	self->repourl = strdup(self->args->repourl);
	assert(self->repourl);
	bgit_just_ff(self->repourl, self->repopath);
}

//  ---------------------------------------------------------------------------
//  prepare_icanhelp
//

static void
prepare_icanhelp (client_t *self)
{
	assert(self->hostarch < ARCH_HOST);
	assert(self->targetarch < ARCH_HOST);
	pkggraph_msg_set_hostarch(self->message, pkg_archs_str[self->hostarch]);
	pkggraph_msg_set_targetarch(self->message, pkg_archs_str[self->targetarch]);
	pkggraph_msg_set_iscross(self->message, self->iscross);
	pkggraph_msg_set_cost(self->message, self->cost);
	pkggraph_msg_set_addr(self->message, 0);
	pkggraph_msg_set_check(self->message, 0);
}

//  ---------------------------------------------------------------------------
//  set_build_params
//

static void
set_build_params (client_t *self)
{
	self->hostarch = bpkg_enum_lookup(self->args->hostarch);
	self->targetarch = bpkg_enum_lookup(self->args->targetarch);
	self->iscross = self->args->iscross;
	self->cost = self->args->cost;
	if (self->hostarch == ARCH_NUM_MAX || self->targetarch == ARCH_NUM_MAX)
		zsock_send(self->cmdpipe, "si", "STATUS", 0, NULL);
	else
		zsock_send(self->cmdpipe, "si", "STATUS", 1, NULL);
}


//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

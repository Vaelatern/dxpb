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
	int bootstrap_wanted;
	char *repopath;
	enum pkg_archs hostarch;
	enum pkg_archs targetarch;
	uint8_t iscross;
	uint16_t cost;
} client_t;

//  Include the generated client engine
#include "pkggraph_worker_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->bootstrap_wanted = 0;
	self->repopath = NULL;
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
	if (self->repopath)
		free(self->repopath);
	self->repopath = NULL;
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
	engine_set_expiry(self, 15000);
}


//  ---------------------------------------------------------------------------
//  act_if_bootstrap_is_wanted
//

static void
act_if_bootstrap_is_wanted (client_t *self)
{
	if (self->bootstrap_wanted)
		do_bootstrap_update(self);
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
	uint8_t more;
	zchunk_t *logs, *tmplogs;
	char *pkgname, *version, *arch;
	char *tmppkgname, *tmpversion, *tmparch;
	pkgname = version = arch = NULL;
	logs = zchunk_new(NULL, 0);

	do {
		frame = zframe_recv(self->provided_pipe);
		assert(zframe_size(frame) == sizeof(enum bbuilder_actions));
		assert(zsock_rcvmore(self->provided_pipe));
		assert(*(zframe_data(frame)) == BBUILDER_LOG);

		zsock_brecv(self->provided_pipe, "sssc1", &tmppkgname,
				&tmpversion, &tmparch, &tmplogs, &more);
		zchunk_extend(logs, zchunk_data(tmplogs), zchunk_size(tmplogs));
		if (!pkgname)
			pkgname = tmppkgname;
		else {
			assert(strcmp(pkgname, tmppkgname) == 0);
			free(tmppkgname);
		}
		tmppkgname = NULL;
		if (!version)
			version = tmpversion;
		else {
			assert(strcmp(version, tmpversion) == 0);
			free(tmpversion);
		}
		tmpversion = NULL;
		if (!arch)
			arch = tmparch;
		else {
			assert(strcmp(arch, tmparch) == 0);
			free(tmparch);
		}
		tmparch = NULL;
	} while (more);

	pkggraph_msg_set_pkgname(self->message, pkgname);
	pkggraph_msg_set_version(self->message, version);
	pkggraph_msg_set_arch(self->message, arch);
	pkggraph_msg_set_logs(self->message, &logs);

	free(pkgname);
	pkgname = NULL;
	free(version);
	version = NULL;
	free(arch);
	arch = NULL;
}


//  ---------------------------------------------------------------------------
//  set_timeout_low
//

static void
set_timeout_low (client_t *self)
{
	// 1 second
	engine_set_expiry(self, 1000);
}


//  ---------------------------------------------------------------------------
//  do_git_ff
//

static void
do_git_ff (client_t *self)
{
	bgit_just_ff(self->repopath);
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
	self->bootstrap_wanted = 0;
}


//  ---------------------------------------------------------------------------
//  set_repopath_to_provided
//

static void
set_repopath_to_provided (client_t *self)
{
	assert(self);
	self->repopath = strdup(self->args->repopath);
	assert(self->repopath);
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

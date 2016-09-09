/*  =========================================================================
    pkggraph_server - pkggraph_server

    =========================================================================
    */

/*
   @header
   Description of class for man page.
   @discuss
   Detailed discussion of the class, if any.
   @end
   */

#include "pkggraph_server.h"
//  TODO: Change these to match your project's needs
#include "../include/pkggraph_msg.h"
#include "../include/pkggraph_server.h"
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bworker.h"
#include "bworker_end_status.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

struct logchunk {
	char *name;
	char *ver;
	char *arch;
	zchunk_t *logs;
};

struct memo {
	int msgid;
	char *hostarch;
	char *targetarch;
	uint16_t addr;
	uint16_t cost;
	uint32_t check;
	uint8_t iscross;
	enum end_status cause;
	char *pkgname;
	char *version;
	char *arch;
};

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
	//  These properties must always be present in the server_t
	//  and are set by the generated engine; do not modify them!
	zsock_t *pipe;              //  Actor pipe back to caller
	zconfig_t *config;          //  Current loaded configuration

	//  Add any properties you need here
	zsock_t *pub;
	struct bworkgroup *workers;
	zlist_t *queued_logs;
	zlist_t *memos_to_grapher;
	client_t *grapher;
	client_t *storage;
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
	//  These properties must always be present in the client_t
	//  and are set by the generated engine; do not modify them!
	server_t *server;           //  Reference to parent server
	pkggraph_msg_t *message;    //  Message in and out

	//  TODO: Add specific properties for your application
	struct bworksubgroup *subgroup;
	zlist_t *assign_addrs;
	uint8_t worker;
	uint8_t num_logs_sent;
	uint8_t memos_handled;
};

//  Include the generated server engine
#include "pkggraph_server_engine.inc"

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
	//  Construct properties here
	self->queued_logs = zlist_new();
	self->memos_to_grapher = zlist_new();
	self->workers = bworker_group_new();
	self->grapher = NULL;
	self->storage = NULL;
	self->pub = NULL;
	return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
	//  Destroy properties here
	bworker_group_destroy(&(self->workers));
	zlist_destroy(&(self->memos_to_grapher));
	zlist_destroy(&(self->queued_logs));
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
	ZPROTO_UNUSED(self);
	ZPROTO_UNUSED(method);
	ZPROTO_UNUSED(msg);
	return NULL;
}

//  Apply new configuration.

static void
server_configuration (server_t *self, zconfig_t *config)
{
	ZPROTO_UNUSED(self);
	ZPROTO_UNUSED(config);
	//  Apply new configuration
}

//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
	self->subgroup = NULL;;
	self->worker = 0;
	self->assign_addrs = zlist_new();
	self->memos_handled = 0;
	return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
	if (self->subgroup)
		bworker_subgroup_destroy(&(self->subgroup));
	zlist_destroy(&(self->assign_addrs));
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkggraph_server_test (bool verbose)
{
	printf (" * pkggraph_server: ");
	if (verbose)
		printf ("\n");

	//  @selftest
	zactor_t *server = zactor_new (pkggraph_server, "server");
	if (verbose)
		zstr_send (server, "VERBOSE");
	zstr_sendx (server, "BIND", "ipc://@/pkggraph_server", NULL);

	zsock_t *client = zsock_new (ZMQ_DEALER);
	assert (client);
	zsock_set_rcvtimeo (client, 2000);
	zsock_connect (client, "ipc://@/pkggraph_server");

	//  TODO: fill this out
	pkggraph_msg_t *request = pkggraph_msg_new ();
	pkggraph_msg_destroy (&request);

	zsock_destroy (&client);
	zactor_destroy (&server);
	//  @end
	printf ("OK\n");
}

//  ---------------------------------------------------------------------------
//  confirm_no_grapher
//

static void
confirm_no_grapher (client_t *self)
{
	if (self->server->grapher)
		engine_set_exception(self, killmenow_event);
}

//  ---------------------------------------------------------------------------
//  register_grapher
//

static void
register_grapher (client_t *self)
{
	self->server->grapher = self;
}

//  ---------------------------------------------------------------------------
//  register_worker
//

static void
register_worker (client_t *self)
{
	self->worker = 1;
}

//  ---------------------------------------------------------------------------
//  confirm_no_storage
//

static void
confirm_no_storage (client_t *self)
{
	if (self->server->storage)
		engine_set_exception(self, killmenow_event);
}

//  ---------------------------------------------------------------------------
//  register_storage
//

static void
register_storage (client_t *self)
{
	self->server->storage = self;
}


//  ---------------------------------------------------------------------------
//  all_workers_should_bootstrap_update
//

static void
all_workers_should_bootstrap_update (client_t *self)
{
	engine_broadcast_event(self->server, self, been_ordered_to_update_bootstrap_event);
}


//  ---------------------------------------------------------------------------
//  register_worker_as_ready_to_help
//

static void
register_worker_as_ready_to_help (client_t *self)
{
	bworker_subgroup_insert(self->subgroup, pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message),
			bpkg_enum_lookup(pkggraph_msg_arch(self->message)),
			bpkg_enum_lookup(pkggraph_msg_hostarch(self->message)),
			pkggraph_msg_iscross(self->message),
			pkggraph_msg_cost(self->message));

	struct bworker *wrkr = bworker_from_remote_addr(self->server->workers,
			pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message));

	/* Handling for a possibly absent grapher */
	struct memo *memo = malloc(sizeof(struct memo));
	if (!memo) {
		perror("Can't even write a memo");
		exit(ERR_CODE_NOMEM);
	}
	memo->msgid = PKGGRAPH_MSG_ICANHELP;
	memo->addr = wrkr->addr;
	memo->check = wrkr->mycheck;
	memo->targetarch = pkg_archs_str[wrkr->arch];
	memo->hostarch = pkg_archs_str[wrkr->hostarch];
	memo->iscross = wrkr->iscross;
	memo->cost = wrkr->cost;
	zlist_append(self->server->memos_to_grapher, memo);

	wrkr = NULL; // Not destroying, just putting away
}


//  ---------------------------------------------------------------------------
//  notify_grapher_if_he_s_around
//

static void
notify_grapher_if_he_s_around (client_t *self)
{
	if (self->server->grapher)
		engine_send_event(self->server->grapher, you_ve_got_a_memo_event);
}


//  ---------------------------------------------------------------------------
//  act_on_job_return
//

static void
act_on_job_return (client_t *self)
{
	int failed = 1;
	char *reason;
	switch (pkggraph_msg_cause(self->message)) {
	case END_STATUS_OK:
		failed = 0;
		break;
	case END_STATUS_OBSOLETE:
		reason = "requested pkg version didn't match what builder expected";
		break;
	case END_STATUS_PKGFAIL:
		reason = "problem with package";
		break;
	case END_STATUS_NODISK:
		reason = "builder ran out of disk";
		break;
	case END_STATUS_NOMEM:
		reason = "builder ran out of memory";
		break;
	case END_STATUS_WRONG_ASSIGNMENT:
		reason = "dxpb fumbled the job assignments";
		break;
	default:
		fprintf(stderr, "Someone forgot to update a switch(){}...\n");
		exit(ERR_CODE_BAD);
	}
	if (failed)
		zstr_sendf(self->server->pub, "%s/%s/%s: build failed: %s",
				pkggraph_msg_pkgname(self->message),
				pkggraph_msg_version(self->message),
				pkggraph_msg_arch(self->message),
				reason);
	else
		zstr_sendf(self->server->pub, "%s/%s/%s: built successfully",
				pkggraph_msg_pkgname(self->message),
				pkggraph_msg_version(self->message),
				pkggraph_msg_arch(self->message));

	struct memo *memo = malloc(sizeof(struct memo));
	if (!memo) {
		perror("Can't even write a memo");
		exit(ERR_CODE_NOMEM);
	}
	memo->msgid = PKGGRAPH_MSG_JOB_ENDED;
	memo->addr = pkggraph_msg_addr(self->message);
	memo->check = pkggraph_msg_check(self->message);
	memo->cause = pkggraph_msg_cause(self->message);
	memo->pkgname= strdup(pkggraph_msg_pkgname(self->message));
	memo->version= strdup(pkggraph_msg_version(self->message));
	memo->arch = strdup(pkggraph_msg_arch(self->message));
	assert(memo->pkgname);
	assert(memo->version);
	assert(memo->arch);
	zlist_append(self->server->memos_to_grapher, memo);
	bworker_job_remove(bworker_from_remote_addr(self->server->workers,
				memo->addr, memo->check));
}


//  ---------------------------------------------------------------------------
//  route_log
//

static void
route_log (client_t *self)
{
	struct logchunk *tmp = malloc(sizeof(struct logchunk));
	tmp->name = strdup(pkggraph_msg_pkgname(self->message));
	tmp->ver = strdup(pkggraph_msg_version(self->message));
	tmp->arch = strdup(pkggraph_msg_arch(self->message));
	tmp->logs = pkggraph_msg_get_logs(self->message);

	zlist_append(self->server->queued_logs, tmp);

	if (self->server->storage) {
		engine_send_event(self->server->storage, send_the_log_event);
		self->server->storage->num_logs_sent = 0;
	}
}


//  ---------------------------------------------------------------------------
//  remove_self_as_grapher
//

static void
remove_self_as_grapher (client_t *self)
{
	assert(self->server->grapher == self);
	self->server->grapher = NULL;
}


//  ---------------------------------------------------------------------------
//  grab_a_log_chunk
//

static void
grab_a_log_chunk (client_t *self)
{
	assert(self->server->storage == self);

	struct logchunk *tmp = zlist_pop(self->server->queued_logs);
	pkggraph_msg_set_pkgname(self->message, tmp->name);
	free(tmp->name);
	pkggraph_msg_set_version(self->message, tmp->ver);
	free(tmp->ver);
	pkggraph_msg_set_arch(self->message, tmp->arch);
	free(tmp->arch);
	if (tmp->logs) {
		pkggraph_msg_set_logs(self->message, &(tmp->logs));
		engine_set_next_event(self, send_loghere_event);
	} else {
		engine_set_next_event(self, send_resetlog_event);
	}

	free(tmp);
}


//  ---------------------------------------------------------------------------
//  post_process_sending_log_chunk
//

static void
post_process_sending_log_chunk (client_t *self)
{
	self->num_logs_sent ++;

	if (zlist_size(self->server->queued_logs) > 0 && self->num_logs_sent < 5)
		engine_set_next_event(self, send_the_log_event);
	// else, give someone else a turn
	// This works because the reactor will process events in a tight loop.
}


//  ---------------------------------------------------------------------------
//  transform_worker_job_for_assignment
//

static void
transform_worker_job_for_assignment (client_t *self)
{
	const struct bworker *wrkr = zlist_pop(self->assign_addrs);
	pkggraph_msg_set_addr(self->message, wrkr->addr);
	pkggraph_msg_set_check(self->message, wrkr->check);
	pkggraph_msg_set_pkgname(self->message, wrkr->job.name);
	pkggraph_msg_set_version(self->message, wrkr->job.ver);
	pkggraph_msg_set_arch(self->message, pkg_archs_str[wrkr->job.arch]);
}


//  ---------------------------------------------------------------------------
//  confirm_worker_still_valid
//

static void
confirm_worker_still_valid (client_t *self)
{
	struct bworker *wrkr = bworker_from_my_addr(self->server->workers,
			pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message));
	if (!wrkr)
		engine_set_exception(self, tell_grapher_worker_gone_event);
}


//  ---------------------------------------------------------------------------
//  tell_the_worker_to_start_work
//

static void
tell_the_worker_to_start_work (client_t *self)
{
	struct bworker *wrkr = bworker_from_my_addr(self->server->workers,
			pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message));
	client_t *owner = bworker_owner_from_my_addr(self->server->workers,
			pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message));
	int rc = bworker_job_assign(wrkr, pkggraph_msg_pkgname(self->message),
			pkggraph_msg_version(self->message),
			bpkg_enum_lookup(pkggraph_msg_arch(self->message)));
	if (rc) {
		zlist_append(owner->assign_addrs, wrkr);
		engine_send_event(owner, you_can_help_with_this_event);
	} else {
		fprintf(stderr, "Couldn't assign a measly job\n");
		exit(ERR_CODE_BAD);
	}
}


//  ---------------------------------------------------------------------------
//  parse_memo
//

static void
parse_memo (client_t *self)
{
	assert(self->server->grapher == self);
	if (zlist_size(self->server->memos_to_grapher) == 0)
		return;

	struct memo *memo = zlist_pop(self->server->memos_to_grapher);
	switch(memo->msgid) {
	case PKGGRAPH_MSG_JOB_ENDED:
		pkggraph_msg_set_addr(self->message, memo->addr);
		pkggraph_msg_set_check(self->message, memo->check);
		pkggraph_msg_set_cause(self->message, memo->cause);
		pkggraph_msg_set_pkgname(self->message, memo->pkgname);
		pkggraph_msg_set_version(self->message, memo->version);
		pkggraph_msg_set_arch(self->message, memo->arch);
		engine_set_next_event(self, tell_grapher_job_ended_event);
		free(memo->pkgname);
		memo->pkgname = NULL;
		free(memo->version);
		memo->version = NULL;
		free(memo->arch);
		memo->arch = NULL;
		break;
	case PKGGRAPH_MSG_ICANHELP:
		pkggraph_msg_set_addr(self->message, memo->addr);
		pkggraph_msg_set_check(self->message, memo->check);
		pkggraph_msg_set_arch(self->message, memo->targetarch);
		pkggraph_msg_set_cost(self->message, memo->cost);
		pkggraph_msg_set_hostarch(self->message, memo->hostarch);
		pkggraph_msg_set_iscross(self->message, memo->iscross);
		engine_set_next_event(self, tell_grapher_worker_can_help_event);
		break;
	case PKGGRAPH_MSG_FORGET_ABOUT_ME:
		pkggraph_msg_set_addr(self->message, memo->addr);
		pkggraph_msg_set_check(self->message, memo->check);
		engine_set_next_event(self, tell_grapher_worker_gone_event);
		break;
	default:
		fprintf(stderr, "Invalid memo!\n");
		break;
	}
	free(memo);
	memo = NULL;
}


//  ---------------------------------------------------------------------------
//  set_event_if_more_memos
//

static void
set_event_if_more_memos (client_t *self)
{
	if (++self->memos_handled >= 5) {
		self->memos_handled = 0;
		return;
	}

	if (zlist_size(self->server->memos_to_grapher))
		engine_set_next_event(self, you_ve_got_a_memo_event);
	else
		self->memos_handled = 0;
}


//  ---------------------------------------------------------------------------
//  remove_all_workers
//

static void
remove_all_workers (client_t *self)
{
	bworker_subgroup_destroy(&self->subgroup);
}


//  ---------------------------------------------------------------------------
//  remove_worker
//

static void
remove_worker (client_t *self)
{
	struct bworker *wrkr = bworker_from_remote_addr(self->server->workers,
			pkggraph_msg_addr(self->message),
			pkggraph_msg_check(self->message));

	/* Handling for a possibly absent grapher */
	struct memo *memo = malloc(sizeof(struct memo));
	if (!memo) {
		perror("Can't even write a memo");
		exit(ERR_CODE_NOMEM);
	}
	memo->msgid = PKGGRAPH_MSG_FORGET_ABOUT_ME;
	memo->addr = wrkr->addr;
	memo->check = wrkr->mycheck;
	zlist_append(self->server->memos_to_grapher, memo);

	bworker_group_remove(wrkr);
}


//  ---------------------------------------------------------------------------
//  tell_logger_to_reset_log
//

static void
tell_logger_to_reset_log (client_t *self)
{
	struct logchunk *tmp = malloc(sizeof(struct logchunk));
	tmp->name = strdup(pkggraph_msg_pkgname(self->message));
	tmp->ver = strdup(pkggraph_msg_version(self->message));
	tmp->arch = strdup(pkggraph_msg_arch(self->message));
	tmp->logs = NULL;

	zlist_append(self->server->queued_logs, tmp);

	if (self->server->storage) {
		engine_send_event(self->server->storage, send_the_log_event);
		self->server->storage->num_logs_sent = 0;
	}
}


//  ---------------------------------------------------------------------------
//  establish_subgroup
//

static void
establish_subgroup (client_t *self)
{
	self->subgroup = bworker_subgroup_new(self->server->workers);
	self->subgroup->owner = self;
}

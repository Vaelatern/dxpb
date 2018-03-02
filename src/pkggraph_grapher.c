/*  =========================================================================
    pkggraph_grapher - Package Grapher

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

#include "pkggraph_grapher.h"
//  TODO: Change these to match your project's needs
#include "./pkggraph_msg.h"
#include "./pkggraph_grapher.h"
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

	//  TODO: Add specific properties for your application
	zlist_t *msgs_to_send;
	int send_update_bootstrap;
} client_t;

//  Include the generated client engine
#include "pkggraph_grapher_engine.inc"

struct message_to_send {
	int type;
	uint16_t addr;
	uint32_t check;
	char *pkgname;
	char *version;
	char *arch;
};

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->msgs_to_send = zlist_new();
	self->send_update_bootstrap = 0;
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	zlist_destroy(&(self->msgs_to_send));
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkggraph_grapher_test (bool verbose)
{
    printf (" * pkggraph_grapher: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkggraph_grapher_t *client = pkggraph_grapher_new ();
    pkggraph_grapher_set_verbose(client, verbose);
    pkggraph_grapher_destroy (&client);
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
//  tell_msgpipe_we_are_connected
//

static void
tell_msgpipe_we_are_connected (client_t *self)
{
	(void) self;
}

//  ---------------------------------------------------------------------------
//  send_message_verbatim_down_the_msgpipe
//

static void
send_message_verbatim_down_the_msgpipe (client_t *self)
{
	int rc;
	rc = pkggraph_msg_send(self->message, self->msgpipe);
	assert(rc == 0);
}


//  ---------------------------------------------------------------------------
//  set_expiry
//

static void
set_expiry (client_t *self)
{
	engine_set_expiry(self, 10000);
}


//  ---------------------------------------------------------------------------
//  trigger_send_saved_messages
//

static void
trigger_send_saved_messages (client_t *self)
{
	if (self->send_update_bootstrap) {
		engine_set_next_event(self, act_on_do_update_bootstrap_event);
		self->send_update_bootstrap = 0;
		return;
	}
	if (zlist_size(self->msgs_to_send) == 0)
		return;
	struct message_to_send *tosend = zlist_first(self->msgs_to_send);
	assert(tosend);
	switch(tosend->type) {
	case PKGGRAPH_MSG_WORKERCANHELP:
		engine_set_next_event(self, act_on_do_workercanhelp_event);
		break;
	default:
		fprintf(stderr, "Saved message is non-sensical\n");
		exit(ERR_CODE_BAD);
		break;
	}
}


//  ---------------------------------------------------------------------------
//  prepare_update_bootstrap_from_pipe
//

static void
prepare_update_bootstrap_from_pipe (client_t *self)
{
	(void) self;
}


//  ---------------------------------------------------------------------------
//  prepare_workercanhelp_from_pipe
//

static void
prepare_workercanhelp_from_pipe (client_t *self)
{
	struct message_to_send *tosend = zlist_pop(self->msgs_to_send);
	assert(tosend);
	pkggraph_msg_set_addr(self->message, tosend->addr);
	pkggraph_msg_set_check(self->message, tosend->check);
	pkggraph_msg_set_pkgname(self->message, tosend->pkgname);
	pkggraph_msg_set_version(self->message, tosend->version);
	pkggraph_msg_set_arch(self->message, tosend->arch);
	FREE(tosend->pkgname);
	FREE(tosend->version);
	FREE(tosend->arch);
	FREE(tosend);
}


//  ---------------------------------------------------------------------------
//  store_update_bootstrap_for_later_sending
//

static void
store_update_bootstrap_for_later_sending (client_t *self)
{
	self->send_update_bootstrap = 1;
}


//  ---------------------------------------------------------------------------
//  store_workercanhelp_for_later_sending
//

static void
store_workercanhelp_for_later_sending (client_t *self)
{
	struct message_to_send *tosave = malloc(sizeof(struct message_to_send));
	if (tosave == NULL) {
		fprintf(stderr, "Couldn't save a message, no memory\n");
		exit(ERR_CODE_NOMEM);
	}
	tosave->addr = self->args->addr;
	tosave->check = self->args->check;
	tosave->pkgname = strdup(self->args->pkgname);
	tosave->version = strdup(self->args->version);
	tosave->arch = strdup(self->args->arch);
	tosave->type = PKGGRAPH_MSG_WORKERCANHELP;
	assert(tosave->pkgname);
	assert(tosave->version);
	assert(tosave->arch);
	zlist_append(self->msgs_to_send, tosave);
}


//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

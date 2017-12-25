/*  =========================================================================
    pkgfiler_grapher - Package Grapher - File Manager Link

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

#include "pkgfiler_grapher.h"
//  TODO: Change these to match your project's needs
#include "./pkgfiles_msg.h"
#include "./pkgfiler_grapher.h"
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
    zsock_t *dealer;            //  Socket to talk to server
    zsock_t *provided_pipe;     //  Unused
    pkgfiles_msg_t *message;    //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  TODO: Add specific properties for your application
    zlist_t *msgs_to_send;
} client_t;

//  Include the generated client engine
#include "pkgfiler_grapher_engine.inc"

struct message_to_send {
	int type;
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
	engine_set_expiry(self, 15000);
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
pkgfiler_grapher_test (bool verbose)
{
    printf (" * pkgfiler_grapher: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkgfiler_grapher_t *client = pkgfiler_grapher_new ();
    pkgfiler_grapher_set_verbose(client, verbose);
    pkgfiler_grapher_destroy (&client);
    //  @end
    printf ("OK\n");
}

//  ---------------------------------------------------------------------------
//  connect_to_server
//

static void
connect_to_server (client_t *self)
{
	assert(self);
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
	assert(self);
	zstr_sendx(self->cmdpipe, "NOCONN", NULL);
}

//  ---------------------------------------------------------------------------
//  tell_msgpipe_we_have_a_pkg
//

static void
tell_msgpipe_we_have_a_pkg (client_t *self)
{
	int rc = pkgfiles_msg_send(self->message, self->msgpipe);
	assert(rc == 0);
}

//  ---------------------------------------------------------------------------
//  tell_msgpipe_we_do_not_have_a_pkg
//

static void
tell_msgpipe_we_do_not_have_a_pkg (client_t *self)
{
	int rc = pkgfiles_msg_send(self->message, self->msgpipe);
	assert(rc == 0);
}


//  ---------------------------------------------------------------------------
//  store_ispkghere_for_later_sending
//

static void
store_message_for_later_sending(client_t *self, int type)
{
	struct message_to_send *tosave = malloc(sizeof(struct message_to_send));
	if (tosave == NULL) {
		fprintf(stderr, "Couldn't save a message, no memory\n");
		exit(ERR_CODE_NOMEM);
	}
	tosave->type = type;
	tosave->pkgname = strdup(self->args->pkgname);
	switch(type) {
	case PKGFILES_MSG_PKGDEL:
		tosave->version = NULL;
		tosave->arch = NULL;
		break;
	case PKGFILES_MSG_ISPKGHERE:
		tosave->version = strdup(self->args->version);
		tosave->arch = strdup(self->args->arch);
		break;
	default:
		fprintf(stderr, "Message to save is non-sensical\n");
		exit(ERR_CODE_BAD);
		break;
	}
	zlist_append(self->msgs_to_send, tosave);
}

static void
store_ispkghere_for_later_sending (client_t *self)
{
	store_message_for_later_sending(self, PKGFILES_MSG_ISPKGHERE);
}


//  ---------------------------------------------------------------------------
//  store_pkgdel_for_later_sending
//  Only to be executed where we can't yet send messages.

static void
store_pkgdel_for_later_sending (client_t *self)
{
	store_message_for_later_sending(self, PKGFILES_MSG_PKGDEL);
}


//  ---------------------------------------------------------------------------
//  trigger_send_saved_messages
//

static void
trigger_send_saved_messages (client_t *self)
{
	if (zlist_size(self->msgs_to_send) == 0)
		return;
	struct message_to_send *tosend = zlist_first(self->msgs_to_send);
	assert(tosend);
	assert(tosend->type);
	switch(tosend->type) {
	case PKGFILES_MSG_PKGDEL:
		assert(tosend->pkgname);
		engine_set_next_event(self, act_on_do_pkgdel_event);
		break;
	case PKGFILES_MSG_ISPKGHERE:
		assert(tosend->pkgname);
		assert(tosend->version);
		assert(tosend->arch);
		engine_set_next_event(self, act_on_do_ispkghere_event);
		break;
	default:
		fprintf(stderr, "Saved message is non-sensical\n");
		exit(ERR_CODE_BAD);
		break;
	}
}


//  ---------------------------------------------------------------------------
//  prepare_ispkghere_from_pipe
//

static void
prepare_ispkghere_from_pipe (client_t *self)
{
	struct message_to_send *tosend = zlist_pop(self->msgs_to_send);
	assert(tosend);
	assert(tosend->type == PKGFILES_MSG_ISPKGHERE);
	assert(tosend->pkgname);
	assert(tosend->version);
	assert(tosend->arch);
	pkgfiles_msg_set_pkgname(self->message, tosend->pkgname);
	pkgfiles_msg_set_version(self->message, tosend->version);
	pkgfiles_msg_set_arch(self->message, tosend->arch);
	free(tosend->pkgname);
	tosend->pkgname = NULL;
	free(tosend->version);
	tosend->version = NULL;
	free(tosend->arch);
	tosend->arch = NULL;
	free(tosend);
	tosend = NULL;
}


//  ---------------------------------------------------------------------------
//  prepare_pkgdel_from_pipe
//

static void
prepare_pkgdel_from_pipe (client_t *self)
{
	struct message_to_send *tosend = zlist_pop(self->msgs_to_send);
	assert(tosend);
	assert(tosend->type == PKGFILES_MSG_PKGDEL);
	assert(tosend->pkgname);
	pkgfiles_msg_set_pkgname(self->message, tosend->pkgname);
	free(tosend->pkgname);
	tosend->pkgname = NULL;
	free(tosend);
	tosend = NULL;
}

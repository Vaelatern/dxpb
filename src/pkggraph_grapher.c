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
} client_t;

//  Include the generated client engine
#include "pkggraph_grapher_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	(void) self;
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	(void) self;
    //  Destroy properties here
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

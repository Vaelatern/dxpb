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

#include "pkgfiler_grapher.h"
//  TODO: Change these to match your project's needs
#include "./pkgfiles_msg.h"
#include "./pkgfiler_grapher.h"

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
    pkgfiles_msg_t *message;    //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  TODO: Add specific properties for your application
} client_t;

//  Include the generated client engine
#include "pkgfiler_grapher_engine.inc"

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

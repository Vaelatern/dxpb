/*  =========================================================================
    pkgimport_poke - Package Poker

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

#include "pkgimport_poke.h"
//  TODO: Change these to match your project's needs
#include "./pkgimport_msg.h"

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
    pkgimport_msg_t *message;   //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  TODO: Add specific properties for your application
} client_t;

//  Include the generated client engine
#include "pkgimport_poke_engine.inc"

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
pkgimport_poke_test (bool verbose)
{
    printf (" * pkgimport_poke: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkgimport_poke_t *client = pkgimport_poke_new ();
    pkgimport_poke_set_verbose(client, verbose);
    pkgimport_poke_destroy (&client);
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
	(void) self;
}

//  ---------------------------------------------------------------------------
//  time_to_die
//

static void
time_to_die (client_t *self)
{
	assert(self);
	zstr_sendx(self->cmdpipe, "KTHNKSBYE", NULL);
}


//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

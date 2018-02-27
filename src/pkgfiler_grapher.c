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

// Capped at max value for a uint8_t by the below source file.
#define MAX_PATIENTS 4

#include "pkgfiler_grapher.h"
//  TODO: Change these to match your project's needs
#include "./pkgfiles_msg.h"
#include "./pkgfiler_grapher.h"
#include "bstring.h"
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
    zlist_t *patients[MAX_PATIENTS]; // We will check in on their status.
    zhash_t *dellater; // Hash for fast lookup
    uint8_t pat_index;
} client_t;

//  Include the generated client engine
#include "pkgfiler_grapher_engine.inc"

struct message_to_send {
	int type;
	char *pkgname;
	char *version;
	char *arch;
};

static char *
msg2send2key(struct message_to_send *msg)
{
	char *rV = NULL;
	uint32_t parA, parB;
	parA = parB = 0;
	rV = bstring_add(rV, msg->pkgname, &parA, &parB);
	assert(rV);
	if (msg->type == PKGFILES_MSG_PKGDEL ||
			msg->type == PKGFILES_MSG_PKGISDEL)
		return rV;
	rV = bstring_add(rV, msg->version, &parA, &parB);
	rV = bstring_add(rV, msg->arch, &parA, &parB);
	assert(rV);
	return rV;
}

static char *
pkgmsg2key(pkgfiles_msg_t *msg)
{
	char *rV = NULL;
	uint32_t parA, parB;
	parA = parB = 0;
	rV = bstring_add(rV, pkgfiles_msg_pkgname(msg), &parA, &parB);
	assert(rV);
	if (pkgfiles_msg_id(msg) == PKGFILES_MSG_PKGDEL ||
			pkgfiles_msg_id(msg) == PKGFILES_MSG_PKGISDEL)
		return rV;
	rV = bstring_add(rV, pkgfiles_msg_version(msg), &parA, &parB);
	rV = bstring_add(rV, pkgfiles_msg_arch(msg), &parA, &parB);
	assert(rV);
	return rV;
}

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->msgs_to_send = zlist_new();
	self->dellater = zhash_new();
	self->pat_index = 0;
	for (uint8_t i = 0; i < MAX_PATIENTS; i++)
		self->patients[i] = zlist_new();
	engine_set_expiry(self, 10000);
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	zlist_destroy(&(self->msgs_to_send));
	for (uint8_t i = 0; i < MAX_PATIENTS; i++)
		zlist_destroy(&(self->patients[i]));
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
//  Waiting List Management
//

static void
add_pkg_to_waiting_lists(client_t *self, struct message_to_send *addthis)
{
	assert(self);
	assert(addthis);
	int index = (MAX_PATIENTS - 1) - self->pat_index;
	zlist_append(self->patients[index], addthis);
}

static void
remove_pkg_from_waiting_lists (client_t *self)
{ /* Need to be clever. Else we end up O(N^2). List appending is constant.
   * Looping through lists to remove any pkg... and then repeating every
   * list... and doing more than just pointer comparisons... that's slow.
   * Append to list of "We want to remove you" and work it out next sending
   * cycle
   */
	char *key = pkgmsg2key(self->message);
	zhash_insert(self->dellater, key, self);
	FREE(key);
}

//  ---------------------------------------------------------------------------
//  queue_old_patients
//

static void
queue_old_patients (client_t *self)
{
	char *key;
	uint32_t curloc = self->pat_index++;
	self->pat_index %= MAX_PATIENTS;
	zlist_t *curList = self->patients[curloc];
	struct message_to_send *cur = NULL;
	for (cur = zlist_first(curList); cur; cur = zlist_next(curList)) {
		key = msg2send2key(cur);
		if (zhash_lookup(self->dellater, key) != NULL)
			zlist_remove(curList, cur); // already got it!
		else {
			zlist_append(self->msgs_to_send, cur);
		}
		FREE(key);
	}
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
	add_pkg_to_waiting_lists(self, tosave);
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
}

//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

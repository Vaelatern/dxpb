/*  =========================================================================
    pkgimport_client - Package Importer

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

#include "pkgimport_client.h"
#include "pkgimport_msg.h"
#include "dxpb.h"
#include "bfs.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bpkg.h"

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
    zsock_t *provided_pipe;     //  Not used.
    pkgimport_msg_t *message;   //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  Add specific properties for your application
    struct pkg_importer *curjob;
    char *pkgname;
    char *xbps_src;
} client_t;

//  Include the generated client engine
#include "pkgimport_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	assert(self);
	 /* Every 10 seconds, expire. This has the practical effect of causing
	  * the expire state to be called if no other traffic is present. This
	  * has the effect of a keepalive.  Also prevents us from being marked
	  * obsolete just because we've had nothing to do.
	  * 2017-04-26
	  */
	engine_set_expiry(self, 10000);
	self->pkgname = NULL;
	self->xbps_src = NULL;
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	assert(self);
	if (self->pkgname)
		free(self->pkgname);
	self->pkgname = NULL;
	if (self->xbps_src)
		free(self->xbps_src);
	self->xbps_src = NULL;
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkgimport_client_test (bool verbose)
{
    printf (" * pkgimport_client: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkgimport_client_t *client = pkgimport_client_new ();
    pkgimport_client_set_verbose(client, verbose);
    pkgimport_client_destroy (&client);
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
		zsock_send (self->cmdpipe, "si", "FAILURE", -1, NULL);
	} else {
		zsys_debug("connected to %s", self->args->endpoint);
		bfs_ensure_sock_perms(self->args->endpoint);
		zsock_send (self->cmdpipe, "si", "SUCCESS", 0, NULL);
		engine_set_connected(self, true);
	}
}

//  ---------------------------------------------------------------------------
//  cease_all_operations
//

static void
cease_all_operations (client_t *self)
{
	assert(self);
	if (self->pkgname)
		free(self->pkgname);
	zsock_disconnect(self->dealer, "%s", self->args->endpoint);
	engine_set_connected(self, false);
}

//  ---------------------------------------------------------------------------
//  complain_about_connection_error
//

static void
complain_about_connection_error (client_t *self)
{
	assert(self);
}

//  ---------------------------------------------------------------------------
//  prepare_pkg_reading
//

static void
prepare_pkg_reading (client_t *self)
{
	assert(self);
	assert(self->xbps_src);
	const char *name = pkgimport_msg_pkgname(self->message);
	if (self->pkgname)
		free(self->pkgname);
	self->pkgname = strdup(name);
	if (self->pkgname == NULL) {
		perror("No memory for a little strdup");
		exit(ERR_CODE_NOMEM);
	}
	self->curjob = bpkg_read_begin(self->xbps_src, name);
}

//  ---------------------------------------------------------------------------
//  prepare_next_pkg
//

static void
prepare_next_pkg (client_t *self)
{
	if (self->curjob->actual_name == NULL) {
		pkgimport_msg_set_pkgname(self->message, self->pkgname);
		engine_set_next_event(self, no_pkg_found_event);
	} else if (self->curjob->toread[0].to_use) {
		pkgimport_msg_set_pkgname(self->message, self->curjob->toread[0].name);
		pkgimport_msg_set_version(self->message, self->curjob->toread[0].ver);
		pkgimport_msg_set_arch(self->message, pkg_archs_str[self->curjob->toread[0].arch]);
		pkgimport_msg_set_depname(self->message, self->curjob->toread[0].depname);
		pkgimport_msg_set_deparch(self->message, pkg_archs_str[self->curjob->toread[0].deparch]);
		self->curjob->toread[0].to_use = 0;
		engine_set_next_event(self, send_virtpkginfo_event);
	} else if (self->curjob->toread[1].to_use) {
		pkgimport_msg_set_pkgname(self->message, self->curjob->toread[1].name);
		pkgimport_msg_set_version(self->message, self->curjob->toread[1].ver);
		pkgimport_msg_set_arch(self->message, pkg_archs_str[self->curjob->toread[1].arch]);
		pkgimport_msg_set_depname(self->message, self->curjob->toread[1].depname);
		pkgimport_msg_set_deparch(self->message, pkg_archs_str[self->curjob->toread[1].deparch]);
		self->curjob->toread[1].to_use = 0;
		engine_set_next_event(self, send_virtpkginfo_event);
	} else {
		bpkg_read_step(self->xbps_src, self->curjob);
		if (self->curjob->to_send == NULL) {
			engine_set_exception(self, move_on_event);
			return;
		}
		pkgimport_msg_set_pkgname(self->message, self->curjob->to_send->name);
		pkgimport_msg_set_version(self->message, self->curjob->to_send->ver);
		pkgimport_msg_set_arch(self->message, pkg_archs_str[self->curjob->to_send->arch]);
		pkgimport_msg_set_crosshostneeds(self->message, bwords_to_units(
					self->curjob->to_send->wneeds_cross_host));
		pkgimport_msg_set_crosstargetneeds(self->message, bwords_to_units(
					self->curjob->to_send->wneeds_cross_target));
		pkgimport_msg_set_nativehostneeds(self->message, bwords_to_units(
					self->curjob->to_send->wneeds_native_host));
		pkgimport_msg_set_nativetargetneeds(self->message, bwords_to_units(
					self->curjob->to_send->wneeds_native_target));
		pkgimport_msg_set_cancross(self->message,
				self->curjob->to_send->can_cross);
		pkgimport_msg_set_provides(self->message, bwords_to_units(
					self->curjob->to_send->provides));
		pkgimport_msg_set_broken(self->message, self->curjob->to_send->broken);
		pkgimport_msg_set_bootstrap(self->message,
				self->curjob->to_send->bootstrap);
		pkgimport_msg_set_restricted(self->message,
				self->curjob->to_send->restricted);
		engine_set_next_event(self, send_pkginfo_event);
	}
}

//  ---------------------------------------------------------------------------
//  set_xbps_src_path
//

static void
set_xbps_src_path (client_t *self)
{
	if (self->xbps_src)
		free(self->xbps_src);
	self->xbps_src = strdup(self->args->xbps_src_path);
	if (!self->xbps_src)
		exit(ERR_CODE_NOMEM);
}

//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

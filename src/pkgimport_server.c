/*  =========================================================================
	pkgimport_server - pkgimport_server

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

#include "pkgimport_server.h"
#include "pkgimport_msg.h"
#include "dxpb.h"
#include "bgit.h"
#include "bfs.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bxsrc.h"
#include "blog.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
	//  These properties must always be present in the server_t
	//  and are set by the generated engine; do not modify them!
	zsock_t *pipe;			  //  Actor pipe back to caller
	zconfig_t *config;		  //  Current loaded configuration

	//  Add any properties you need here
	zsock_t *pub;
	char *pubpath;
	zlist_t *workers;
	zlist_t *toread;
	zlist_t *curjobs;
	zlist_t *tmppkgs;
	client_t *knowngrapher;
	char *curhash;
	char *repopath;
	char *repourl;
	char *xbps_src;
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
	//  These properties must always be present in the client_t
	//  and are set by the generated engine; do not modify them!
	server_t *server;		   //  Reference to parent server
	pkgimport_msg_t *message;   //  Message in and out

	//  Add specific properties for your application here
	char *curjob;
	unsigned int imaworker : 1;
	unsigned int imthegrapher : 1;
};

//  Include the generated server engine
#include "pkgimport_server_engine.inc"

// This is a temporary structure for routing pkg data
struct tmppkg {
	enum	jobtype {PKGINFO, PKGDEL} jobtype;
	char	*name;
	char	*arch;
	char	*version;
	char	*cross_host;
	char	*cross_trgt;
	char	*native_host;
	char	*native_trgt;
	unsigned        int can_cross : 1;
	unsigned        int broken : 1;
	unsigned        int bootstrap : 1;
	unsigned        int restricted : 1;
};

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
	self->pub = NULL;
	self->pubpath = NULL;
	self->curhash = NULL;
	self->knowngrapher = NULL;
	self->workers = zlist_new();
	self->toread = zlist_new();
	self->curjobs = zlist_new();
	self->tmppkgs = zlist_new();
	zlist_autofree(self->toread);
	zlist_autofree(self->curjobs);
	zlist_comparefn (self->toread, (zlist_compare_fn *) strcmp);
	zlist_comparefn (self->curjobs, (zlist_compare_fn *) strcmp);
	/* No processing should take more than 2 minutes */
	engine_configure(self, "server/timeout", "120000");
	return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
	if (self->pub)
		zsock_destroy(&(self->pub));
	if (self->pubpath)
		free(self->pubpath);
	self->pubpath = NULL;
	zlist_destroy(&(self->workers));
	zlist_destroy(&(self->toread));
	zlist_destroy(&(self->curjobs));
	zlist_destroy(&(self->tmppkgs));
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
	self->curjob = NULL;
	self->imaworker = 0;
	self->imthegrapher = 0;
	return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
	assert(self);
	if (self->imthegrapher) {
		self->imthegrapher = 0;
		self->server->knowngrapher = NULL;
	}
	if (self->imaworker) {
		self->imaworker = 0;
		zlist_remove(self->server->workers, self);
	}
	//  Destroy properties here
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkgimport_server_test (bool verbose)
{
	printf (" * pkgimport_server: ");
	if (verbose)
		printf ("\n");

	//  @selftest
	zactor_t *server = zactor_new (pkgimport_server, "server");
	if (verbose)
		zstr_send (server, "VERBOSE");
	zstr_sendx (server, "BIND", "ipc://@/pkgimport_server", NULL);

	zsock_t *client = zsock_new (ZMQ_DEALER);
	assert (client);
	zsock_set_rcvtimeo (client, 2000);
	zsock_connect (client, "ipc://@/pkgimport_server");

	//  TODO: fill this out
	pkgimport_msg_t *request = pkgimport_msg_new ();
	pkgimport_msg_destroy (&request);

	zsock_destroy (&client);
	zactor_destroy (&server);
	//  @end
	printf ("OK\n");
}

//  --------------------------------------------------------------------------
//  zlist_append_wrapper
//  Changes zlist_append's type signature to be reasonable for the package
//  callbacks.

static int
zlist_append_wrapper(void *passed, char *data)
{
	return zlist_append((zlist_t *)passed, (void *)data);
}

//  ---------------------------------------------------------------------------
//  add_worker_to_queue
//

static void
add_worker_to_queue (client_t *self)
{
	// This means the newest available worker is now at the front of the
	// list, for easy reference.
	self->imaworker = 1;
	zlist_push(self->server->workers, self);
}

//  ---------------------------------------------------------------------------
//  assign_pending_work
//

static void
assign_pending_work (client_t *self)
{
	if (zlist_size(self->server->toread) == 0) {
		return;
	}
	zlist_t *wrkrs = self->server->workers;
	zlist_t *queue = self->server->toread;
	zlist_t *actives = self->server->curjobs;
	client_t *curwrkr = zlist_first(wrkrs);
	client_t *canary = curwrkr;
	char *job;
	while (curwrkr != NULL && zlist_size(queue) > 0) {
		assert(curwrkr->imaworker);
		if (curwrkr->curjob == NULL) {
			zlist_push(actives, (job = zlist_pop(queue)));
			pkgimport_msg_set_pkgname(curwrkr->message, job);
			curwrkr->curjob = job;
			if (curwrkr == self)
				engine_set_next_event(self, ask_worker_to_read_event);
			else
				engine_send_event(curwrkr, ask_worker_to_read_event);
			if (self->server->pub) {
				zstr_sendm(self->server->pub, "TRACE");
				zstr_sendf(self->server->pub, "asking to read %s", job);
			}
		}
		curwrkr = zlist_next(wrkrs);
	}
	/* Make sure the list type is non destructive */
	assert(canary == zlist_first(wrkrs));
}

//  ---------------------------------------------------------------------------
//  pkg_read_finished
//

static void
pkg_read_finished (client_t *self)
{
	if (self->curjob != NULL)
		zlist_remove(self->server->curjobs, self->curjob);
	self->curjob = NULL;
}

//  ---------------------------------------------------------------------------
//  route_pkginfo
//

static void
route_pkginfo (client_t *self)
{
	pkgimport_msg_t *me_sage = self->message;
	struct tmppkg *tmp = calloc(1, sizeof(struct tmppkg));
	if (tmp == NULL) {
		perror("Couldn't even create a measly struct");
		exit(ERR_CODE_NOMEM);
	}
	tmp->jobtype = PKGINFO;
	tmp->name = strdup(pkgimport_msg_pkgname(me_sage));
	tmp->version = strdup(pkgimport_msg_version(me_sage));
	tmp->arch = strdup(pkgimport_msg_arch(me_sage));
	tmp->native_host = strdup(pkgimport_msg_nativehostneeds(me_sage));
	tmp->native_trgt = strdup(pkgimport_msg_nativetargetneeds(me_sage));
	tmp->cross_host = strdup(pkgimport_msg_crosshostneeds(me_sage));
	tmp->cross_trgt = strdup(pkgimport_msg_crosstargetneeds(me_sage));
	tmp->can_cross = pkgimport_msg_cancross(me_sage);
	tmp->broken = pkgimport_msg_broken(me_sage);
	tmp->bootstrap = pkgimport_msg_bootstrap(me_sage);
	tmp->restricted = pkgimport_msg_restricted(me_sage);
	if (!tmp->name || !tmp->name || !tmp->version || !tmp->arch ||
				!tmp->native_host || !tmp->native_trgt ||
				!tmp->cross_host || !tmp->cross_trgt) {
		perror("Had memory issues while creating temporary pkgs");
		exit(ERR_CODE_NOMEM);
	}

	blog_pkgImported(tmp->name, tmp->version, bpkg_enum_lookup(tmp->arch));

	zlist_append(self->server->tmppkgs, tmp);
	if (self->server->knowngrapher)
		engine_send_event(self->server->knowngrapher, process_pkgs_event);
}

//  ---------------------------------------------------------------------------
//  request_pkg_deletion
//

static void
request_pkg_deletion (client_t *self)
{
	struct tmppkg *tmp = calloc(1, sizeof(struct tmppkg));
	if (tmp == NULL) {
		perror("Couldn't even create a measly struct");
		exit(ERR_CODE_NOMEM);
	}
	tmp->jobtype = PKGDEL;
	tmp->name = strdup(pkgimport_msg_pkgname(self->message));
	if (tmp->name == NULL) {
		perror("Had memory issues while creating temporary pkgs");
		exit(ERR_CODE_NOMEM);
	}
	zlist_append(self->server->tmppkgs, tmp);
	if (self->server->knowngrapher)
		engine_send_event(self->server->knowngrapher, process_pkgs_event);

	blog_pkgImportedForDeletion(tmp->name);
}

//  ---------------------------------------------------------------------------
//  get_new_pkgs_from_git
//

static void
get_new_pkgs_from_git (client_t *self)
{
	self->imaworker = 0;
	if (self->server->curhash != NULL)
		free(self->server->curhash);
	self->server->curhash = NULL;
	bgit_proc_changed_pkgs(self->server->repopath, self->server->xbps_src,
			zlist_append_wrapper, (void *)self->server->toread);
	self->server->curhash = bgit_get_head_hash(self->server->repourl,
			self->server->repopath);
	if (self->server->pub) {
		zstr_sendm(self->server->pub, "TRACE");
		zstr_sendf(self->server->pub, "Updated pkgs from git");
	}
}

//  ---------------------------------------------------------------------------
//  return_job_to_queue
//

static void
return_job_to_queue (client_t *self)
{
	// Reasonable on timeout of git watcher
	if (self->imaworker == 0 || self->curjob == NULL) {
		return;
	}
	char *job = self->curjob;
	if (self->server->pub) {
		zstr_sendm(self->server->pub, "ERROR");
		zstr_sendf(self->server->pub, "Pkg reader timed out on pkg: %s", job);
	}
	zlist_push(self->server->toread, job);
	zlist_remove(self->server->curjobs, job);
	self->curjob = NULL;
	if (self->server->pub) {
		zstr_sendm(self->server->pub, "ERROR");
		zstr_sendf(self->server->pub, "Pkg reader timed out");
	}
}

//  ---------------------------------------------------------------------------
//  add_all_packages_to_the_queue
//

static void
add_all_packages_to_the_queue (client_t *self)
{
	bfs_srcpkgs_to_cb(self->server->repopath, zlist_append_wrapper, (void *)self->server->toread);
}

//  ---------------------------------------------------------------------------
//  respond_specifying_stability
//

static void
respond_specifying_stability (client_t *self)
{

	// DEAD CODE: Replace this with stats for scraping
	//		printf ("curjobs: %d\ttoread: %d\ttmppkgs: %d\n",
	//				zlist_size(self->server->curjobs),
	//				zlist_size(self->server->toread),
	//				zlist_size(self->server->tmppkgs));
	//

	if (zlist_size(self->server->curjobs) == 0 &&
			zlist_size(self->server->toread) == 0 &&
			zlist_size(self->server->tmppkgs) == 0) {
		engine_set_next_event(self, send_stable_event);
	} else {
		engine_set_next_event(self, send_unstable_event);
	}
}

//  ---------------------------------------------------------------------------
//  set_commithash
//

static void
set_commithash (client_t *self)
{
	pkgimport_msg_set_commithash(self->message, self->server->curhash);
	if (self->server->pub) {
		zstr_sendm(self->server->pub, "DEBUG");
		zstr_sendf(self->server->pub, "Commit hash now: %s", self->server->curhash);
	}
}

//  ---------------------------------------------------------------------------
//  set_grapher_bit
//

static void
set_grapher_bit (client_t *self)
{
	if (self->server->knowngrapher)
		engine_send_event(self->server->knowngrapher, terminate_event);
	self->imthegrapher = 1;
	self->server->knowngrapher = self;
}

//  ---------------------------------------------------------------------------
//  ensure_git_consistency
//  If git isn't at the correct hash, make it be at the correct hash.
//  If the hash changed, turnonthenews

static void
ensure_git_consistency (client_t *self)
{
	const char *hash = pkgimport_msg_commithash(self->message);
	if (strcmp(self->server->curhash, hash) == 0)
		return;
	else {
		int rc = bgit_checkout_hash(self->server->repopath, hash);
		if (rc == -2) {
			perror("Client has bad hash on record");
			if (self->server->pub) {
				zstr_sendm(self->server->pub, "ERROR");
				zstr_sendf(self->server->pub, "Bad hash on grapher record");
			}
		}
	}
}

//  ---------------------------------------------------------------------------
//  grab_tmppkg_and_process
//

static void
grab_tmppkg_and_process (client_t *self)
{
	struct tmppkg *tmp = zlist_pop(self->server->tmppkgs);
	if (tmp == NULL)
		return;
	pkgimport_msg_set_pkgname(self->message, tmp->name);
	if (tmp->jobtype == PKGDEL) {
		engine_set_next_event(self, send_myself_pkgdel_event); 
	} else if (tmp->jobtype == PKGINFO) {
		pkgimport_msg_set_version(self->message, tmp->version);
		pkgimport_msg_set_arch(self->message, tmp->arch);
		pkgimport_msg_set_nativehostneeds(self->message, tmp->native_host);
		pkgimport_msg_set_nativetargetneeds(self->message, tmp->native_trgt);
		pkgimport_msg_set_crosshostneeds(self->message, tmp->cross_host);
		pkgimport_msg_set_crosstargetneeds(self->message, tmp->cross_trgt);
		pkgimport_msg_set_cancross(self->message, tmp->can_cross);
		pkgimport_msg_set_broken(self->message, tmp->broken);
		pkgimport_msg_set_bootstrap(self->message, tmp->bootstrap);
		pkgimport_msg_set_restricted(self->message, tmp->restricted);
		engine_set_next_event(self, send_myself_pkginfo_event);
		free(tmp->version);
		free(tmp->arch);
		free(tmp->native_host);
		free(tmp->native_trgt);
		free(tmp->cross_host);
		free(tmp->cross_trgt);
	}
	free(tmp->name);
	free(tmp);
}

//  ---------------------------------------------------------------------------
//  ensure_all_configuration_is_complete
//

static void
ensure_all_configuration_is_complete (client_t *self)
{
	if (!self->server->xbps_src)
		self->server->xbps_src = zconfig_get(self->server->config, "dxpb/xbps_src", NULL);
	if (!self->server->repopath)
		self->server->repopath = zconfig_get(self->server->config, "dxpb/repopath", NULL);
	if (!self->server->repourl)
		self->server->repourl = zconfig_get(self->server->config, "dxpb/repourl", NULL);
	if (!self->server->pubpath) {
		self->server->pubpath = zconfig_get(self->server->config, "dxpb/pubpoint", NULL);
		self->server->pub = zsock_new_pub(self->server->pubpath);
		bfs_ensure_sock_perms(self->server->pubpath);
	}
	if (!self->server->xbps_src || !self->server->repopath || !self->server->repourl) {
		fprintf(stderr, "Caller neglected to set xbps_src, repourl, and repopath!\n");
		exit(ERR_CODE_BAD);
	}
	self->server->curhash = bgit_get_head_hash(self->server->repourl, self->server->repopath);

	bxsrc_run_dumb_bootstrap(self->server->xbps_src);
}

//  ---------------------------------------------------------------------------
//  remove_self_from_worker_list
//

static void
remove_self_from_worker_list (client_t *self)
{
	zlist_remove(self->server->workers, self);
}

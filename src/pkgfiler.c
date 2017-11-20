/*  =========================================================================
    pkgfiler - pkgfiler

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

#include "pkgfiler.h"
//  TODO: Change these to match your project's needs
#include "../include/pkgfiles_msg.h"
#include "../include/pkgfiler.h"

#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bxbps.h"
#include "bfs.h"
#include "bstring.h"


//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

char *
pkgdef_to_str(const char *name, const char *ver, const char *arch)
{
	assert(name);
	assert(ver);
	assert(arch);
	uint32_t parA = 0, parB = 0;
	return bstring_add(bstring_add(bstring_add(NULL, name, &parA, &parB),
				ver, &parA, &parB), arch, &parA, &parB);
}

char *
pkgmsg_to_str(pkgfiles_msg_t *message)
{
	return pkgdef_to_str(pkgfiles_msg_pkgname(message),
			pkgfiles_msg_version(message),
			pkgfiles_msg_arch(message));
}

struct hunt {
	client_t *asker;
	char *pkgname;
	char *version;
	char *arch;
	zlist_t *responded;
	zlist_t *pkg_here;
	zlist_t *resp_pending;
};

static struct hunt *
new_hunt(client_t *asker, const char *pkgname, const char *version, const char *arch)
{
	struct hunt *hunt = malloc(sizeof(struct hunt));
	hunt->asker = asker;
	hunt->pkgname = strdup(pkgname);
	hunt->version = strdup(version);
	hunt->arch = strdup(arch);
	hunt->responded = zlist_new();
	hunt->pkg_here = zlist_new();
	hunt->resp_pending = zlist_new();
	assert(hunt->pkgname);
	assert(hunt->version);
	assert(hunt->arch);
	assert(hunt->responded);
	assert(hunt->pkg_here);
	assert(hunt->resp_pending);
	return hunt;
}

static void
destroy_hunt(struct hunt **todie)
{
	struct hunt *hunt = *todie;
	free(hunt->pkgname);
	free(hunt->version);
	free(hunt->arch);
	zlist_destroy(&(hunt->responded));
	zlist_destroy(&(hunt->pkg_here));
	zlist_destroy(&(hunt->resp_pending));
	free(hunt);
	*todie = NULL;
}

struct hunt_atom {
	client_t *client;
	uint8_t start_pings;
};

struct filefetch {
	client_t *asker;
	client_t *provider;
	char *subpath;
	char *pkgname;
	char *version;
	char *arch;
	char *filepath;
	FILE *fp;
};

struct memo {
	client_t *client;
	pkgfiles_msg_t *msg;
};

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
	//  These properties must always be present in the server_t
	//  and are set by the generated engine; do not modify them!
	zsock_t *pipe;              //  Actor pipe back to caller
	zconfig_t *config;          //  Current loaded configuration

	//  Add any properties you need here
	zsock_t *pub;
	char *pubpath;
	struct hunt *curhunt;
	client_t *grapher;
	zhash_t *hunts;
	zlist_t *followups;
	zhash_t *peering; // list of struct filefetch *
	char *repodir;
	char *stagingdir;
	uint8_t max_fetch_slots;
	uint8_t available_fetch_slots;
	uint8_t wanted_fetch_slots;
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
	//  These properties must always be present in the client_t
	//  and are set by the generated engine; do not modify them!
	server_t *server;           //  Reference to parent server
	pkgfiles_msg_t *message;    //  Message in and out

	//  Add specific properties for your application
	uint8_t num_pings;
	zlist_t *refs;
	struct filefetch *curfetch;
	uint32_t numfetchs; // Safe reference counter due to no self-referencing
	uint8_t fetch_slots_inflight;
};

inline static uint32_t
get_max_inflight(client_t *self)
{
	return (self->server->max_fetch_slots / zhash_size(self->server->peering)) * self->numfetchs;
}

//  Include the generated server engine
#include "pkgfiler_engine.inc"

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
	self->pub = NULL;
	self->pubpath = NULL;
	self->available_fetch_slots = 50;
	self->followups = zlist_new();
	self->hunts = zhash_new();
	self->peering = zhash_new();
	self->repodir = NULL;
	self->stagingdir = NULL;
	//  Construct properties here
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
	zhash_destroy(&(self->hunts));
	zlist_destroy(&(self->followups));
	zhash_destroy(&(self->peering));
	//  Destroy properties here
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
	self->fetch_slots_inflight = 0;
	self->refs = zlist_new();
	self->numfetchs = 0;
	return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
	zlist_destroy(&(self->refs));
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkgfiler_test (bool verbose)
{
    printf (" * pkgfiler: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    zactor_t *server = zactor_new (pkgfiler, "server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/pkgfiler", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, "ipc://@/pkgfiler");

    //  TODO: fill this out
    pkgfiles_msg_t *request = pkgfiles_msg_new ();
    pkgfiles_msg_destroy (&request);

    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  check_for_pkg_locally
//

static void
check_for_pkg_locally (client_t *self)
{
	assert(self->server->repodir);
	char *pkgfile = bxbps_pkg_to_filename(
			pkgfiles_msg_pkgname(self->message),
			pkgfiles_msg_version(self->message),
			pkgfiles_msg_arch(self->message));
	int present = bfs_find_file_in_subdir(self->server->repodir, pkgfile, 0);
	if (present)
		engine_set_exception(self, pkg_here_event);
	free(pkgfile);
	pkgfile = NULL;

	if (self->server->pub) {
		zstr_sendm(self->server->pub, "TRACE");
		zstr_sendf(self->server->pub, "%s/%s/%s: looked for file and did%.*s find it",
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message),
				(!!(present))*3, "n't"); // 3 is strlen("n't")
	}
}


//  ---------------------------------------------------------------------------
//  broadcast_pkg_locate_request
//

static void
broadcast_pkg_locate_request (client_t *self)
{
	struct hunt *hunt = new_hunt(self, pkgfiles_msg_pkgname(self->message),
			pkgfiles_msg_version(self->message),
			pkgfiles_msg_arch(self->message));
	char *key = pkgmsg_to_str(self->message);
	zhash_insert(self->server->hunts, key, hunt);
	self->server->curhunt = hunt;
	engine_broadcast_event(self->server, self, trigger_job_hunt_event);
	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  delete_package
//

static void
delete_package (client_t *self)
{
	assert(self->server->stagingdir);
	char *filename = bxbps_pkg_to_filename(
			pkgfiles_msg_pkgname(self->message),
			pkgfiles_msg_version(self->message),
			pkgfiles_msg_arch(self->message));
	uint32_t parA, parB;
	parA = parB = 0;
	char *subdir;
	int rc = bfs_find_file_in_subdir(self->server->repodir, filename, &subdir);
	if (rc == 0)
		return; // Package doesn't exist to be deleted.
	char *delfilename = bstring_add(bstring_add(bstring_add(bstring_add(
						NULL, self->server->stagingdir,
						&parA, &parB),
					"/", &parA, &parB), filename,
				&parA, &parB),
			".del", &parA, &parB);
	bfs_touch(delfilename);
	free(delfilename);
	delfilename = NULL;
	if (self->server->pub) {
		zstr_sendm(self->server->pub, "TRACE");
		zstr_sendf(self->server->pub, "Deleting %s/%s/%s",
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
	}
}



//  ---------------------------------------------------------------------------
//  verify_this_remote_is_the_right_peer
//

static void
verify_this_remote_is_the_right_peer (client_t *self)
{
	self->fetch_slots_inflight--;
	self->server->available_fetch_slots++;

	if (!pkgfiles_msg_validchunk(self->message))
		return;
	char *key = pkgmsg_to_str(self->message);
	struct filefetch *fetch = zhash_lookup(self->server->peering, key);
	if (fetch->provider != self)
		engine_set_exception(self, invalid_event);
	if (!strcmp(fetch->subpath, pkgfiles_msg_subpath(self->message)))
		engine_set_exception(self, invalid_event);

	self->curfetch = fetch;
	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  write_file_chunk
//

static void
write_file_chunk (client_t *self)
{
	assert(self->server->stagingdir);
	if (!pkgfiles_msg_validchunk(self->message))
		return;
	int rc;
	struct filefetch *fetch = self->curfetch;
	zchunk_t *data = pkgfiles_msg_get_data(self->message);
	if (fetch->fp == NULL) {
		char *pkgfile = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
		// We don't need the below variable, since we already saved it,
		// but it would be nice to decide TODO: if we need to ensure
		// the subpaths line up with what we want, to detect bad peers.
		// const char *subpath = pkgfiles_msg_subpath(self->message);
		uint32_t parA, parB;
		char *filepath = bstring_add(NULL, self->server->stagingdir,
				&parA, &parB);
		if (filepath[parB] != '/')
			filepath = bstring_add(filepath, "/", &parA, &parB);
		if (fetch->subpath) {
			filepath = bstring_add(filepath, fetch->subpath, &parA, &parB);
		}
		if (filepath[parB] != '/')
			filepath = bstring_add(filepath, "/", &parA, &parB);
		filepath = bstring_add(filepath, pkgfile, &parA, &parB);
		fetch->fp = fopen(filepath, "wb");
		fetch->filepath = filepath;
		free(pkgfile);
		pkgfile = NULL;
	}
	rc = zchunk_write(data, fetch->fp);
	assert(rc == 0);
	zchunk_destroy(&data);

	if (self->server->pub) {
		zstr_sendm(self->server->pub, "DEBUG");
		zstr_sendf(self->server->pub, "Wrote chunk for %s/%s/%s",
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
	}
}


//  ---------------------------------------------------------------------------
//  obsolete_peer_agreement
//

static void
obsolete_peer_agreement (client_t *self)
{
	char *key = pkgmsg_to_str(self->message);
	self->numfetchs--;
	struct hunt *hunt = zhash_lookup(self->server->hunts, key);
	if (!hunt)
		engine_set_exception(self, invalid_event);

	struct filefetch *todie = zhash_lookup(self->server->peering, key);
	free(todie->pkgname);
	todie->pkgname = NULL;
	free(todie->version);
	todie->version = NULL;
	free(todie->arch);
	todie->arch = NULL;
	todie = NULL;
	zhash_freefn(self->server->peering, key, free);
	zhash_delete(self->server->peering, key);

	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  tell_asker_pkg_not_here
//

static void
tell_asker_pkg_not_here (client_t *self)
{
	engine_send_event(self->server->curhunt->asker, pkg_not_here_event);
}


//  ---------------------------------------------------------------------------
//  mark_pkg_at_remote_location
//

static void
mark_pkg_at_remote_location (client_t *self)
{
	char *key = pkgmsg_to_str(self->message);
	struct hunt *hunt = zhash_lookup(self->server->hunts, key);
	if (!hunt) {
		engine_set_exception(self, invalid_event);
		return;
	}
	zlist_append(hunt->pkg_here, self);
	zlist_append(hunt->responded, self);
	zlist_remove(hunt->resp_pending, self);
}


//  ---------------------------------------------------------------------------
//  establish_peer_agreement
//

static void
establish_peer_agreement (client_t *self)
{
	char *key = pkgmsg_to_str(self->message);
	struct hunt *hunt = zhash_lookup(self->server->hunts, key);
	if (!hunt) {
		engine_set_exception(self, invalid_event);
		goto end;
	}

	struct filefetch *peer = malloc(sizeof(struct filefetch));
	peer->provider = self;
	peer->asker = hunt->asker;
	peer->pkgname = strdup(hunt->pkgname);
	peer->version = strdup(hunt->version);
	peer->arch = strdup(hunt->arch);
	assert(peer->pkgname);
	assert(peer->version);
	assert(peer->arch);
	peer->fp = NULL;
	zhash_insert(self->server->peering, key, peer);

	self->curfetch = peer;
	self->numfetchs++;

end:
	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  act_if_all_remotes_returned_or_expired
//

static void
act_if_all_remotes_returned_or_expired (client_t *self)
{
	zlist_t *tmphunts = zlist_new();
	client_t *client = NULL;
	struct hunt *curhunt;
	struct hunt_atom *curatom;
	for (curhunt = zhash_first(self->server->hunts); curhunt;
			curhunt = zhash_next(self->server->hunts)) {
		if (zlist_size(curhunt->resp_pending) == 0) {
			zlist_append(tmphunts, curhunt);
		}
	}
	while ((curhunt = zlist_pop(tmphunts))) {
		struct memo *memo = calloc(1, sizeof(struct memo));
		pkgfiles_msg_set_pkgname(memo->msg, curhunt->pkgname);
		pkgfiles_msg_set_version(memo->msg, curhunt->version);
		pkgfiles_msg_set_arch(memo->msg, curhunt->arch);
		switch (zlist_size(curhunt->pkg_here)) {
		case 0:
			client = curhunt->asker;
			pkgfiles_msg_set_id(memo->msg, PKGFILES_MSG_PKGNOTHERE);
			break;
		default:
			/* We chose the source of the package */
			/* Most common for noarch subpackages */
			curatom = zlist_first(curhunt->pkg_here);
			client = curatom->client;
			pkgfiles_msg_set_id(memo->msg, PKGFILES_MSG_WANNASHARE_);
			break;
		}
		memo->client = client;
		zlist_append(self->server->followups, memo);
	}
}


//  ---------------------------------------------------------------------------
//  mark_pkg_not_at_remote_location
//

static void
mark_pkg_not_at_remote_location (client_t *self)
{
	char *key = pkgmsg_to_str(self->message);
	struct hunt *hunt = zhash_lookup(self->server->hunts, key);
	if (!hunt) {
		engine_set_exception(self, invalid_event);
		goto end;
	}
	assert(hunt->responded);
	assert(hunt->resp_pending);
	zlist_append(hunt->responded, self);
	zlist_remove(hunt->resp_pending, self);
end:
	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  mark_all_pkgs_not_at_this_remote_location
//

static void
mark_all_pkgs_not_at_this_remote_location (client_t *self)
{
	struct hunt_atom *togo = NULL;
	for (struct hunt *curhunt = zlist_first(self->refs); curhunt; curhunt = zlist_next(self->refs)) {
		for (struct hunt_atom *atom = zlist_first(curhunt->responded);
				atom; atom = zlist_next(curhunt->responded)) {
			if (atom->client == self)
				togo = atom;
		}
		zlist_remove(curhunt->responded, togo);
	}
	for (struct hunt *curhunt = zlist_first(self->refs); curhunt; curhunt = zlist_next(self->refs)) {
		for (struct hunt_atom *atom = zlist_first(curhunt->pkg_here);
				atom; atom = zlist_next(curhunt->pkg_here)) {
			if (atom->client == self)
				togo = atom;
		}
		zlist_remove(curhunt->pkg_here, togo);
	}
	for (struct hunt *curhunt = zlist_first(self->refs); curhunt; curhunt = zlist_next(self->refs)) {
		for (struct hunt_atom *atom = zlist_first(curhunt->resp_pending);
				atom; atom = zlist_next(curhunt->resp_pending)) {
			if (atom->client == self)
				togo = atom;
		}
		zlist_remove(curhunt->resp_pending, togo);
	}
}


//  ---------------------------------------------------------------------------
//  curhunt_to_my_own_message
//

static void
curhunt_to_my_own_message (client_t *self)
{
	pkgfiles_msg_set_pkgname(self->message, self->server->curhunt->pkgname);
	pkgfiles_msg_set_version(self->message, self->server->curhunt->version);
	pkgfiles_msg_set_arch(self->message, self->server->curhunt->arch);
}


//  ---------------------------------------------------------------------------
//  set_myself_response_pending
//

static void
set_myself_response_pending (client_t *self)
{
	struct hunt_atom *atom = malloc(sizeof(struct hunt_atom));
	atom->client = self;
	atom->start_pings = self->num_pings;
	zlist_append(self->refs, self->server->curhunt);
	zlist_append(self->server->curhunt->resp_pending, atom);
}


//  ---------------------------------------------------------------------------
//  end_hunt
//

static void
end_hunt (client_t *self)
{
	char *key = pkgmsg_to_str(self->message);
	assert(zhash_lookup(self->server->hunts, key));
	zhash_delete(self->server->hunts, key);
	destroy_hunt(&(self->server->curhunt));
	free(key);
	key = NULL;
}


//  ---------------------------------------------------------------------------
//  postprocess_chunk
//

static void
postprocess_chunk (client_t *self)
{
	if (!pkgfiles_msg_validchunk(self->message))
		return;

	if (pkgfiles_msg_eof(self->message)) {
		struct filefetch *fetch = self->curfetch;
		char *key = pkgmsg_to_str(self->message);
		char *filename = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
		int rc = fclose(fetch->fp);
		if (rc != 0) {
			perror("Error closing file - this should be handled properly");
			exit(ERR_CODE_BAD);
		}
		fetch->fp = NULL;

		uint32_t parA, parB;
		parA = parB = 0;
		char *newfilepath = bstring_add(NULL, self->server->repodir,
				&parA, &parB);
		if (newfilepath[parB] != '/')
			newfilepath = bstring_add(newfilepath, "/", &parA, &parB);
		if (fetch->subpath)
			newfilepath = bstring_add(newfilepath, fetch->subpath, &parA, &parB);
		if (newfilepath[parB] != '/')
			newfilepath = bstring_add(newfilepath, "/", &parA, &parB);
		newfilepath = bstring_add(newfilepath, filename, &parA, &parB);

		rc = bfs_rename(fetch->filepath, newfilepath);
		if (rc != 0) {
			perror("Error renaming file - this should be handled properly");
			exit(ERR_CODE_BAD);
		}

		fetch->filepath = bstring_add(fetch->filepath, ".new", NULL, NULL);
		bfs_touch(fetch->filepath);

		free(newfilepath);
		newfilepath = NULL;
		free(fetch->filepath);
		fetch->filepath = NULL;
		zhash_freefn(self->server->peering, key, free);
		zhash_delete(self->server->peering, key);
		self->curfetch = NULL;
		self->numfetchs--;
		free(key);
		key = NULL;
	}
}

//  ---------------------------------------------------------------------------
//  ask_for_more
//

static void
ask_for_more (client_t *self)
{
	if (!pkgfiles_msg_validchunk(self->message))
		return;

	if (self->curfetch == NULL)
		return;

	for (; get_max_inflight(self) > self->fetch_slots_inflight;
			self->fetch_slots_inflight++, self->server->available_fetch_slots--) {
		// noop
	}

}


//  ---------------------------------------------------------------------------
//  parse_memos
//

static void
parse_memos (client_t *self)
{
	struct memo *memo = zlist_pop(self->server->followups);
	if (memo && memo->client->message) {
		switch(pkgfiles_msg_id(memo->msg)) {
			case PKGFILES_MSG_PKGNOTHERE:
				engine_send_event(memo->client, pkg_not_here_event);
				break;
			case PKGFILES_MSG_WANNASHARE_:
				engine_send_event(memo->client, i_might_want_to_share_event);
				break;
		}
		pkgfiles_msg_set_pkgname(memo->client->message, pkgfiles_msg_pkgname(memo->msg));
		pkgfiles_msg_set_version(memo->client->message, pkgfiles_msg_version(memo->msg));
		pkgfiles_msg_set_arch(memo->client->message, pkgfiles_msg_arch(memo->msg));
		pkgfiles_msg_destroy(&(memo->msg));
	}
	if (memo)
		free(memo);
	memo = NULL;

	if (self->server->pub) {
		zstr_sendm(self->server->pub, "TRACE");
		zstr_sendf(self->server->pub, "Parsed a memo");
	}
}


//  ---------------------------------------------------------------------------
//  ensure_configuration_is_set
//

static void
ensure_configuration_is_set (client_t *self)
{
	if (!self->server->stagingdir)
		self->server->stagingdir = zconfig_get(self->server->config, "dxpb/stagingdir", NULL);
	if (!self->server->repodir)
		self->server->repodir = zconfig_get(self->server->config, "dxpb/repodir", NULL);
	if (!self->server->stagingdir || !self->server->repodir) {
		fprintf(stderr, "Caller neglected to set both staging and repo directory paths\n");
		exit(ERR_CODE_BAD);
	}
	if (!self->server->pubpath) {
		self->server->pubpath = zconfig_get(self->server->config, "dxpb/pubpoint", NULL);
		self->server->pub = zsock_new_pub(self->server->pubpath);
		bfs_ensure_sock_perms(self->server->pubpath);
	}
}

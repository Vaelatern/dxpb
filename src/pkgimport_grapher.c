/*  =========================================================================
    pkgimport_grapher - Package Grapher

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

#include <sqlite3.h>
#include <czmq.h>
#include "pkgimport_grapher.h"
#include "pkgimport_msg.h"
#include "pkggraph_msg.h"
#include "pkgfiles_msg.h"
#include "dxpb.h"
#include "bwords.h"
#include "bvirtpkg.h"
#include "bxpkg.h"
#include "bpkg.h"
#include "bgraph.h"
#include "bdb.h"
#include "bfs.h"
#include "bworker.h"
#include "bworkermatch.h"
#include "btranslate.h"
#include "blog.h"

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
	zsock_t *provided_pipe;     //  Not in use
	zsock_t *dealer;            //  Socket to talk to server
	pkgimport_msg_t *message;   //  Message to/from server
	client_args_t *args;        //  Arguments from methods

	//  Specific properties for this application
	zsock_t			*pub;
	char 			 hash[256];
	char			*dbpath;
	zlist_t			*nextup[ARCH_HOST];
	bgraph			 pkggraph;
	struct bworkgroup	*workers;
} client_t;

//  Include the generated client engine
#include "pkgimport_grapher_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	/* Every 15 seconds, expire. This has the practical effect of
	 * causing the expire state to be called if no other traffic is 
	 * present. This has the effect of a keepalive and prevents us from 
	 * being marked obsolete just because we've had nothing to do.
	 */
	self->pub = NULL;
	self->workers = bworker_group_new();
	engine_set_expiry(self, 15000);
	self->pkggraph = NULL;
	self->dbpath = NULL;
	self->hash[0] = '\0';
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	if (self->pub)
		zsock_destroy(&(self->pub));
	bworker_group_destroy(&(self->workers));
	if (self->dbpath)
		free(self->dbpath);
	self->dbpath = NULL;
	bgraph_destroy(&(self->pkggraph));
}

//  ---------------------------------------------------------------------------
//  Selftest

void
pkgimport_grapher_test (bool verbose)
{
    printf (" * pkgimport_grapher: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkgimport_grapher_t *client = pkgimport_grapher_new ();
    pkgimport_grapher_set_verbose(client, verbose);
    pkgimport_grapher_destroy (&client);
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
//  create_pkg_from_info
//

static void
create_pkg_from_info (client_t *self)
{
	assert(self);
	assert(self->pkggraph);
	int rc;
	rc = bgraph_insert_pkg(self->pkggraph, bpkg_read(self->message));
	assert(rc == 0); // only != 0 if top level graph is very broken.
	blog_pkgAddedToGraph(pkgimport_msg_pkgname(self->message),
				pkgimport_msg_version(self->message),
				bpkg_enum_lookup(pkgimport_msg_arch(self->message)));
}

//  ---------------------------------------------------------------------------
//  mark_pkg_for_deletion
//

static void
mark_pkg_for_deletion (client_t *self)
{
	pkgfiles_msg_t *msg;
	int rc;

	rc = btranslate_prepare_socket(self->msgpipe, TRANSLATE_FILES);
	if (rc != ERR_CODE_OK)
		return; // TODO: Probably take action here.

	msg = pkgfiles_msg_new();
	pkgfiles_msg_set_id(msg, PKGFILES_MSG_PKGDEL);
	pkgfiles_msg_set_pkgname(msg, pkgimport_msg_pkgname(self->message));
	rc = pkgfiles_msg_send(msg, self->msgpipe);
	if (rc != 0)
		return; // TODO: Perhaps take action here.
	if (self->pub) {
		zstr_sendm(self->pub, "DEBUG");
		zstr_sendf(self->pub, "Marked package for deletion: %s",
				pkgimport_msg_pkgname(self->message));
	}
	pkgfiles_msg_destroy(&msg);
}

//  ---------------------------------------------------------------------------
//  cease_all_operations
//

static void
cease_all_operations (client_t *self)
{
	ZPROTO_UNUSED(self);
}

//  ---------------------------------------------------------------------------
//  add_worker_to_list
//

static void
add_worker_to_list (client_t *self)
{
	enum pkg_archs targetarch, hostarch;

	targetarch = bpkg_enum_lookup(self->args->targetarch);
	if (strcmp(self->args->targetarch, self->args->hostarch) == 0)
		hostarch = targetarch;
	else
		hostarch = bpkg_enum_lookup(self->args->hostarch);

	if (bworker_group_insert(self->workers, self->args->addr, self->args->check,
				targetarch, hostarch, self->args->iscross,
				self->args->cost) == UINT16_MAX) {
		fprintf(stderr, "Failed to add a worker!\n");
		if (self->pub) {
			zstr_sendm(self->pub, "DEBUG");
			zstr_sendf(self->pub, "Failed to add worker to grapher");
		}
	} else // Success adding workers
		blog_workerAddedToGraphGroup(
				bworker_from_remote_addr(self->workers,
					self->args->addr,
					self->args->check));
}

//  ---------------------------------------------------------------------------
//  match_workers_to_packages
//  Goals: Be quick. Run this many times. Never break. Choose the most
//  important packages first, then make sure to get everybody you can.

static enum ret_codes
pkgimport_grapher_ask_worker_to_help(client_t *self, struct bworker *wrkr, struct pkg *pkg)
{
	assert(pkg->arch == wrkr->arch || pkg->arch == ARCH_NOARCH);
	pkggraph_msg_t *msg;
	int rc = ERR_CODE_OK;

	rc = btranslate_prepare_socket(self->msgpipe, TRANSLATE_GRAPH);
	if (rc != ERR_CODE_OK)
		return rc;

	msg = pkggraph_msg_new();
	pkggraph_msg_set_id(msg, PKGGRAPH_MSG_WORKERCANHELP);
	pkggraph_msg_set_addr(msg, wrkr->addr);
	pkggraph_msg_set_check(msg, wrkr->check);
	pkggraph_msg_set_pkgname(msg, pkg->name);
	pkggraph_msg_set_version(msg, pkg->ver);
	pkggraph_msg_set_arch(msg, pkg_archs_str[pkg->arch]);
	rc = pkggraph_msg_send(msg, self->msgpipe);
	if (rc != 0)
		rc = ERR_CODE_SADSOCK;
	if (rc == ERR_CODE_OK) {
		(void)bworker_job_assign(wrkr, pkg->name, pkg->ver, pkg->arch);
		pkg->status = PKG_STATUS_BUILDING;
		blog_workerAssigning(wrkr,
				pkggraph_msg_pkgname(msg),
				pkggraph_msg_version(msg),
				bpkg_enum_lookup(pkggraph_msg_arch(msg)));
	}
	pkggraph_msg_destroy(&msg);

	return rc;
}

static enum ret_codes
pkgimport_grapher_ask_around_for_pkg(client_t *self, struct pkg *pkg)
{
	pkgfiles_msg_t *msg;
	enum ret_codes ret;
	int rc;

	ret = btranslate_prepare_socket(self->msgpipe, TRANSLATE_FILES);
	if (ret != ERR_CODE_OK)
		return ret;

	msg = pkgfiles_msg_new();
	pkgfiles_msg_set_id(msg, PKGFILES_MSG_ISPKGHERE);
	pkgfiles_msg_set_pkgname(msg, pkg->name);
	pkgfiles_msg_set_version(msg, pkg->ver);
	pkgfiles_msg_set_arch(msg, pkg_archs_str[pkg->arch]);
	rc = pkgfiles_msg_send(msg, self->msgpipe);
	if (rc != 0)
		ret = ERR_CODE_SADSOCK;
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Asking filer for %s/%s/%s",
				pkgfiles_msg_pkgname(msg),
				pkgfiles_msg_version(msg),
				pkgfiles_msg_arch(msg));
	}
	pkgfiles_msg_destroy(&msg);

	return ret;

}

static enum ret_codes
pkgimport_grapher_ask_files_for_missing_pkgs(client_t *self)
{
	enum ret_codes retVal = ERR_CODE_OK;
	struct pkg *pkg;
	enum pkg_archs i;

	for (i = ARCH_NOARCH; i < ARCH_HOST; i++) {
		for (pkg = zlist_first(self->nextup[i]);
				pkg; // zlist_* returns NULL at end of list
				pkg = zlist_next(self->nextup[i])) {
			assert(pkg->status != PKG_STATUS_IN_REPO); // can't be nextup if in_repo
			if (pkg->status > PKG_STATUS_NONE)
				continue;
			if (pkg->arch == ARCH_TARGET)
				continue;

			retVal = pkgimport_grapher_ask_around_for_pkg(self, pkg);
			if (retVal == ERR_CODE_OK)
				pkg->status = PKG_STATUS_ASKING;
			else
				break;
		}
	}
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Asked files to confirm packages");
	}
	return retVal;
}

//  ---------------------------------------------------------------------------
//  write_graph_to_db
//

static void
write_graph_to_db (client_t *self)
{
	assert(self->dbpath);
	assert(self->hash);
	assert(self->pkggraph);
	if (self->pub) {
		zstr_sendm(self->pub, "DEBUG");
		zstr_sendf(self->pub, "Writing all packages to db");
	}
	int rc = bdb_write_all(self->dbpath, self->pkggraph, self->hash);
	if (rc != ERR_CODE_OK) {
		perror("rc != ERR_CODE_OK for writing packages to database");
		/* TODO: XXX: MUST HAVE a provision to work around this!
		     2017-05-02*/
	} else
		blog_graphSaved(self->hash);
}

//  ---------------------------------------------------------------------------
//  resolve_graph
//

static void
resolve_graph (client_t *self)
{
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Beginning graph resolution");
	}
	int rc = bgraph_attempt_resolution(self->pkggraph);
	if (rc != ERR_CODE_OK) {
		if (self->pub) {
			zstr_sendm(self->pub, "ERROR");
			zstr_sendf(self->pub, "Failed graph resolution");
		}
	}
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Ended graph resolution");
	}
}

//  ---------------------------------------------------------------------------
//  get_packages
//

static void
get_packages (client_t *self)
{
	for (enum pkg_archs i = ARCH_NOARCH; i < ARCH_HOST; i++) {
		if (self->nextup[i] != NULL)
			zlist_destroy(&(self->nextup[i]));
		self->nextup[i] = bgraph_what_next_for_arch(self->pkggraph, i);
		blog_queueSelected(self->nextup[i]);
	}
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Got packages");
	}
}

//  ---------------------------------------------------------------------------
//  store_hash
//

static void
store_hash (client_t *self)
{
	const char *tmp = pkgimport_msg_commithash(self->message);
	if (tmp != NULL)
		strncpy(self->hash, tmp, 255);
	else
		self->hash[0] = '\0';
	self->hash[255] = '\0'; // Ensure a null termination.
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Saved git hash");
	}
}

//  ---------------------------------------------------------------------------
//  read_all_from_db
//

static void
read_all_from_db (client_t *self)
{
	assert(self->dbpath);
	if (self->pkggraph != NULL)
		return;
	int rc;
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "About to read pkgs from the db");
	}
	struct bdb_bound_params *params = bdb_params_init_ro(self->dbpath);
	if (params->DO_NOT_USE_PARAMS) {
		self->hash[0] = '\0';
		self->pkggraph = bgraph_new();
	} else {
		char *tmp = bdb_read_hash(params);
		strncpy(self->hash, tmp != NULL ? tmp : "", 255);
		self->hash[255] = '\0';

		rc = bdb_read_pkgs_to_graph(&self->pkggraph, params);
		if (rc != ERR_CODE_OK) {
			; /* TODO: XXX: MUST HAVE a provision to work around
			     this!
			     2017-03-15*/
		}
	}
	bdb_params_destroy(&params);

	blog_graphRead(self->hash);
}

//  ---------------------------------------------------------------------------
//  decide_next_action_from_hash
//

static void
decide_next_action_from_hash (client_t *self)
{
	pkgimport_msg_set_commithash(self->message, self->hash);
	if (self->hash[0] == '\0')
		engine_set_next_event(self, no_hash_found_event);
	else
		engine_set_next_event(self, confirm_hash_event);
}

//  ---------------------------------------------------------------------------
//  remove_worker_from_list
//

static void
remove_worker_from_list (client_t *self)
{
	void *bworker = bworker_from_remote_addr(self->workers, self->args->addr, self->args->check);
	if (!bworker)
		return; // TODO: Take an action
	bworker_group_remove(bworker);
}

//  ---------------------------------------------------------------------------
//  set_expiry_low
//

static void
set_expiry_low (client_t *self)
{
	// 1.5 s
	engine_set_expiry(self, 1500);
}

//  ---------------------------------------------------------------------------
//  set_expiry_high
//

static void
set_expiry_high (client_t *self)
{
	// 15 seconds
	engine_set_expiry(self, 15000);
}

//  ---------------------------------------------------------------------------
//  note_pkg_not_yet_around
//

static void
note_pkg_not_yet_around (client_t *self)
{
	enum pkg_archs arch = bpkg_enum_lookup(self->args->arch);
	if (arch == ARCH_NUM_MAX)
		return; // TODO: Decide if an action is appropiate on bad input

	int rc = bgraph_mark_pkg_absent(self->pkggraph, self->args->pkgname,
			self->args->version, arch);

	if (rc != ERR_CODE_OK && rc != ERR_CODE_NO)
		return; // TODO: Decide if an action is appropiate.
}

//  ---------------------------------------------------------------------------
//  note_pkg_present
//  We can rely on the package of version and arch being around.

static void
note_pkg_present (client_t *self)
{
	enum pkg_archs arch = bpkg_enum_lookup(self->args->arch);
	if (arch == ARCH_NUM_MAX)
		return; // TODO: Decide if an action is appropiate on bad input

	int rc = bgraph_mark_pkg_present(self->pkggraph, self->args->pkgname,
			self->args->version, arch);

	if (rc != ERR_CODE_OK && rc != ERR_CODE_NO)
		return; // TODO: Decide if an action is appropiate.
}

//  ---------------------------------------------------------------------------
//  mark_pkg_bad
//  The package is not to be built again until the template is edited.

static void
mark_pkg_bad (client_t *self)
{
	enum pkg_archs arch = bpkg_enum_lookup(self->args->arch);
	if (arch == ARCH_NUM_MAX)
		return; // TODO: Decide if an action is appropiate on bad input

	int rc = bgraph_mark_pkg_bad(self->pkggraph, self->args->pkgname,
			self->args->version, arch);

	if (rc != ERR_CODE_OK && rc != ERR_CODE_NO)
		return; // TODO: Decide if an action is appropiate.

	if (rc != ERR_CODE_NO)
		blog_pkgMarkedUnbuildable(self->args->pkgname,
				self->args->version, arch);
}

//  ---------------------------------------------------------------------------
//  return_pkg_to_pending
//

static void
return_pkg_to_pending (client_t *self)
{
	struct bworker *wrkr = bworker_from_remote_addr(self->workers,
			self->args->addr, self->args->check);
	if (!wrkr)
		return;
	if (wrkr->job.name == NULL || wrkr->job.arch == ARCH_NUM_MAX)
		return;

	int rc = bgraph_mark_pkg_absent(self->pkggraph,
			wrkr->job.name, wrkr->job.ver, wrkr->job.arch);

	assert(rc == ERR_CODE_OK);
}

//  ---------------------------------------------------------------------------
//  ask_around_for_wanted_pkgs
//

static void
ask_around_for_wanted_pkgs (client_t *self)
{
	int rc = pkgimport_grapher_ask_files_for_missing_pkgs(self);
	if (rc != ERR_CODE_OK)
		return; // TODO: Decide if action here is appropiate
}

//  ---------------------------------------------------------------------------
//  match_workers_to_this_pkg
//

static void
match_workers_to_this_pkg (client_t *self)
{
	struct bworkermatch *matches;
	struct bworker *wrkr;
	struct pkg *pkg = bgraph_get_pkg(self->pkggraph, self->args->pkgname,
				self->args->version, bpkg_enum_lookup(self->args->arch));
	if (pkg == NULL)
		return; // TODO: We don't need a reaction here, perhaps. To be determined...

	assert(pkg->status == PKG_STATUS_TOBUILD);

	if ((matches = bworkermatch_grp_pkg(self->workers, self->pkggraph, pkg)) == NULL)
		return;
	if ((wrkr = bworkermatch_group_choose(matches)) == NULL) {
		fprintf(stderr, "Told of possible workers, but it was a lie\n");
		exit(ERR_CODE_BADDOBBY);
	}
	bworkermatch_destroy(&matches);

	if (pkgimport_grapher_ask_worker_to_help(self, wrkr, pkg) != ERR_CODE_OK)
		return; // TODO: Is a reaction appropiate?
}

//  ---------------------------------------------------------------------------
//  get_package_for_arch
//

static void
get_package_list_for_arch (client_t *self)
{
	enum pkg_archs i = bpkg_enum_lookup(self->args->arch);
	if (i == ARCH_NOARCH)
		get_packages(self);
	else {
		if (self->nextup[i] != NULL)
			zlist_destroy(&(self->nextup[i]));
		self->nextup[i] = bgraph_what_next_for_arch(self->pkggraph, i);
		blog_queueSelected(self->nextup[i]);
	}
}

//  ---------------------------------------------------------------------------
//  find_package_for_worker
//

static void
find_package_for_worker (client_t *self)
{
	struct bworker *wrkr = bworker_from_remote_addr(self->workers,
			self->args->addr, self->args->check);
	assert(wrkr);
	assert(wrkr->job.name == NULL);
	enum pkg_archs arch = wrkr->arch;
	struct pkg *pkg;
	int assigned = 0;
	bgraph pkgarch = NULL;
	bgraph hostarch = zhash_lookup(self->pkggraph, pkg_archs_str[wrkr->hostarch]);
tryagain:
	pkgarch = zhash_lookup(self->pkggraph, pkg_archs_str[arch]);
	for (pkg = zlist_first(self->nextup[arch]);
			pkg; // zlist_* returns NULL at end of list
			pkg = zlist_next(self->nextup[arch])) {
		if (pkg->status == PKG_STATUS_TOBUILD && bworkermatch_pkg(wrkr, pkgarch, hostarch, pkg)) {
			if (pkgimport_grapher_ask_worker_to_help(self, wrkr, pkg) != ERR_CODE_OK) {
				fprintf(stderr, "Failed to ask a worker to help!\n");
				return; // Possible an action should be taken
			} else {
				assigned = 1;
				break; // We found a package for this worker, now we let it work.
			}
		}
	}
	if (!assigned && arch != ARCH_NOARCH) {
		arch = ARCH_NOARCH;
		goto tryagain;
	}
}

//  ---------------------------------------------------------------------------
//  ask_around_for_this_pkg
//

static void
ask_around_for_this_pkg (client_t *self)
{
	struct pkg *pkg = bgraph_get_pkg(self->pkggraph, self->args->pkgname,
				self->args->version, bpkg_enum_lookup(self->args->arch));

	if (pkg == NULL)
		return; // TODO: We don't need a reaction here, perhaps. To be determined...

	if (pkg->status != PKG_STATUS_BUILDING)
		return; // TODO: Maybe a strong reaction

	if (pkgimport_grapher_ask_around_for_pkg(self, pkg) == ERR_CODE_OK)
		pkg->status = PKG_STATUS_ASKING; // Also this way we are always sure we get the package in the end.
	else
		return; // TODO: Is a reaction appropiate?
}

//  ---------------------------------------------------------------------------
//  set_db_path_as_supplied
//

static void
set_db_path_as_supplied (client_t *self)
{
	self->dbpath = strdup(self->args->dbpath);
	assert(self->dbpath);
}

//  ---------------------------------------------------------------------------
//  set_publish_endpoint_as_supplied
//

static void
set_publish_endpoint_as_supplied (client_t *self)
{
	if (self->pub)
		zsock_destroy(&(self->pub));
	self->pub = zsock_new_pub(self->args->pubpoint);
	bfs_ensure_sock_perms(self->args->pubpoint);
}

//  ---------------------------------------------------------------------------
//  set_ssl_client_keys
//

static void
set_ssl_client_keys (client_t *self)
{
        #include "set_ssl_client_keys.inc"
}

//  ---------------------------------------------------------------------------
//  record_virtualpkgs
//

static void
record_virtualpkgs (client_t *self)
{
	char *vpkgs_units = strdup(pkgimport_msg_virtualpkgs(self->message));
	bwords vpkgs = bwords_from_units(vpkgs_units);
	bgraph_set_vpkgs(self->pkggraph, bvirtpkg_from_words(vpkgs));
	bwords_destroy(&vpkgs, 1);
	FREE(vpkgs_units);
}

//  ---------------------------------------------------------------------------
//  create_virtpkg_from_info
//

static void
create_virtpkg_from_info (client_t *self)
{
	assert(self);
	assert(self->pkggraph);
	int rc;
	rc = bgraph_insert_pkg(self->pkggraph, bpkg_virt_read(self->message));
	assert(rc == 0); // only != 0 if top level graph is very broken.
	blog_pkgAddedToGraph(pkgimport_msg_pkgname(self->message),
				pkgimport_msg_version(self->message),
				bpkg_enum_lookup(pkgimport_msg_arch(self->message)));
}

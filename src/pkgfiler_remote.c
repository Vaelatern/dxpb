/*  =========================================================================
    pkgfiler_remote - Remote Hostdir Manager

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

#include "pkgfiler_remote.h"
//  TODO: Change these to match your project's needs
#include "./pkgfiles_msg.h"
#include "./pkgfiler_remote.h"

#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bgraph.h"
#include "bxbps.h"
#include "bfs.h"
#include "bstring.h"
#include "brepowatch.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

struct filefetch {
	char		*subpath;
	uint64_t	 pos;
	uint64_t	 eofpos;
	int		 fd;
};

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
    pkgfiles_msg_t *message;    //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  Add specific properties for your application
    zsock_t *finder;
    pthread_t *finder_thread;
    char *hostdir;
    zhash_t *open_fds;
    struct filefetch *curfetch;
} client_t;

//  Include the generated client engine
#include "pkgfiler_remote_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->hostdir = NULL;
	self->finder = NULL;
	self->finder_thread = NULL;
	self->open_fds = zhash_new();
	engine_set_expiry(self, 10000);
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	for (struct filefetch *fp = zhash_first(self->open_fds); fp;
			fp = zhash_next(self->open_fds)) {
		close(fp->fd);
		fp->fd = -1;
		free(fp);
	}
	zhash_destroy(&(self->open_fds));
	if (self->hostdir)
		free(self->hostdir);
	if (self->finder_thread != NULL) {
		zstr_send(self->finder, NULL);
		void *given = NULL;
		pthread_join(*(self->finder_thread), &given);
		zsock_destroy(&(self->finder));
	}
}


//  ---------------------------------------------------------------------------
//  Selftest

void
pkgfiler_remote_test (bool verbose)
{
    printf (" * pkgfiler_remote: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkgfiler_remote_t *client = pkgfiler_remote_new ();
    pkgfiler_remote_set_verbose(client, verbose);
    pkgfiler_remote_destroy (&client);
    //  @end
    printf ("OK\n");
}

//  ---------------------------------------------------------------------------
//  file_path_in_repo
//  A way to quickly get the relative path to a file in our hostdir

char *
file_path_in_repo(client_t *self, const char *filename)
{
	zstr_send(self->finder, filename);
	char *rV = zstr_recv(self->finder);
	if (rV[0] == '\0') {
		free(rV);
		rV = NULL;
	}
	return rV;
}

int
file_exists_in_repo(client_t *self, const char *filename)
{
	char *in = file_path_in_repo(self, filename);
	int rV = in[0] != '\0';
	free(in);
	in = NULL;
	return rV;
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
	ZPROTO_UNUSED(self);
}


//  ---------------------------------------------------------------------------
//  check_for_pkg_locally
//

static void
check_for_pkg_locally (client_t *self)
{
	assert(self->hostdir);
	char *pkgfile = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
	int present = file_exists_in_repo(self, pkgfile);
	if (present)
		pkgfiles_msg_set_id(self->message, PKGFILES_MSG_PKGHERE);
	else
		pkgfiles_msg_set_id(self->message, PKGFILES_MSG_PKGNOTHERE);
	pkgfiles_msg_send(self->message, self->dealer);
	free(pkgfile);
	pkgfile = NULL;
}


//  ---------------------------------------------------------------------------
//  open_file_for_sending
//

static void
open_file_for_sending (client_t *self)
{
	assert(self->hostdir);
	assert(self->open_fds);
	struct filefetch *fd = malloc(sizeof(struct filefetch));
	if (!fd) {
		fprintf(stderr, "Couldn't allocate memory for a simple file pointer\n");
		exit(ERR_CODE_NOMEM);
	}
	char *pkgfile = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
	fd->subpath = file_path_in_repo(self, pkgfile);
	assert(fd->subpath);
	char *fullpath = bstring_add(bstring_add(strdup(self->hostdir), fd->subpath,
				NULL, NULL), pkgfile, NULL, NULL);
	assert(fullpath);
	fd->fd = open(fullpath, O_RDONLY | O_NONBLOCK);
	fd->pos = 0;
	fd->eofpos = bfs_size(fd->fd);
	zhash_insert(self->open_fds, pkgfile, fd);
	free(fullpath);
	fullpath = NULL;
	free(pkgfile);
	pkgfile = NULL;
}

//  ---------------------------------------------------------------------------
//  pick_a_fetch
//

static void
pick_a_fetch (client_t *self)
{
	self->curfetch = zhash_first(self->open_fds);
}


//  ---------------------------------------------------------------------------
//  prepare_chunk
//

static void
prepare_chunk (client_t *self)
{
	if (!self->curfetch) {
		zchunk_t *chunk = zchunk_new(NULL, 0);
		pkgfiles_msg_set_validchunk(self->message, 0);
		pkgfiles_msg_set_data(self->message, &chunk);
		pkgfiles_msg_set_eof(self->message, 1);
		return;
	}

	assert(self->curfetch->pos <= self->curfetch->eofpos);
	zchunk_t *tosend;
	char *buf;
	int64_t size = 1024*64; // 64 kilobytes at a time;
	assert(self->curfetch->fd >= 0 || self->curfetch->pos == self->curfetch->eofpos);

	pkgfiles_msg_set_validchunk(self->message, 1);

	if (self->curfetch->fd >= 0) {
		buf = malloc(sizeof(char)*size);
		size = read(self->curfetch->fd, buf, size);
		tosend = zchunk_new(buf, size);

		pkgfiles_msg_set_position(self->message, self->curfetch->pos);
		pkgfiles_msg_set_data(self->message, &tosend);
		pkgfiles_msg_set_subpath(self->message, self->curfetch->subpath);
		self->curfetch->pos += size;
		pkgfiles_msg_set_eof(self->message,
					self->curfetch->pos == self->curfetch->eofpos);
		FREE(buf);
	}
	assert(size != 0 || self->curfetch->pos == self->curfetch->eofpos);
	assert(self->curfetch->pos <= self->curfetch->eofpos);
}


//  ---------------------------------------------------------------------------
//  postprocess_chunk
//

static void
postprocess_chunk (client_t *self)
{
	if (!self->curfetch)
		return;

	if (self->curfetch->fd >= 0 && self->curfetch->pos == self->curfetch->eofpos) {
		char *pkgfile = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
		close(self->curfetch->fd);
		self->curfetch->fd = -1;
		FREE(self->curfetch->subpath);
		zhash_delete(self->open_fds, pkgfile);
		FREE(pkgfile);
		FREE(self->curfetch);
	}
}


//  ---------------------------------------------------------------------------
//  confirm_pkg_is_local_and_want_to_share
//

static void
confirm_pkg_is_local_and_want_to_share (client_t *self)
{
	assert(self);
	assert(self->hostdir);
	char *pkgfile = bxbps_pkg_to_filename(
				pkgfiles_msg_pkgname(self->message),
				pkgfiles_msg_version(self->message),
				pkgfiles_msg_arch(self->message));
	int present = file_exists_in_repo(self, pkgfile);
	if (!present) {
		pkgfiles_msg_set_id(self->message, PKGFILES_MSG_IDONTWANNASHARE);
		pkgfiles_msg_send(self->message, self->dealer);
		engine_set_exception(self, NULL_event);
	}
	free(pkgfile);
	pkgfile = NULL;
}


//  ---------------------------------------------------------------------------
//  set_hostdir_location
//

static void
set_hostdir_location (client_t *self)
{
	if (self->hostdir)
		free(self->hostdir);
	self->hostdir = strdup(self->args->hostdir);
	if (!self->hostdir) {
		fprintf(stderr, "Couldn't set hostdir\n");
		exit(ERR_CODE_NOMEM);
	}
	if (self->finder_thread != NULL) {
		zstr_send(self->finder, NULL);
		void *given = NULL;
		pthread_join(*(self->finder_thread), &given);
		zsock_destroy(&(self->finder));
	}
	self->finder_thread = brepowatch_filefinder_prepare(&(self->finder),
			self->hostdir, "dxpb-hostdir-remote-finder");
}

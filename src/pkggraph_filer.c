/*  =========================================================================
    pkggraph_filer - Package Builder - Where Work Gets Done

    =========================================================================
*/

#define _POSIX_C_SOURCE 200809L

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include "pkggraph_filer.h"
//  TODO: Change these to match your project's needs
#include "./pkggraph_msg.h"
#include "./pkggraph_filer.h"
#include "dxpb.h"
#include "bfs.h"

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

    //  Add specific properties for your application
    zsock_t *pub;
    char *logdir;
    int logfd;
} client_t;

//  Include the generated client engine
#include "pkggraph_filer_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
	self->pub = NULL;
	self->logdir = NULL;
	self->logfd = -1;
	// 1.2 seconds
	engine_set_expiry(self, 1200);
	return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
	if (self->logdir)
		free(self->logdir);
	self->logdir = NULL;
	if (self->logfd >= 0)
		close(self->logfd);
	self->logfd = -1;
}


//  ---------------------------------------------------------------------------
//  Selftest

void
pkggraph_filer_test (bool verbose)
{
    printf (" * pkggraph_filer: ");
    if (verbose)
        printf ("\n");

    //  @selftest
    // TODO: fill this out
    pkggraph_filer_t *client = pkggraph_filer_new ();
    pkggraph_filer_set_verbose(client, verbose);
    pkggraph_filer_destroy (&client);
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
	ZPROTO_UNUSED(self);
}


//  ---------------------------------------------------------------------------
//  write_log_to_file
//

static void
write_log_to_file (client_t *self)
{
	assert(self->logdir);
	assert(self->logfd >= 0);

	int pkgdir = openat(self->logfd, pkggraph_msg_pkgname(self->message),
			O_RDWR | O_DIRECTORY | O_CREAT, 0755);
	if (pkgdir < 0) {
		perror("Couldn't open log directory for a package");
		exit(ERR_CODE_BADDIR);
	}

	int verdir = openat(pkgdir, pkggraph_msg_version(self->message),
			O_RDWR | O_DIRECTORY | O_CREAT, 0755);
	if (verdir < 0) {
		perror("Couldn't open log directory for a package's version");
		exit(ERR_CODE_BADDIR);
	}

	int destfd = openat(verdir, pkggraph_msg_arch(self->message),
			O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (destfd < 0) {
		perror("Couldn't open log file for a package's version and arch");
		exit(ERR_CODE_BADDIR);
	}

	// Don't handle failures except to crash hard
	zchunk_t *log = pkggraph_msg_logs(self->message);
	if (zchunk_size(log) >= SSIZE_MAX) {
		fprintf(stderr, "If you see this, please fix write_log_to_file.\n");
		fprintf(stderr, "Obviously the author did not anticipate log chunks larger than SSIZE_MAX\n");
		dprintf(destfd, "dxpb=> If you see this, please fix write_log_to_file.\n");
		dprintf(destfd, "dxpb=> Obviously the author did not anticipate log chunks larger than SSIZE_MAX\n");
		if (self->pub) {
			zstr_sendm(self->pub, "DEBUG");
			zstr_sendf(self->pub, "Log chunk exceeded SSIZE_MAX: %s/%s/%s",
					pkggraph_msg_pkgname(self->message),
					pkggraph_msg_version(self->message),
					pkggraph_msg_arch(self->message));
		}
		goto close;
	}

	ssize_t written = write(destfd, zchunk_data(log), zchunk_size(log));
	// the size check is so we can do the typecast safer
	// yes I know we just did this ten lines up. If we have not, then
	// someone changed the code without paying attention.
	if (zchunk_size(log) >= SSIZE_MAX || written < (ssize_t)zchunk_size(log)) {
		perror("Couldn't write complete log");
		dprintf(destfd, "dxpb=> COuldn't write complete log\n");
		if (self->pub) {
			zstr_sendm(self->pub, "DEBUG");
			zstr_sendf(self->pub, "Could not write log: %s/%s/%s",
					pkggraph_msg_pkgname(self->message),
					pkggraph_msg_version(self->message),
					pkggraph_msg_arch(self->message));
		}
	}

	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Wrote log chunk for %s/%s/%s",
				pkggraph_msg_pkgname(self->message),
				pkggraph_msg_version(self->message),
				pkggraph_msg_arch(self->message));
	}

close:
	close(destfd);
	close(verdir);
	close(pkgdir);
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
//  set_logdir_to_provided
//

static void
set_logdir_to_provided (client_t *self)
{
	if (self->logdir)
		free(self->logdir);
	if (self->logfd >= 0)
		close(self->logfd);
	self->logdir = strdup(self->args->dir);
	if (!self->logdir) {
		perror("Couldn't dupe the logdir path");
		exit(ERR_CODE_NOMEM);
	}
	errno = 0;
	self->logfd = open(self->logdir, O_RDONLY | O_DIRECTORY, 0755);
	if (errno != 0) {
		perror("Tried to open the logdir and failed");
		exit(ERR_CODE_BADDIR);
	}
}


//  ---------------------------------------------------------------------------
//  truncate_log
//

static void
truncate_log (client_t *self)
{
	assert(self->logdir);
	assert(self->logfd >= 0);
	int pkgdir = openat(self->logfd, pkggraph_msg_pkgname(self->message),
			O_RDWR | O_DIRECTORY | O_CREAT, 0755);
	if (pkgdir < 0) {
		perror("Couldn't open log directory for a package");
		exit(ERR_CODE_BADDIR);
	}
	int verdir = openat(pkgdir, pkggraph_msg_version(self->message),
			O_RDWR | O_DIRECTORY | O_CREAT, 0755);
	if (verdir < 0) {
		perror("Couldn't open log directory for a package's version");
		exit(ERR_CODE_BADDIR);
	}
	int destfd = openat(verdir, pkggraph_msg_arch(self->message),
			O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (destfd < 0) {
		perror("Couldn't open log file for a package's version and arch");
		exit(ERR_CODE_BADDIR);
	}
	close(destfd);
	close(verdir);
	close(pkgdir);
	if (self->pub) {
		zstr_sendm(self->pub, "TRACE");
		zstr_sendf(self->pub, "Truncated log for %s/%s/%s",
				pkggraph_msg_pkgname(self->message),
				pkggraph_msg_version(self->message),
				pkggraph_msg_arch(self->message));
	}
}


//  ---------------------------------------------------------------------------
//  set_pub_to_provided
//

static void
set_pub_to_provided (client_t *self)
{
	if (self->pub)
		zsock_destroy(&(self->pub));
	self->pub = zsock_new_pub(self->args->pubpath);
	bfs_ensure_sock_perms(self->args->pubpath);
}

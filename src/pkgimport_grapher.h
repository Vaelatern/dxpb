/*  =========================================================================
    pkgimport_grapher - Package Grapher

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_grapher.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGIMPORT_GRAPHER_H_INCLUDED
#define PKGIMPORT_GRAPHER_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGIMPORT_GRAPHER_T_DEFINED
typedef struct _pkgimport_grapher_t pkgimport_grapher_t;
#define PKGIMPORT_GRAPHER_T_DEFINED
#endif

//  @interface
//  Create a new pkgimport_grapher, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkgimport_grapher_t *
pkgimport_grapher_new (void);

//  Destroy the pkgimport_grapher and free all memory used by the object.
void
    pkgimport_grapher_destroy (pkgimport_grapher_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkgimport_grapher_actor (pkgimport_grapher_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkgimport_grapher_msgpipe (pkgimport_grapher_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkgimport_grapher_connected (pkgimport_grapher_t *self);

//  Used to connect to the server.
int
    pkgimport_grapher_construct (pkgimport_grapher_t *self, const char *endpoint);

//  No explanation
int
    pkgimport_grapher_pkg_now_completed (pkgimport_grapher_t *self, int addr, int check, const char *pkgname, const char *version, const char *arch);

//  No explanation
int
    pkgimport_grapher_worker_available (pkgimport_grapher_t *self, int addr, int check, const char *hostarch, const char *targetarch, uint8_t iscross, uint16_t cost);

//  No explanation
int
    pkgimport_grapher_worker_gone (pkgimport_grapher_t *self, int addr, int check);

//  No explanation
int
    pkgimport_grapher_pkg_here (pkgimport_grapher_t *self, const char *pkgname, const char *version, const char *arch);

//  No explanation
int
    pkgimport_grapher_pkg_not_here (pkgimport_grapher_t *self, const char *pkgname, const char *version, const char *arch);

//  No explanation
int
    pkgimport_grapher_pkg_failed_to_build (pkgimport_grapher_t *self, int addr, int check, const char *pkgname, const char *version, const char *arch);

//  No explanation
int
    pkgimport_grapher_worker_not_appropiate (pkgimport_grapher_t *self, int addr, int check);

//  No explanation
int
    pkgimport_grapher_set_db_path (pkgimport_grapher_t *self, const char *dbpath);

//  No explanation
int
    pkgimport_grapher_set_publish_endpoint (pkgimport_grapher_t *self, const char *pubpoint);

//  No explanation
int
    pkgimport_grapher_set_ssl_client_keys (pkgimport_grapher_t *self, const char *privkey, const char *pubkey, const char *serverkey);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkgimport_grapher_set_verbose (pkgimport_grapher_t *self, bool verbose);

//  Self test of this class
void
    pkgimport_grapher_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

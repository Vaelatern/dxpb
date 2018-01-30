/*  =========================================================================
    pkgfiler_grapher - Package Grapher - File Manager Link

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgfiler_grapher.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGFILER_GRAPHER_H_INCLUDED
#define PKGFILER_GRAPHER_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGFILER_GRAPHER_T_DEFINED
typedef struct _pkgfiler_grapher_t pkgfiler_grapher_t;
#define PKGFILER_GRAPHER_T_DEFINED
#endif

//  @interface
//  Create a new pkgfiler_grapher, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkgfiler_grapher_t *
pkgfiler_grapher_new (void);

//  Destroy the pkgfiler_grapher and free all memory used by the object.
void
    pkgfiler_grapher_destroy (pkgfiler_grapher_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkgfiler_grapher_actor (pkgfiler_grapher_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkgfiler_grapher_msgpipe (pkgfiler_grapher_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkgfiler_grapher_connected (pkgfiler_grapher_t *self);

//  Used to connect to the server.
int
    pkgfiler_grapher_construct (pkgfiler_grapher_t *self, const char *endpoint);

//  No explanation
int
    pkgfiler_grapher_do_ispkghere (pkgfiler_grapher_t *self, const char *pkgname, const char *version, const char *arch);

//  No explanation
int
    pkgfiler_grapher_do_pkgdel (pkgfiler_grapher_t *self, const char *pkgname);


//  Takes privkey, pubkey, and serverkey, and sets them on the socket
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkgfiler_grapher_set_verbose (pkgfiler_grapher_t *self, bool verbose);

//  Self test of this class
void
    pkgfiler_grapher_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

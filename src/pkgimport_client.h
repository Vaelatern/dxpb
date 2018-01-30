/*  =========================================================================
    pkgimport_client - Package Importer

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_client.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGIMPORT_CLIENT_H_INCLUDED
#define PKGIMPORT_CLIENT_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGIMPORT_CLIENT_T_DEFINED
typedef struct _pkgimport_client_t pkgimport_client_t;
#define PKGIMPORT_CLIENT_T_DEFINED
#endif

//  @interface
//  Create a new pkgimport_client, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkgimport_client_t *
pkgimport_client_new (void);

//  Destroy the pkgimport_client and free all memory used by the object.
void
    pkgimport_client_destroy (pkgimport_client_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkgimport_client_actor (pkgimport_client_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkgimport_client_msgpipe (pkgimport_client_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkgimport_client_connected (pkgimport_client_t *self);

//  Used to connect to the server.
int
    pkgimport_client_construct (pkgimport_client_t *self, const char *endpoint);

//  Used to connect to the server.
int
    pkgimport_client_set_xbps_src_path (pkgimport_client_t *self, const char *xbps_src_path);


//  Takes privkey, pubkey, and serverkey, and sets them on the socket
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkgimport_client_set_verbose (pkgimport_client_t *self, bool verbose);

//  Self test of this class
void
    pkgimport_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

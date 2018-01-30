/*  =========================================================================
    pkgimport_poke - Package Poker

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_poke.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGIMPORT_POKE_H_INCLUDED
#define PKGIMPORT_POKE_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGIMPORT_POKE_T_DEFINED
typedef struct _pkgimport_poke_t pkgimport_poke_t;
#define PKGIMPORT_POKE_T_DEFINED
#endif

//  @interface
//  Create a new pkgimport_poke, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkgimport_poke_t *
pkgimport_poke_new (void);

//  Destroy the pkgimport_poke and free all memory used by the object.
void
    pkgimport_poke_destroy (pkgimport_poke_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkgimport_poke_actor (pkgimport_poke_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkgimport_poke_msgpipe (pkgimport_poke_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkgimport_poke_connected (pkgimport_poke_t *self);

//  Used to connect to the server.
int
    pkgimport_poke_connect_now (pkgimport_poke_t *self, const char *endpoint);


//  Takes privkey, pubkey, and serverkey, and sets them on the socket
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkgimport_poke_set_verbose (pkgimport_poke_t *self, bool verbose);

//  Self test of this class
void
    pkgimport_poke_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

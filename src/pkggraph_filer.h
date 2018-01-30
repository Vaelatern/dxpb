/*  =========================================================================
    pkggraph_filer - Package Builder - Where Logs Meet Disk

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_filer.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGGRAPH_FILER_H_INCLUDED
#define PKGGRAPH_FILER_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGGRAPH_FILER_T_DEFINED
typedef struct _pkggraph_filer_t pkggraph_filer_t;
#define PKGGRAPH_FILER_T_DEFINED
#endif

//  @interface
//  Create a new pkggraph_filer, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkggraph_filer_t *
pkggraph_filer_new (void);

//  Destroy the pkggraph_filer and free all memory used by the object.
void
    pkggraph_filer_destroy (pkggraph_filer_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkggraph_filer_actor (pkggraph_filer_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkggraph_filer_msgpipe (pkggraph_filer_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkggraph_filer_connected (pkggraph_filer_t *self);

//  Used to connect to the server.
int
    pkggraph_filer_construct (pkggraph_filer_t *self, const char *endpoint);

//  No explanation
int
    pkggraph_filer_set_logdir (pkggraph_filer_t *self, const char *dir);

//  No explanation
int
    pkggraph_filer_set_pubpath (pkggraph_filer_t *self, const char *pubpath);


//  Takes privkey, pubkey, and serverkey, and sets them on the socket
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkggraph_filer_set_verbose (pkggraph_filer_t *self, bool verbose);

//  Self test of this class
void
    pkggraph_filer_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

/*  =========================================================================
    pkggraph_worker - Package Builder - Where Work Gets Done

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_worker.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGGRAPH_WORKER_H_INCLUDED
#define PKGGRAPH_WORKER_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGGRAPH_WORKER_T_DEFINED
typedef struct _pkggraph_worker_t pkggraph_worker_t;
#define PKGGRAPH_WORKER_T_DEFINED
#endif

//  @interface
//  Create a new pkggraph_worker, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkggraph_worker_t *
pkggraph_worker_new (zsock_t *);

//  Destroy the pkggraph_worker and free all memory used by the object.
void
    pkggraph_worker_destroy (pkggraph_worker_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkggraph_worker_actor (pkggraph_worker_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkggraph_worker_msgpipe (pkggraph_worker_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkggraph_worker_connected (pkggraph_worker_t *self);

//  Used to connect to the server.
int
    pkggraph_worker_construct (pkggraph_worker_t *self, const char *endpoint);

//  No explanation
int
    pkggraph_worker_set_repopath (pkggraph_worker_t *self, const char *repopath);

//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.
uint32_t
    pkggraph_worker_set_build_params (pkggraph_worker_t *self, const char *hostarch, const char *targetarch, uint8_t iscross, uint16_t cost);

//  Return last received ok
uint32_t
    pkggraph_worker_ok (pkggraph_worker_t *self);


//  Takes privkey, pubkey, and serverkey, and sets them on the socket
void    pkgall_client_ssl_setcurve(void *, const char *, const char *, const char *);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkggraph_worker_set_verbose (pkggraph_worker_t *self, bool verbose);

//  Self test of this class
void
    pkggraph_worker_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

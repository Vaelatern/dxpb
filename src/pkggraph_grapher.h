/*  =========================================================================
    pkggraph_grapher - Package Grapher - World Link

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_grapher.xml, or
     * The code generation script that built this file: ../exec/zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGGRAPH_GRAPHER_H_INCLUDED
#define PKGGRAPH_GRAPHER_H_INCLUDED

#define ZPROTO_UNUSED(object) (void)object

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGGRAPH_GRAPHER_T_DEFINED
typedef struct _pkggraph_grapher_t pkggraph_grapher_t;
#define PKGGRAPH_GRAPHER_T_DEFINED
#endif

//  @interface
//  Create a new pkggraph_grapher, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
pkggraph_grapher_t *
pkggraph_grapher_new (void);

//  Destroy the pkggraph_grapher and free all memory used by the object.
void
    pkggraph_grapher_destroy (pkggraph_grapher_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    pkggraph_grapher_actor (pkggraph_grapher_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    pkggraph_grapher_msgpipe (pkggraph_grapher_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    pkggraph_grapher_connected (pkggraph_grapher_t *self);

//  Used to connect to the server.                                                  
int 
    pkggraph_grapher_construct (pkggraph_grapher_t *self, const char *endpoint);

//  Send UPDATE BOOTSTRAP message to server, takes ownership of message
//  and destroys message when done sending it.
int
    pkggraph_grapher_update_bootstrap (pkggraph_grapher_t *self);

//  Send WORKERCANHELP message to server, takes ownership of message
//  and destroys message when done sending it.
int
    pkggraph_grapher_workercanhelp (pkggraph_grapher_t *self, uint16_t addr, uint32_t check, const char *pkgname, const char *version, const char *arch);

//  Enable verbose tracing (animation) of state machine activity.
void
    pkggraph_grapher_set_verbose (pkggraph_grapher_t *self, bool verbose);

//  Self test of this class
void
    pkggraph_grapher_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

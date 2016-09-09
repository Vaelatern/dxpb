/*  =========================================================================
    pkgimport_msg - Package Import Managment Protocol

    Codec header for pkgimport_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGIMPORT_MSG_H_INCLUDED
#define PKGIMPORT_MSG_H_INCLUDED

/*  These are the pkgimport_msg messages:

    HELLO - 
        proto_version       string      Package Import Management Protocol Version 00

    ROGER - 
        proto_version       string      Package Import Management Protocol Version 00

    IFORGOTU - 
        proto_version       string      Package Import Management Protocol Version 00

    RUREADY - 
        proto_version       string      Package Import Management Protocol Version 00

    READY - 
        proto_version       string      Package Import Management Protocol Version 00

    RESET - 
        proto_version       string      Package Import Management Protocol Version 00

    INVALID - 
        proto_version       string      Package Import Management Protocol Version 00

    RDY2WRK - 
        proto_version       string      Package Import Management Protocol Version 00

    PING - 
        proto_version       string      Package Import Management Protocol Version 00

    PLZREADALL - 
        proto_version       string      Package Import Management Protocol Version 00

    IAMTHEGRAPHER - 
        proto_version       string      Package Import Management Protocol Version 00

    KTHNKSBYE - 
        proto_version       string      Package Import Management Protocol Version 00

    STABLESTATUSPLZ - 
        proto_version       string      Package Import Management Protocol Version 00

    STOP - 
        proto_version       string      Package Import Management Protocol Version 00

    TURNONTHENEWS - Used to trigger a git fast forward and reading of new packages.
        proto_version       string      Package Import Management Protocol Version 00

    PLZREAD - 
        proto_version       string      Package Import Management Protocol Version 00
        pkgname             string      

    DIDREAD - 
        proto_version       string      Package Import Management Protocol Version 00
        pkgname             string      

    PKGINFO - 
        proto_version       string      Package Import Management Protocol Version 00
        info_revision       string      Pkginfo Revision Within Protocol Version
        pkgname             string      
        version             string      
        arch                string      
        nativehostneeds     longstr     
        nativetargetneeds   longstr     
        crosshostneeds      longstr     
        crosstargetneeds    longstr     
        cancross            number 1    
        broken              number 1    
        bootstrap           number 1    
        restricted          number 1    

    PKGDEL - 
        proto_version       string      Package Import Management Protocol Version 00
        pkgname             string      

    STABLE - 
        proto_version       string      Package Import Management Protocol Version 00
        commithash          string      Hash corresponding to git HEAD

    UNSTABLE - 
        proto_version       string      Package Import Management Protocol Version 00
        commithash          string      Empty field to mean "No"

    WESEEHASH - 
        proto_version       string      Package Import Management Protocol Version 00
        commithash          string      Hash of git HEAD as seen by persistent storage
*/


#define PKGIMPORT_MSG_HELLO                 1
#define PKGIMPORT_MSG_ROGER                 2
#define PKGIMPORT_MSG_IFORGOTU              3
#define PKGIMPORT_MSG_RUREADY               4
#define PKGIMPORT_MSG_READY                 5
#define PKGIMPORT_MSG_RESET                 6
#define PKGIMPORT_MSG_INVALID               7
#define PKGIMPORT_MSG_RDY2WRK               8
#define PKGIMPORT_MSG_PING                  9
#define PKGIMPORT_MSG_PLZREADALL            10
#define PKGIMPORT_MSG_IAMTHEGRAPHER         11
#define PKGIMPORT_MSG_KTHNKSBYE             12
#define PKGIMPORT_MSG_STABLESTATUSPLZ       13
#define PKGIMPORT_MSG_STOP                  14
#define PKGIMPORT_MSG_TURNONTHENEWS         15
#define PKGIMPORT_MSG_PLZREAD               16
#define PKGIMPORT_MSG_DIDREAD               17
#define PKGIMPORT_MSG_PKGINFO               18
#define PKGIMPORT_MSG_PKGDEL                19
#define PKGIMPORT_MSG_STABLE                20
#define PKGIMPORT_MSG_UNSTABLE              21
#define PKGIMPORT_MSG_WESEEHASH             22

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGIMPORT_MSG_T_DEFINED
typedef struct _pkgimport_msg_t pkgimport_msg_t;
#define PKGIMPORT_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty pkgimport_msg
pkgimport_msg_t *
    pkgimport_msg_new (void);

//  Destroy a pkgimport_msg instance
void
    pkgimport_msg_destroy (pkgimport_msg_t **self_p);

//  Create a deep copy of a pkgimport_msg instance
pkgimport_msg_t *
    pkgimport_msg_dup (pkgimport_msg_t *other);

//  Receive a pkgimport_msg from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
int
    pkgimport_msg_recv (pkgimport_msg_t *self, zsock_t *input);

//  Send the pkgimport_msg to the output socket, does not destroy it
int
    pkgimport_msg_send (pkgimport_msg_t *self, zsock_t *output);


//  Print contents of message to stdout
void
    pkgimport_msg_print (pkgimport_msg_t *self);

//  Get/set the message routing id
zframe_t *
    pkgimport_msg_routing_id (pkgimport_msg_t *self);
void
    pkgimport_msg_set_routing_id (pkgimport_msg_t *self, zframe_t *routing_id);

//  Get the pkgimport_msg id and printable command
int
    pkgimport_msg_id (pkgimport_msg_t *self);
void
    pkgimport_msg_set_id (pkgimport_msg_t *self, int id);
const char *
    pkgimport_msg_command (pkgimport_msg_t *self);

//  Get/set the pkgname field
const char *
    pkgimport_msg_pkgname (pkgimport_msg_t *self);
void
    pkgimport_msg_set_pkgname (pkgimport_msg_t *self, const char *value);

//  Get/set the version field
const char *
    pkgimport_msg_version (pkgimport_msg_t *self);
void
    pkgimport_msg_set_version (pkgimport_msg_t *self, const char *value);

//  Get/set the arch field
const char *
    pkgimport_msg_arch (pkgimport_msg_t *self);
void
    pkgimport_msg_set_arch (pkgimport_msg_t *self, const char *value);

//  Get/set the nativehostneeds field
const char *
    pkgimport_msg_nativehostneeds (pkgimport_msg_t *self);
void
    pkgimport_msg_set_nativehostneeds (pkgimport_msg_t *self, const char *value);

//  Get/set the nativetargetneeds field
const char *
    pkgimport_msg_nativetargetneeds (pkgimport_msg_t *self);
void
    pkgimport_msg_set_nativetargetneeds (pkgimport_msg_t *self, const char *value);

//  Get/set the crosshostneeds field
const char *
    pkgimport_msg_crosshostneeds (pkgimport_msg_t *self);
void
    pkgimport_msg_set_crosshostneeds (pkgimport_msg_t *self, const char *value);

//  Get/set the crosstargetneeds field
const char *
    pkgimport_msg_crosstargetneeds (pkgimport_msg_t *self);
void
    pkgimport_msg_set_crosstargetneeds (pkgimport_msg_t *self, const char *value);

//  Get/set the cancross field
byte
    pkgimport_msg_cancross (pkgimport_msg_t *self);
void
    pkgimport_msg_set_cancross (pkgimport_msg_t *self, byte cancross);

//  Get/set the broken field
byte
    pkgimport_msg_broken (pkgimport_msg_t *self);
void
    pkgimport_msg_set_broken (pkgimport_msg_t *self, byte broken);

//  Get/set the bootstrap field
byte
    pkgimport_msg_bootstrap (pkgimport_msg_t *self);
void
    pkgimport_msg_set_bootstrap (pkgimport_msg_t *self, byte bootstrap);

//  Get/set the restricted field
byte
    pkgimport_msg_restricted (pkgimport_msg_t *self);
void
    pkgimport_msg_set_restricted (pkgimport_msg_t *self, byte restricted);

//  Get/set the commithash field
const char *
    pkgimport_msg_commithash (pkgimport_msg_t *self);
void
    pkgimport_msg_set_commithash (pkgimport_msg_t *self, const char *value);

//  Self test of this class
void
    pkgimport_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define pkgimport_msg_dump  pkgimport_msg_print

#ifdef __cplusplus
}
#endif

#endif

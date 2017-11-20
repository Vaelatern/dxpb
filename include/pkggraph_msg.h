/*  =========================================================================
    pkggraph_msg - Package Graph Potocol

    Codec header for pkggraph_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_msg.xml, or
     * The code generation script that built this file: ../exec/zproto_codec_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGGRAPH_MSG_H_INCLUDED
#define PKGGRAPH_MSG_H_INCLUDED

/*  These are the pkggraph_msg messages:

    HELLO - 
        proto_version       string      Package Graph Protocol Version 00

    ROGER - 
        proto_version       string      Package Graph Protocol Version 00

    IFORGOTU - 
        proto_version       string      Package Graph Protocol Version 00

    INVALID - 
        proto_version       string      Package Graph Protocol Version 00

    PING - 
        proto_version       string      Package Graph Protocol Version 00

    STOP - 
        proto_version       string      Package Graph Protocol Version 00

    PKGDEL - 
        proto_version       string      Package Graph Protocol Version 00
        pkgname             string      

    NEEDPKG - 
        proto_version       string      Package Graph Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    ICANHELP - 
        proto_version       string      Package Graph Protocol Version 00
        hostarch            string      
        targetarch          string      
        iscross             number 1    Strict servers might double check this.
        cost                number 2    This should be 0 unless the builder is expensive or slow enough to warrant a lower priority when handing out work. If it's just really fast, then more builders are the answer, not a negative cost.
        addr                number 2    
        check               number 4    

    WORKERCANHELP - 
        proto_version       string      Package Graph Protocol Version 00
        addr                number 2    
        check               number 4    
        pkgname             string      
        version             string      
        arch                string      

    FORGET_ABOUT_ME - 
        proto_version       string      Package Graph Protocol Version 00
        addr                number 2    
        check               number 4    

    RESETLOG - 
        proto_version       string      Package Graph Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    LOGHERE - 
        proto_version       string      Package Graph Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      
        logs                chunk       Arbitrary text field to append to appropiate log file.

    UPDATE_BOOTSTRAP - Useful for when a bootstrap package has been upgraded.
        proto_version       string      Package Graph Protocol Version 00

    PKGDONE - 
        proto_version       string      Package Graph Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    JOB_ENDED - For any reason. It is an error to do this with success without
also marking a package as done using a PKGDONE.
        proto_version       string      Package Graph Protocol Version 00
        addr                number 2    
        check               number 4    
        cause               number 1    These are enumerated in bworker_exit_status.h
        pkgname             string      
        version             string      
        arch                string      

    IMTHEGRAPHER - 
        proto_version       string      Package Graph Protocol Version 00

    IAMSTORAGE - 
        proto_version       string      Package Graph Protocol Version 00

    IMAWORKER - 
        proto_version       string      Package Graph Protocol Version 00

    GRAPHREADY - 
        proto_version       string      Package Graph Protocol Version 00

    GRAPHNOTREADY - 
        proto_version       string      Package Graph Protocol Version 00
*/


#define PKGGRAPH_MSG_HELLO                  1
#define PKGGRAPH_MSG_ROGER                  2
#define PKGGRAPH_MSG_IFORGOTU               3
#define PKGGRAPH_MSG_INVALID                4
#define PKGGRAPH_MSG_PING                   5
#define PKGGRAPH_MSG_STOP                   6
#define PKGGRAPH_MSG_PKGDEL                 7
#define PKGGRAPH_MSG_NEEDPKG                8
#define PKGGRAPH_MSG_ICANHELP               9
#define PKGGRAPH_MSG_WORKERCANHELP          10
#define PKGGRAPH_MSG_FORGET_ABOUT_ME        11
#define PKGGRAPH_MSG_RESETLOG               12
#define PKGGRAPH_MSG_LOGHERE                13
#define PKGGRAPH_MSG_UPDATE_BOOTSTRAP       14
#define PKGGRAPH_MSG_PKGDONE                15
#define PKGGRAPH_MSG_JOB_ENDED              16
#define PKGGRAPH_MSG_IMTHEGRAPHER           17
#define PKGGRAPH_MSG_IAMSTORAGE             18
#define PKGGRAPH_MSG_IMAWORKER              19
#define PKGGRAPH_MSG_GRAPHREADY             20
#define PKGGRAPH_MSG_GRAPHNOTREADY          21

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGGRAPH_MSG_T_DEFINED
typedef struct _pkggraph_msg_t pkggraph_msg_t;
#define PKGGRAPH_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty pkggraph_msg
pkggraph_msg_t *
    pkggraph_msg_new (void);

//  Destroy a pkggraph_msg instance
void
    pkggraph_msg_destroy (pkggraph_msg_t **self_p);

//  Create a deep copy of a pkggraph_msg instance
pkggraph_msg_t *
    pkggraph_msg_dup (pkggraph_msg_t *other);

//  Receive a pkggraph_msg from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
int
    pkggraph_msg_recv (pkggraph_msg_t *self, zsock_t *input);

//  Send the pkggraph_msg to the output socket, does not destroy it
int
    pkggraph_msg_send (pkggraph_msg_t *self, zsock_t *output);


//  Print contents of message to stdout
void
    pkggraph_msg_print (pkggraph_msg_t *self);

//  Get/set the message routing id
zframe_t *
    pkggraph_msg_routing_id (pkggraph_msg_t *self);
void
    pkggraph_msg_set_routing_id (pkggraph_msg_t *self, zframe_t *routing_id);

//  Get the pkggraph_msg id and printable command
int
    pkggraph_msg_id (pkggraph_msg_t *self);
void
    pkggraph_msg_set_id (pkggraph_msg_t *self, int id);
const char *
    pkggraph_msg_command (pkggraph_msg_t *self);

//  Get/set the pkgname field
const char *
    pkggraph_msg_pkgname (pkggraph_msg_t *self);
void
    pkggraph_msg_set_pkgname (pkggraph_msg_t *self, const char *value);

//  Get/set the version field
const char *
    pkggraph_msg_version (pkggraph_msg_t *self);
void
    pkggraph_msg_set_version (pkggraph_msg_t *self, const char *value);

//  Get/set the arch field
const char *
    pkggraph_msg_arch (pkggraph_msg_t *self);
void
    pkggraph_msg_set_arch (pkggraph_msg_t *self, const char *value);

//  Get/set the hostarch field
const char *
    pkggraph_msg_hostarch (pkggraph_msg_t *self);
void
    pkggraph_msg_set_hostarch (pkggraph_msg_t *self, const char *value);

//  Get/set the targetarch field
const char *
    pkggraph_msg_targetarch (pkggraph_msg_t *self);
void
    pkggraph_msg_set_targetarch (pkggraph_msg_t *self, const char *value);

//  Get/set the iscross field
byte
    pkggraph_msg_iscross (pkggraph_msg_t *self);
void
    pkggraph_msg_set_iscross (pkggraph_msg_t *self, byte iscross);

//  Get/set the cost field
uint16_t
    pkggraph_msg_cost (pkggraph_msg_t *self);
void
    pkggraph_msg_set_cost (pkggraph_msg_t *self, uint16_t cost);

//  Get/set the addr field
uint16_t
    pkggraph_msg_addr (pkggraph_msg_t *self);
void
    pkggraph_msg_set_addr (pkggraph_msg_t *self, uint16_t addr);

//  Get/set the check field
uint32_t
    pkggraph_msg_check (pkggraph_msg_t *self);
void
    pkggraph_msg_set_check (pkggraph_msg_t *self, uint32_t check);

//  Get a copy of the logs field
zchunk_t *
    pkggraph_msg_logs (pkggraph_msg_t *self);
//  Get the logs field and transfer ownership to caller
zchunk_t *
    pkggraph_msg_get_logs (pkggraph_msg_t *self);
//  Set the logs field, transferring ownership from caller
void
    pkggraph_msg_set_logs (pkggraph_msg_t *self, zchunk_t **chunk_p);

//  Get/set the cause field
byte
    pkggraph_msg_cause (pkggraph_msg_t *self);
void
    pkggraph_msg_set_cause (pkggraph_msg_t *self, byte cause);

//  Self test of this class
void
    pkggraph_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define pkggraph_msg_dump   pkggraph_msg_print

#ifdef __cplusplus
}
#endif

#endif

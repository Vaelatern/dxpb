/*  =========================================================================
    pkgfiles_msg - DXPB File Location and Management Protocol

    Codec header for pkgfiles_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgfiles_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGFILES_MSG_H_INCLUDED
#define PKGFILES_MSG_H_INCLUDED

/*  These are the pkgfiles_msg messages:

    HELLO - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    ROGER - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    IFORGOTU - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    PING - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    INVALID - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    PKGDEL - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    WANNASHARE_ - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    IWANNASHARE - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    IDONTWANNASHARE - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    GIMMEGIMMEGIMME - 
        proto_version       string      DXPB PKG Management Protocol Version 00

    CHUNK - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        validchunk          number 1    
        pkgname             string      
        version             string      
        arch                string      
        subpath             string      Subdir of hostdir where the pkg can be found
        position            number 8    
        eof                 number 1    
        data                chunk       

    PKGNOTHERE - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    PKGHERE - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      

    ISPKGHERE - 
        proto_version       string      DXPB PKG Management Protocol Version 00
        pkgname             string      
        version             string      
        arch                string      
*/


#define PKGFILES_MSG_HELLO                  1
#define PKGFILES_MSG_ROGER                  2
#define PKGFILES_MSG_IFORGOTU               3
#define PKGFILES_MSG_PING                   4
#define PKGFILES_MSG_INVALID                5
#define PKGFILES_MSG_PKGDEL                 6
#define PKGFILES_MSG_WANNASHARE_            7
#define PKGFILES_MSG_IWANNASHARE            8
#define PKGFILES_MSG_IDONTWANNASHARE        9
#define PKGFILES_MSG_GIMMEGIMMEGIMME        10
#define PKGFILES_MSG_CHUNK                  11
#define PKGFILES_MSG_PKGNOTHERE             12
#define PKGFILES_MSG_PKGHERE                13
#define PKGFILES_MSG_ISPKGHERE              14

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef PKGFILES_MSG_T_DEFINED
typedef struct _pkgfiles_msg_t pkgfiles_msg_t;
#define PKGFILES_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty pkgfiles_msg
pkgfiles_msg_t *
    pkgfiles_msg_new (void);

//  Destroy a pkgfiles_msg instance
void
    pkgfiles_msg_destroy (pkgfiles_msg_t **self_p);

//  Create a deep copy of a pkgfiles_msg instance
pkgfiles_msg_t *
    pkgfiles_msg_dup (pkgfiles_msg_t *other);

//  Receive a pkgfiles_msg from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
int
    pkgfiles_msg_recv (pkgfiles_msg_t *self, zsock_t *input);

//  Send the pkgfiles_msg to the output socket, does not destroy it
int
    pkgfiles_msg_send (pkgfiles_msg_t *self, zsock_t *output);


//  Print contents of message to stdout
void
    pkgfiles_msg_print (pkgfiles_msg_t *self);

//  Get/set the message routing id
zframe_t *
    pkgfiles_msg_routing_id (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_routing_id (pkgfiles_msg_t *self, zframe_t *routing_id);

//  Get the pkgfiles_msg id and printable command
int
    pkgfiles_msg_id (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_id (pkgfiles_msg_t *self, int id);
const char *
    pkgfiles_msg_command (pkgfiles_msg_t *self);

//  Get/set the pkgname field
const char *
    pkgfiles_msg_pkgname (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_pkgname (pkgfiles_msg_t *self, const char *value);

//  Get/set the version field
const char *
    pkgfiles_msg_version (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_version (pkgfiles_msg_t *self, const char *value);

//  Get/set the arch field
const char *
    pkgfiles_msg_arch (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_arch (pkgfiles_msg_t *self, const char *value);

//  Get/set the validchunk field
byte
    pkgfiles_msg_validchunk (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_validchunk (pkgfiles_msg_t *self, byte validchunk);

//  Get/set the subpath field
const char *
    pkgfiles_msg_subpath (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_subpath (pkgfiles_msg_t *self, const char *value);

//  Get/set the position field
uint64_t
    pkgfiles_msg_position (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_position (pkgfiles_msg_t *self, uint64_t position);

//  Get/set the eof field
byte
    pkgfiles_msg_eof (pkgfiles_msg_t *self);
void
    pkgfiles_msg_set_eof (pkgfiles_msg_t *self, byte eof);

//  Get a copy of the data field
zchunk_t *
    pkgfiles_msg_data (pkgfiles_msg_t *self);
//  Get the data field and transfer ownership to caller
zchunk_t *
    pkgfiles_msg_get_data (pkgfiles_msg_t *self);
//  Set the data field, transferring ownership from caller
void
    pkgfiles_msg_set_data (pkgfiles_msg_t *self, zchunk_t **chunk_p);

//  Self test of this class
void
    pkgfiles_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define pkgfiles_msg_dump   pkgfiles_msg_print

#ifdef __cplusplus
}
#endif

#endif

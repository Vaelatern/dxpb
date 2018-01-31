/*  =========================================================================
    pkgfiles_msg - DXPB File Location and Management Protocol

    Codec class for pkgfiles_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgfiles_msg.xml, or
     * The code generation script that built this file: ../exec/zproto_codec_c
    ************************************************************************
    =========================================================================
*/

/*
@header
    pkgfiles_msg - DXPB File Location and Management Protocol
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/pkgfiles_msg.h"

//  Structure of our class

struct _pkgfiles_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  pkgfiles_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char pkgname [256];                 //  pkgname
    char version [256];                 //  version
    char arch [256];                    //  arch
    char checksum [256];                //  checksum
    byte validchunk;                    //  validchunk
    char subpath [256];                 //  Subdir of hostdir where the pkg can be found
    uint64_t position;                  //  position
    byte eof;                           //  eof
    zchunk_t *data;                     //  data
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("pkgfiles_msg: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("pkgfiles_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("pkgfiles_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("pkgfiles_msg: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("pkgfiles_msg: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("pkgfiles_msg: GET_STRING failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("pkgfiles_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new pkgfiles_msg

pkgfiles_msg_t *
pkgfiles_msg_new (void)
{
    pkgfiles_msg_t *self = (pkgfiles_msg_t *) zmalloc (sizeof (pkgfiles_msg_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the pkgfiles_msg

void
pkgfiles_msg_destroy (pkgfiles_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkgfiles_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zchunk_destroy (&self->data);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a pkgfiles_msg instance

pkgfiles_msg_t *
pkgfiles_msg_dup (pkgfiles_msg_t *other)
{
    assert (other);
    pkgfiles_msg_t *copy = pkgfiles_msg_new ();

    // Copy the routing and message id
    pkgfiles_msg_set_routing_id (copy, zframe_dup (pkgfiles_msg_routing_id (other)));
    pkgfiles_msg_set_id (copy, pkgfiles_msg_id (other));

    // Copy the rest of the fields
    pkgfiles_msg_set_pkgname (copy, pkgfiles_msg_pkgname (other));
    pkgfiles_msg_set_version (copy, pkgfiles_msg_version (other));
    pkgfiles_msg_set_arch (copy, pkgfiles_msg_arch (other));
    pkgfiles_msg_set_checksum (copy, pkgfiles_msg_checksum (other));
    pkgfiles_msg_set_validchunk (copy, pkgfiles_msg_validchunk (other));
    pkgfiles_msg_set_subpath (copy, pkgfiles_msg_subpath (other));
    pkgfiles_msg_set_position (copy, pkgfiles_msg_position (other));
    pkgfiles_msg_set_eof (copy, pkgfiles_msg_eof (other));
    {
        zchunk_t *dup_chunk = zchunk_dup (pkgfiles_msg_data (other));
        pkgfiles_msg_set_data (copy, &dup_chunk);
    }

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a pkgfiles_msg from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
pkgfiles_msg_recv (pkgfiles_msg_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("pkgfiles_msg: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("pkgfiles_msg: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0)) {
        zsys_warning ("pkgfiles_msg: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case PKGFILES_MSG_HELLO:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_ROGER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_IFORGOTU:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_PING:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_INVALID:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_PKGDEL:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            break;

        case PKGFILES_MSG_WANNASHARE_:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGFILES_MSG_IWANNASHARE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            GET_STRING (self->checksum);
            break;

        case PKGFILES_MSG_IDONTWANNASHARE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGFILES_MSG_GIMMEGIMMEGIMME:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGFILES_MSG_CHUNK:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER1 (self->validchunk);
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            GET_STRING (self->subpath);
            GET_NUMBER8 (self->position);
            GET_NUMBER1 (self->eof);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("pkgfiles_msg: data is missing data");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
                zchunk_destroy (&self->data);
                self->data = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case PKGFILES_MSG_PKGNOTHERE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGFILES_MSG_PKGHERE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGFILES_MSG_ISPKGHERE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "DPKG00")) {
                    zsys_warning ("pkgfiles_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        default:
            zsys_warning ("pkgfiles_msg: bad message ID");
            rc = -2;            //  Malformed
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return rc;

    //  Error returns
    malformed:
        zmq_msg_close (&frame);
        return rc;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the pkgfiles_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
pkgfiles_msg_send (pkgfiles_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case PKGFILES_MSG_HELLO:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_ROGER:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_IFORGOTU:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_PING:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_INVALID:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_PKGDEL:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            break;
        case PKGFILES_MSG_WANNASHARE_:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGFILES_MSG_IWANNASHARE:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            frame_size += 1 + strlen (self->checksum);
            break;
        case PKGFILES_MSG_IDONTWANNASHARE:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGFILES_MSG_GIMMEGIMMEGIMME:
            frame_size += 1 + strlen ("DPKG00");
            break;
        case PKGFILES_MSG_CHUNK:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1;            //  validchunk
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            frame_size += 1 + strlen (self->subpath);
            frame_size += 8;            //  position
            frame_size += 1;            //  eof
            frame_size += 4;            //  Size is 4 octets
            if (self->data)
                frame_size += zchunk_size (self->data);
            break;
        case PKGFILES_MSG_PKGNOTHERE:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGFILES_MSG_PKGHERE:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGFILES_MSG_ISPKGHERE:
            frame_size += 1 + strlen ("DPKG00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case PKGFILES_MSG_HELLO:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_ROGER:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_IFORGOTU:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_PING:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_INVALID:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_PKGDEL:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            break;

        case PKGFILES_MSG_WANNASHARE_:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGFILES_MSG_IWANNASHARE:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            PUT_STRING (self->checksum);
            break;

        case PKGFILES_MSG_IDONTWANNASHARE:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGFILES_MSG_GIMMEGIMMEGIMME:
            PUT_STRING ("DPKG00");
            break;

        case PKGFILES_MSG_CHUNK:
            PUT_STRING ("DPKG00");
            PUT_NUMBER1 (self->validchunk);
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            PUT_STRING (self->subpath);
            PUT_NUMBER8 (self->position);
            PUT_NUMBER1 (self->eof);
            if (self->data) {
                PUT_NUMBER4 (zchunk_size (self->data));
                memcpy (self->needle,
                        zchunk_data (self->data),
                        zchunk_size (self->data));
                self->needle += zchunk_size (self->data);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case PKGFILES_MSG_PKGNOTHERE:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGFILES_MSG_PKGHERE:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGFILES_MSG_ISPKGHERE:
            PUT_STRING ("DPKG00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
pkgfiles_msg_print (pkgfiles_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGFILES_MSG_HELLO:
            zsys_debug ("PKGFILES_MSG_HELLO:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_ROGER:
            zsys_debug ("PKGFILES_MSG_ROGER:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_IFORGOTU:
            zsys_debug ("PKGFILES_MSG_IFORGOTU:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_PING:
            zsys_debug ("PKGFILES_MSG_PING:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_INVALID:
            zsys_debug ("PKGFILES_MSG_INVALID:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_PKGDEL:
            zsys_debug ("PKGFILES_MSG_PKGDEL:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            break;

        case PKGFILES_MSG_WANNASHARE_:
            zsys_debug ("PKGFILES_MSG_WANNASHARE_:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGFILES_MSG_IWANNASHARE:
            zsys_debug ("PKGFILES_MSG_IWANNASHARE:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            zsys_debug ("    checksum='%s'", self->checksum);
            break;

        case PKGFILES_MSG_IDONTWANNASHARE:
            zsys_debug ("PKGFILES_MSG_IDONTWANNASHARE:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGFILES_MSG_GIMMEGIMMEGIMME:
            zsys_debug ("PKGFILES_MSG_GIMMEGIMMEGIMME:");
            zsys_debug ("    proto_version=dpkg00");
            break;

        case PKGFILES_MSG_CHUNK:
            zsys_debug ("PKGFILES_MSG_CHUNK:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    validchunk=%ld", (long) self->validchunk);
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            zsys_debug ("    subpath='%s'", self->subpath);
            zsys_debug ("    position=%ld", (long) self->position);
            zsys_debug ("    eof=%ld", (long) self->eof);
            zsys_debug ("    data=[ ... ]");
            break;

        case PKGFILES_MSG_PKGNOTHERE:
            zsys_debug ("PKGFILES_MSG_PKGNOTHERE:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGFILES_MSG_PKGHERE:
            zsys_debug ("PKGFILES_MSG_PKGHERE:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGFILES_MSG_ISPKGHERE:
            zsys_debug ("PKGFILES_MSG_ISPKGHERE:");
            zsys_debug ("    proto_version=dpkg00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
pkgfiles_msg_routing_id (pkgfiles_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
pkgfiles_msg_set_routing_id (pkgfiles_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the pkgfiles_msg id

int
pkgfiles_msg_id (pkgfiles_msg_t *self)
{
    assert (self);
    return self->id;
}

void
pkgfiles_msg_set_id (pkgfiles_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
pkgfiles_msg_command (pkgfiles_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGFILES_MSG_HELLO:
            return ("HELLO");
            break;
        case PKGFILES_MSG_ROGER:
            return ("ROGER");
            break;
        case PKGFILES_MSG_IFORGOTU:
            return ("IFORGOTU");
            break;
        case PKGFILES_MSG_PING:
            return ("PING");
            break;
        case PKGFILES_MSG_INVALID:
            return ("INVALID");
            break;
        case PKGFILES_MSG_PKGDEL:
            return ("PKGDEL");
            break;
        case PKGFILES_MSG_WANNASHARE_:
            return ("WANNASHARE_");
            break;
        case PKGFILES_MSG_IWANNASHARE:
            return ("IWANNASHARE");
            break;
        case PKGFILES_MSG_IDONTWANNASHARE:
            return ("IDONTWANNASHARE");
            break;
        case PKGFILES_MSG_GIMMEGIMMEGIMME:
            return ("GIMMEGIMMEGIMME");
            break;
        case PKGFILES_MSG_CHUNK:
            return ("CHUNK");
            break;
        case PKGFILES_MSG_PKGNOTHERE:
            return ("PKGNOTHERE");
            break;
        case PKGFILES_MSG_PKGHERE:
            return ("PKGHERE");
            break;
        case PKGFILES_MSG_ISPKGHERE:
            return ("ISPKGHERE");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the pkgname field

const char *
pkgfiles_msg_pkgname (pkgfiles_msg_t *self)
{
    assert (self);
    return self->pkgname;
}

void
pkgfiles_msg_set_pkgname (pkgfiles_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->pkgname)
        return;
    strncpy (self->pkgname, value, 255);
    self->pkgname [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the version field

const char *
pkgfiles_msg_version (pkgfiles_msg_t *self)
{
    assert (self);
    return self->version;
}

void
pkgfiles_msg_set_version (pkgfiles_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->version)
        return;
    strncpy (self->version, value, 255);
    self->version [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the arch field

const char *
pkgfiles_msg_arch (pkgfiles_msg_t *self)
{
    assert (self);
    return self->arch;
}

void
pkgfiles_msg_set_arch (pkgfiles_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->arch)
        return;
    strncpy (self->arch, value, 255);
    self->arch [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the checksum field

const char *
pkgfiles_msg_checksum (pkgfiles_msg_t *self)
{
    assert (self);
    return self->checksum;
}

void
pkgfiles_msg_set_checksum (pkgfiles_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->checksum)
        return;
    strncpy (self->checksum, value, 255);
    self->checksum [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the validchunk field

byte
pkgfiles_msg_validchunk (pkgfiles_msg_t *self)
{
    assert (self);
    return self->validchunk;
}

void
pkgfiles_msg_set_validchunk (pkgfiles_msg_t *self, byte validchunk)
{
    assert (self);
    self->validchunk = validchunk;
}


//  --------------------------------------------------------------------------
//  Get/set the subpath field

const char *
pkgfiles_msg_subpath (pkgfiles_msg_t *self)
{
    assert (self);
    return self->subpath;
}

void
pkgfiles_msg_set_subpath (pkgfiles_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->subpath)
        return;
    strncpy (self->subpath, value, 255);
    self->subpath [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the position field

uint64_t
pkgfiles_msg_position (pkgfiles_msg_t *self)
{
    assert (self);
    return self->position;
}

void
pkgfiles_msg_set_position (pkgfiles_msg_t *self, uint64_t position)
{
    assert (self);
    self->position = position;
}


//  --------------------------------------------------------------------------
//  Get/set the eof field

byte
pkgfiles_msg_eof (pkgfiles_msg_t *self)
{
    assert (self);
    return self->eof;
}

void
pkgfiles_msg_set_eof (pkgfiles_msg_t *self, byte eof)
{
    assert (self);
    self->eof = eof;
}


//  --------------------------------------------------------------------------
//  Get the data field without transferring ownership

zchunk_t *
pkgfiles_msg_data (pkgfiles_msg_t *self)
{
    assert (self);
    return self->data;
}

//  Get the data field and transfer ownership to caller

zchunk_t *
pkgfiles_msg_get_data (pkgfiles_msg_t *self)
{
    zchunk_t *data = self->data;
    self->data = NULL;
    return data;
}

//  Set the data field, transferring ownership from caller

void
pkgfiles_msg_set_data (pkgfiles_msg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->data);
    self->data = *chunk_p;
    *chunk_p = NULL;
}



//  --------------------------------------------------------------------------
//  Selftest

void
pkgfiles_msg_test (bool verbose)
{
    printf (" * pkgfiles_msg: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    pkgfiles_msg_t *self = pkgfiles_msg_new ();
    assert (self);
    pkgfiles_msg_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-pkgfiles_msg");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-pkgfiles_msg");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = pkgfiles_msg_new ();
    pkgfiles_msg_set_id (self, PKGFILES_MSG_HELLO);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_ROGER);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_IFORGOTU);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_PING);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_INVALID);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_PKGDEL);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_WANNASHARE_);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_IWANNASHARE);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_checksum (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_checksum (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_IDONTWANNASHARE);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_GIMMEGIMMEGIMME);

    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_CHUNK);

    pkgfiles_msg_set_validchunk (self, 123);
    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_subpath (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_position (self, 123);
    pkgfiles_msg_set_eof (self, 123);
    zchunk_t *chunk_data = zchunk_new ("Captcha Diem", 12);
    pkgfiles_msg_set_data (self, &chunk_data);
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (pkgfiles_msg_validchunk (self) == 123);
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_subpath (self), "Life is short but Now lasts for ever"));
        assert (pkgfiles_msg_position (self) == 123);
        assert (pkgfiles_msg_eof (self) == 123);
        assert (memcmp (zchunk_data (pkgfiles_msg_data (self)), "Captcha Diem", 12) == 0);
        if (instance == 1)
            zchunk_destroy (&chunk_data);
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_PKGNOTHERE);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_PKGHERE);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkgfiles_msg_set_id (self, PKGFILES_MSG_ISPKGHERE);

    pkgfiles_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgfiles_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgfiles_msg_send (self, output);
    pkgfiles_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgfiles_msg_recv (self, input);
        assert (pkgfiles_msg_routing_id (self));
        assert (streq (pkgfiles_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgfiles_msg_arch (self), "Life is short but Now lasts for ever"));
    }

    pkgfiles_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}

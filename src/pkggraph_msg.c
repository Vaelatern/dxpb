/*  =========================================================================
    pkggraph_msg - Package Graph Potocol

    Codec class for pkggraph_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    =========================================================================
*/

/*
@header
    pkggraph_msg - Package Graph Potocol
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/pkggraph_msg.h"

//  Structure of our class

struct _pkggraph_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  pkggraph_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char pkgname [256];                 //  pkgname
    char version [256];                 //  version
    char arch [256];                    //  arch
    char hostarch [256];                //  hostarch
    char targetarch [256];              //  targetarch
    byte iscross;                       //  Strict servers might double check this.
    uint16_t cost;                      //  This should be 0 unless the builder is expensive or slow enough to warrant a lower priority when handing out work. If it's just really fast, then more builders are the answer, not a negative cost.
    uint16_t addr;                      //  addr
    uint32_t check;                     //  check
    zchunk_t *logs;                     //  Arbitrary text field to append to appropiate log file.
    byte cause;                         //  These are enumerated in bworker_exit_status.h
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
        zsys_warning ("pkggraph_msg: GET_OCTETS failed"); \
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
        zsys_warning ("pkggraph_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("pkggraph_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("pkggraph_msg: GET_NUMBER4 failed"); \
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
        zsys_warning ("pkggraph_msg: GET_NUMBER8 failed"); \
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
        zsys_warning ("pkggraph_msg: GET_STRING failed"); \
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
        zsys_warning ("pkggraph_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new pkggraph_msg

pkggraph_msg_t *
pkggraph_msg_new (void)
{
    pkggraph_msg_t *self = (pkggraph_msg_t *) zmalloc (sizeof (pkggraph_msg_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the pkggraph_msg

void
pkggraph_msg_destroy (pkggraph_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkggraph_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zchunk_destroy (&self->logs);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a pkggraph_msg instance

pkggraph_msg_t *
pkggraph_msg_dup (pkggraph_msg_t *other)
{
    assert (other);
    pkggraph_msg_t *copy = pkggraph_msg_new ();

    // Copy the routing and message id
    pkggraph_msg_set_routing_id (copy, zframe_dup (pkggraph_msg_routing_id (other)));
    pkggraph_msg_set_id (copy, pkggraph_msg_id (other));

    // Copy the rest of the fields
    pkggraph_msg_set_pkgname (copy, pkggraph_msg_pkgname (other));
    pkggraph_msg_set_version (copy, pkggraph_msg_version (other));
    pkggraph_msg_set_arch (copy, pkggraph_msg_arch (other));
    pkggraph_msg_set_hostarch (copy, pkggraph_msg_hostarch (other));
    pkggraph_msg_set_targetarch (copy, pkggraph_msg_targetarch (other));
    pkggraph_msg_set_iscross (copy, pkggraph_msg_iscross (other));
    pkggraph_msg_set_cost (copy, pkggraph_msg_cost (other));
    pkggraph_msg_set_addr (copy, pkggraph_msg_addr (other));
    pkggraph_msg_set_check (copy, pkggraph_msg_check (other));
    {
        zchunk_t *dup_chunk = zchunk_dup (pkggraph_msg_logs (other));
        pkggraph_msg_set_logs (copy, &dup_chunk);
    }
    pkggraph_msg_set_cause (copy, pkggraph_msg_cause (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a pkggraph_msg from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
pkggraph_msg_recv (pkggraph_msg_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("pkggraph_msg: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("pkggraph_msg: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1)) {
        zsys_warning ("pkggraph_msg: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case PKGGRAPH_MSG_HELLO:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_ROGER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_INVALID:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_PING:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_STOP:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_PKGDEL:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            break;

        case PKGGRAPH_MSG_NEEDPKG:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_ICANHELP:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->hostarch);
            GET_STRING (self->targetarch);
            GET_NUMBER1 (self->iscross);
            GET_NUMBER2 (self->cost);
            GET_NUMBER2 (self->addr);
            GET_NUMBER4 (self->check);
            break;

        case PKGGRAPH_MSG_WORKERCANHELP:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->addr);
            GET_NUMBER4 (self->check);
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_FORGET_ABOUT_ME:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->addr);
            GET_NUMBER4 (self->check);
            break;

        case PKGGRAPH_MSG_RESETLOG:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_LOGHERE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("pkggraph_msg: logs is missing data");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
                zchunk_destroy (&self->logs);
                self->logs = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_PKGDONE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_JOB_ENDED:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->addr);
            GET_NUMBER4 (self->check);
            GET_NUMBER1 (self->cause);
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_IMTHEGRAPHER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_IAMSTORAGE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_IMAWORKER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_GRAPHREADY:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGGRAPH_MSG_GRAPHNOTREADY:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "GRPH00")) {
                    zsys_warning ("pkggraph_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        default:
            zsys_warning ("pkggraph_msg: bad message ID");
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
//  Send the pkggraph_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
pkggraph_msg_send (pkggraph_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case PKGGRAPH_MSG_HELLO:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_ROGER:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_INVALID:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_PING:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_STOP:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_PKGDEL:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->pkgname);
            break;
        case PKGGRAPH_MSG_NEEDPKG:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGGRAPH_MSG_ICANHELP:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->hostarch);
            frame_size += 1 + strlen (self->targetarch);
            frame_size += 1;            //  iscross
            frame_size += 2;            //  cost
            frame_size += 2;            //  addr
            frame_size += 4;            //  check
            break;
        case PKGGRAPH_MSG_WORKERCANHELP:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 2;            //  addr
            frame_size += 4;            //  check
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGGRAPH_MSG_FORGET_ABOUT_ME:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 2;            //  addr
            frame_size += 4;            //  check
            break;
        case PKGGRAPH_MSG_RESETLOG:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGGRAPH_MSG_LOGHERE:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            frame_size += 4;            //  Size is 4 octets
            if (self->logs)
                frame_size += zchunk_size (self->logs);
            break;
        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_PKGDONE:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGGRAPH_MSG_JOB_ENDED:
            frame_size += 1 + strlen ("GRPH00");
            frame_size += 2;            //  addr
            frame_size += 4;            //  check
            frame_size += 1;            //  cause
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            break;
        case PKGGRAPH_MSG_IMTHEGRAPHER:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_IAMSTORAGE:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_IMAWORKER:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_GRAPHREADY:
            frame_size += 1 + strlen ("GRPH00");
            break;
        case PKGGRAPH_MSG_GRAPHNOTREADY:
            frame_size += 1 + strlen ("GRPH00");
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case PKGGRAPH_MSG_HELLO:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_ROGER:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_INVALID:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_PING:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_STOP:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_PKGDEL:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->pkgname);
            break;

        case PKGGRAPH_MSG_NEEDPKG:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_ICANHELP:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->hostarch);
            PUT_STRING (self->targetarch);
            PUT_NUMBER1 (self->iscross);
            PUT_NUMBER2 (self->cost);
            PUT_NUMBER2 (self->addr);
            PUT_NUMBER4 (self->check);
            break;

        case PKGGRAPH_MSG_WORKERCANHELP:
            PUT_STRING ("GRPH00");
            PUT_NUMBER2 (self->addr);
            PUT_NUMBER4 (self->check);
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_FORGET_ABOUT_ME:
            PUT_STRING ("GRPH00");
            PUT_NUMBER2 (self->addr);
            PUT_NUMBER4 (self->check);
            break;

        case PKGGRAPH_MSG_RESETLOG:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_LOGHERE:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            if (self->logs) {
                PUT_NUMBER4 (zchunk_size (self->logs));
                memcpy (self->needle,
                        zchunk_data (self->logs),
                        zchunk_size (self->logs));
                self->needle += zchunk_size (self->logs);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_PKGDONE:
            PUT_STRING ("GRPH00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_JOB_ENDED:
            PUT_STRING ("GRPH00");
            PUT_NUMBER2 (self->addr);
            PUT_NUMBER4 (self->check);
            PUT_NUMBER1 (self->cause);
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            break;

        case PKGGRAPH_MSG_IMTHEGRAPHER:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_IAMSTORAGE:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_IMAWORKER:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_GRAPHREADY:
            PUT_STRING ("GRPH00");
            break;

        case PKGGRAPH_MSG_GRAPHNOTREADY:
            PUT_STRING ("GRPH00");
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
pkggraph_msg_print (pkggraph_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGGRAPH_MSG_HELLO:
            zsys_debug ("PKGGRAPH_MSG_HELLO:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_ROGER:
            zsys_debug ("PKGGRAPH_MSG_ROGER:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_INVALID:
            zsys_debug ("PKGGRAPH_MSG_INVALID:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_PING:
            zsys_debug ("PKGGRAPH_MSG_PING:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_STOP:
            zsys_debug ("PKGGRAPH_MSG_STOP:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_PKGDEL:
            zsys_debug ("PKGGRAPH_MSG_PKGDEL:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            break;

        case PKGGRAPH_MSG_NEEDPKG:
            zsys_debug ("PKGGRAPH_MSG_NEEDPKG:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGGRAPH_MSG_ICANHELP:
            zsys_debug ("PKGGRAPH_MSG_ICANHELP:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    hostarch='%s'", self->hostarch);
            zsys_debug ("    targetarch='%s'", self->targetarch);
            zsys_debug ("    iscross=%ld", (long) self->iscross);
            zsys_debug ("    cost=%ld", (long) self->cost);
            zsys_debug ("    addr=%ld", (long) self->addr);
            zsys_debug ("    check=%ld", (long) self->check);
            break;

        case PKGGRAPH_MSG_WORKERCANHELP:
            zsys_debug ("PKGGRAPH_MSG_WORKERCANHELP:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    addr=%ld", (long) self->addr);
            zsys_debug ("    check=%ld", (long) self->check);
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGGRAPH_MSG_FORGET_ABOUT_ME:
            zsys_debug ("PKGGRAPH_MSG_FORGET_ABOUT_ME:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    addr=%ld", (long) self->addr);
            zsys_debug ("    check=%ld", (long) self->check);
            break;

        case PKGGRAPH_MSG_RESETLOG:
            zsys_debug ("PKGGRAPH_MSG_RESETLOG:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGGRAPH_MSG_LOGHERE:
            zsys_debug ("PKGGRAPH_MSG_LOGHERE:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            zsys_debug ("    logs=[ ... ]");
            break;

        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            zsys_debug ("PKGGRAPH_MSG_UPDATE_BOOTSTRAP:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_PKGDONE:
            zsys_debug ("PKGGRAPH_MSG_PKGDONE:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGGRAPH_MSG_JOB_ENDED:
            zsys_debug ("PKGGRAPH_MSG_JOB_ENDED:");
            zsys_debug ("    proto_version=grph00");
            zsys_debug ("    addr=%ld", (long) self->addr);
            zsys_debug ("    check=%ld", (long) self->check);
            zsys_debug ("    cause=%ld", (long) self->cause);
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            break;

        case PKGGRAPH_MSG_IMTHEGRAPHER:
            zsys_debug ("PKGGRAPH_MSG_IMTHEGRAPHER:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_IAMSTORAGE:
            zsys_debug ("PKGGRAPH_MSG_IAMSTORAGE:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_IMAWORKER:
            zsys_debug ("PKGGRAPH_MSG_IMAWORKER:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_GRAPHREADY:
            zsys_debug ("PKGGRAPH_MSG_GRAPHREADY:");
            zsys_debug ("    proto_version=grph00");
            break;

        case PKGGRAPH_MSG_GRAPHNOTREADY:
            zsys_debug ("PKGGRAPH_MSG_GRAPHNOTREADY:");
            zsys_debug ("    proto_version=grph00");
            break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
pkggraph_msg_routing_id (pkggraph_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
pkggraph_msg_set_routing_id (pkggraph_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the pkggraph_msg id

int
pkggraph_msg_id (pkggraph_msg_t *self)
{
    assert (self);
    return self->id;
}

void
pkggraph_msg_set_id (pkggraph_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
pkggraph_msg_command (pkggraph_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGGRAPH_MSG_HELLO:
            return ("HELLO");
            break;
        case PKGGRAPH_MSG_ROGER:
            return ("ROGER");
            break;
        case PKGGRAPH_MSG_INVALID:
            return ("INVALID");
            break;
        case PKGGRAPH_MSG_PING:
            return ("PING");
            break;
        case PKGGRAPH_MSG_STOP:
            return ("STOP");
            break;
        case PKGGRAPH_MSG_PKGDEL:
            return ("PKGDEL");
            break;
        case PKGGRAPH_MSG_NEEDPKG:
            return ("NEEDPKG");
            break;
        case PKGGRAPH_MSG_ICANHELP:
            return ("ICANHELP");
            break;
        case PKGGRAPH_MSG_WORKERCANHELP:
            return ("WORKERCANHELP");
            break;
        case PKGGRAPH_MSG_FORGET_ABOUT_ME:
            return ("FORGET_ABOUT_ME");
            break;
        case PKGGRAPH_MSG_RESETLOG:
            return ("RESETLOG");
            break;
        case PKGGRAPH_MSG_LOGHERE:
            return ("LOGHERE");
            break;
        case PKGGRAPH_MSG_UPDATE_BOOTSTRAP:
            return ("UPDATE_BOOTSTRAP");
            break;
        case PKGGRAPH_MSG_PKGDONE:
            return ("PKGDONE");
            break;
        case PKGGRAPH_MSG_JOB_ENDED:
            return ("JOB_ENDED");
            break;
        case PKGGRAPH_MSG_IMTHEGRAPHER:
            return ("IMTHEGRAPHER");
            break;
        case PKGGRAPH_MSG_IAMSTORAGE:
            return ("IAMSTORAGE");
            break;
        case PKGGRAPH_MSG_IMAWORKER:
            return ("IMAWORKER");
            break;
        case PKGGRAPH_MSG_GRAPHREADY:
            return ("GRAPHREADY");
            break;
        case PKGGRAPH_MSG_GRAPHNOTREADY:
            return ("GRAPHNOTREADY");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the pkgname field

const char *
pkggraph_msg_pkgname (pkggraph_msg_t *self)
{
    assert (self);
    return self->pkgname;
}

void
pkggraph_msg_set_pkgname (pkggraph_msg_t *self, const char *value)
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
pkggraph_msg_version (pkggraph_msg_t *self)
{
    assert (self);
    return self->version;
}

void
pkggraph_msg_set_version (pkggraph_msg_t *self, const char *value)
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
pkggraph_msg_arch (pkggraph_msg_t *self)
{
    assert (self);
    return self->arch;
}

void
pkggraph_msg_set_arch (pkggraph_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->arch)
        return;
    strncpy (self->arch, value, 255);
    self->arch [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the hostarch field

const char *
pkggraph_msg_hostarch (pkggraph_msg_t *self)
{
    assert (self);
    return self->hostarch;
}

void
pkggraph_msg_set_hostarch (pkggraph_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->hostarch)
        return;
    strncpy (self->hostarch, value, 255);
    self->hostarch [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the targetarch field

const char *
pkggraph_msg_targetarch (pkggraph_msg_t *self)
{
    assert (self);
    return self->targetarch;
}

void
pkggraph_msg_set_targetarch (pkggraph_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->targetarch)
        return;
    strncpy (self->targetarch, value, 255);
    self->targetarch [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the iscross field

byte
pkggraph_msg_iscross (pkggraph_msg_t *self)
{
    assert (self);
    return self->iscross;
}

void
pkggraph_msg_set_iscross (pkggraph_msg_t *self, byte iscross)
{
    assert (self);
    self->iscross = iscross;
}


//  --------------------------------------------------------------------------
//  Get/set the cost field

uint16_t
pkggraph_msg_cost (pkggraph_msg_t *self)
{
    assert (self);
    return self->cost;
}

void
pkggraph_msg_set_cost (pkggraph_msg_t *self, uint16_t cost)
{
    assert (self);
    self->cost = cost;
}


//  --------------------------------------------------------------------------
//  Get/set the addr field

uint16_t
pkggraph_msg_addr (pkggraph_msg_t *self)
{
    assert (self);
    return self->addr;
}

void
pkggraph_msg_set_addr (pkggraph_msg_t *self, uint16_t addr)
{
    assert (self);
    self->addr = addr;
}


//  --------------------------------------------------------------------------
//  Get/set the check field

uint32_t
pkggraph_msg_check (pkggraph_msg_t *self)
{
    assert (self);
    return self->check;
}

void
pkggraph_msg_set_check (pkggraph_msg_t *self, uint32_t check)
{
    assert (self);
    self->check = check;
}


//  --------------------------------------------------------------------------
//  Get the logs field without transferring ownership

zchunk_t *
pkggraph_msg_logs (pkggraph_msg_t *self)
{
    assert (self);
    return self->logs;
}

//  Get the logs field and transfer ownership to caller

zchunk_t *
pkggraph_msg_get_logs (pkggraph_msg_t *self)
{
    zchunk_t *logs = self->logs;
    self->logs = NULL;
    return logs;
}

//  Set the logs field, transferring ownership from caller

void
pkggraph_msg_set_logs (pkggraph_msg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->logs);
    self->logs = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the cause field

byte
pkggraph_msg_cause (pkggraph_msg_t *self)
{
    assert (self);
    return self->cause;
}

void
pkggraph_msg_set_cause (pkggraph_msg_t *self, byte cause)
{
    assert (self);
    self->cause = cause;
}



//  --------------------------------------------------------------------------
//  Selftest

void
pkggraph_msg_test (bool verbose)
{
    printf (" * pkggraph_msg: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    pkggraph_msg_t *self = pkggraph_msg_new ();
    assert (self);
    pkggraph_msg_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-pkggraph_msg");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-pkggraph_msg");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = pkggraph_msg_new ();
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_HELLO);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_ROGER);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_INVALID);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_PING);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_STOP);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_PKGDEL);

    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_NEEDPKG);

    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_ICANHELP);

    pkggraph_msg_set_hostarch (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_targetarch (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_iscross (self, 123);
    pkggraph_msg_set_cost (self, 123);
    pkggraph_msg_set_addr (self, 123);
    pkggraph_msg_set_check (self, 123);
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_hostarch (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_targetarch (self), "Life is short but Now lasts for ever"));
        assert (pkggraph_msg_iscross (self) == 123);
        assert (pkggraph_msg_cost (self) == 123);
        assert (pkggraph_msg_addr (self) == 123);
        assert (pkggraph_msg_check (self) == 123);
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_WORKERCANHELP);

    pkggraph_msg_set_addr (self, 123);
    pkggraph_msg_set_check (self, 123);
    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (pkggraph_msg_addr (self) == 123);
        assert (pkggraph_msg_check (self) == 123);
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_FORGET_ABOUT_ME);

    pkggraph_msg_set_addr (self, 123);
    pkggraph_msg_set_check (self, 123);
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (pkggraph_msg_addr (self) == 123);
        assert (pkggraph_msg_check (self) == 123);
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_RESETLOG);

    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_LOGHERE);

    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    zchunk_t *loghere_logs = zchunk_new ("Captcha Diem", 12);
    pkggraph_msg_set_logs (self, &loghere_logs);
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
        assert (memcmp (zchunk_data (pkggraph_msg_logs (self)), "Captcha Diem", 12) == 0);
        if (instance == 1)
            zchunk_destroy (&loghere_logs);
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_UPDATE_BOOTSTRAP);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_PKGDONE);

    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_JOB_ENDED);

    pkggraph_msg_set_addr (self, 123);
    pkggraph_msg_set_check (self, 123);
    pkggraph_msg_set_cause (self, 123);
    pkggraph_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_version (self, "Life is short but Now lasts for ever");
    pkggraph_msg_set_arch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
        assert (pkggraph_msg_addr (self) == 123);
        assert (pkggraph_msg_check (self) == 123);
        assert (pkggraph_msg_cause (self) == 123);
        assert (streq (pkggraph_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkggraph_msg_arch (self), "Life is short but Now lasts for ever"));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_IMTHEGRAPHER);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_IAMSTORAGE);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_IMAWORKER);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_GRAPHREADY);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }
    pkggraph_msg_set_id (self, PKGGRAPH_MSG_GRAPHNOTREADY);

    //  Send twice
    pkggraph_msg_send (self, output);
    pkggraph_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkggraph_msg_recv (self, input);
        assert (pkggraph_msg_routing_id (self));
    }

    pkggraph_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}

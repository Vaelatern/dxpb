/*  =========================================================================
    pkgimport_msg - Package Import Managment Protocol

    Codec class for pkgimport_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_msg.xml, or
     * The code generation script that built this file: ../exec/zproto_codec_c
    ************************************************************************
    =========================================================================
*/

/*
@header
    pkgimport_msg - Package Import Managment Protocol
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/pkgimport_msg.h"

//  Structure of our class

struct _pkgimport_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  pkgimport_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char pkgname [256];                 //  pkgname
    char version [256];                 //  version
    char arch [256];                    //  arch
    char *nativehostneeds;              //  nativehostneeds
    char *nativetargetneeds;            //  nativetargetneeds
    char *crosshostneeds;               //  crosshostneeds
    char *crosstargetneeds;             //  crosstargetneeds
    char *provides;                     //  provides
    byte cancross;                      //  cancross
    byte broken;                        //  broken
    byte bootstrap;                     //  bootstrap
    byte restricted;                    //  restricted
    char commithash [256];              //  Hash corresponding to git HEAD
    char *virtualpkgs;                  //  virtualpkgs
    char depname [256];                 //  depname
    char deparch [256];                 //  deparch
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
        zsys_warning ("pkgimport_msg: GET_OCTETS failed"); \
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
        zsys_warning ("pkgimport_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("pkgimport_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("pkgimport_msg: GET_NUMBER4 failed"); \
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
        zsys_warning ("pkgimport_msg: GET_NUMBER8 failed"); \
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
        zsys_warning ("pkgimport_msg: GET_STRING failed"); \
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
        zsys_warning ("pkgimport_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new pkgimport_msg

pkgimport_msg_t *
pkgimport_msg_new (void)
{
    pkgimport_msg_t *self = (pkgimport_msg_t *) zmalloc (sizeof (pkgimport_msg_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the pkgimport_msg

void
pkgimport_msg_destroy (pkgimport_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pkgimport_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->nativehostneeds);
        free (self->nativetargetneeds);
        free (self->crosshostneeds);
        free (self->crosstargetneeds);
        free (self->provides);
        free (self->virtualpkgs);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a pkgimport_msg instance

pkgimport_msg_t *
pkgimport_msg_dup (pkgimport_msg_t *other)
{
    assert (other);
    pkgimport_msg_t *copy = pkgimport_msg_new ();

    // Copy the routing and message id
    pkgimport_msg_set_routing_id (copy, zframe_dup (pkgimport_msg_routing_id (other)));
    pkgimport_msg_set_id (copy, pkgimport_msg_id (other));

    // Copy the rest of the fields
    pkgimport_msg_set_pkgname (copy, pkgimport_msg_pkgname (other));
    pkgimport_msg_set_version (copy, pkgimport_msg_version (other));
    pkgimport_msg_set_arch (copy, pkgimport_msg_arch (other));
    {
        const char *str = pkgimport_msg_nativehostneeds(other);
        if (str) {
            pkgimport_msg_set_nativehostneeds(copy, str);
        }
    }
    {
        const char *str = pkgimport_msg_nativetargetneeds(other);
        if (str) {
            pkgimport_msg_set_nativetargetneeds(copy, str);
        }
    }
    {
        const char *str = pkgimport_msg_crosshostneeds(other);
        if (str) {
            pkgimport_msg_set_crosshostneeds(copy, str);
        }
    }
    {
        const char *str = pkgimport_msg_crosstargetneeds(other);
        if (str) {
            pkgimport_msg_set_crosstargetneeds(copy, str);
        }
    }
    {
        const char *str = pkgimport_msg_provides(other);
        if (str) {
            pkgimport_msg_set_provides(copy, str);
        }
    }
    pkgimport_msg_set_cancross (copy, pkgimport_msg_cancross (other));
    pkgimport_msg_set_broken (copy, pkgimport_msg_broken (other));
    pkgimport_msg_set_bootstrap (copy, pkgimport_msg_bootstrap (other));
    pkgimport_msg_set_restricted (copy, pkgimport_msg_restricted (other));
    pkgimport_msg_set_commithash (copy, pkgimport_msg_commithash (other));
    {
        const char *str = pkgimport_msg_virtualpkgs(other);
        if (str) {
            pkgimport_msg_set_virtualpkgs(copy, str);
        }
    }
    pkgimport_msg_set_depname (copy, pkgimport_msg_depname (other));
    pkgimport_msg_set_deparch (copy, pkgimport_msg_deparch (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a pkgimport_msg from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
pkgimport_msg_recv (pkgimport_msg_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("pkgimport_msg: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("pkgimport_msg: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1)) {
        zsys_warning ("pkgimport_msg: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case PKGIMPORT_MSG_HELLO:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_ROGER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_IFORGOTU:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_RUREADY:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_READY:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_RESET:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_INVALID:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_RDY2WRK:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_PING:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_PLZREADALL:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_KTHNKSBYE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_STOP:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_TURNONTHENEWS:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_PLZREAD:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_DIDREAD:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_PKGINFO:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            {
                char info_revision [256];
                GET_STRING (info_revision);
                if (strneq (info_revision, "00")) {
                    zsys_warning ("pkgimport_msg: info_revision is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            GET_LONGSTR (self->nativehostneeds);
            GET_LONGSTR (self->nativetargetneeds);
            GET_LONGSTR (self->crosshostneeds);
            GET_LONGSTR (self->crosstargetneeds);
            GET_LONGSTR (self->provides);
            GET_NUMBER1 (self->cancross);
            GET_NUMBER1 (self->broken);
            GET_NUMBER1 (self->bootstrap);
            GET_NUMBER1 (self->restricted);
            break;

        case PKGIMPORT_MSG_PKGDEL:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_STABLE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->commithash);
            break;

        case PKGIMPORT_MSG_UNSTABLE:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            {
                char commithash [256];
                GET_STRING (commithash);
                if (strneq (commithash, "")) {
                    zsys_warning ("pkgimport_msg: commithash is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            break;

        case PKGIMPORT_MSG_WESEEHASH:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->commithash);
            break;

        case PKGIMPORT_MSG_HEREVIRTUALPKGS:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_LONGSTR (self->virtualpkgs);
            break;

        case PKGIMPORT_MSG_VIRTPKGINFO:
            {
                char proto_version [256];
                GET_STRING (proto_version);
                if (strneq (proto_version, "READ00")) {
                    zsys_warning ("pkgimport_msg: proto_version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->pkgname);
            GET_STRING (self->version);
            GET_STRING (self->arch);
            GET_STRING (self->depname);
            GET_STRING (self->deparch);
            break;

        default:
            zsys_warning ("pkgimport_msg: bad message ID");
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
//  Send the pkgimport_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
pkgimport_msg_send (pkgimport_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case PKGIMPORT_MSG_HELLO:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_ROGER:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_IFORGOTU:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_RUREADY:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_READY:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_RESET:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_INVALID:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_RDY2WRK:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_PING:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_PLZREADALL:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_KTHNKSBYE:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_STOP:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_TURNONTHENEWS:
            frame_size += 1 + strlen ("READ00");
            break;
        case PKGIMPORT_MSG_PLZREAD:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->pkgname);
            break;
        case PKGIMPORT_MSG_DIDREAD:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->pkgname);
            break;
        case PKGIMPORT_MSG_PKGINFO:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen ("00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            frame_size += 4;
            if (self->nativehostneeds)
                frame_size += strlen (self->nativehostneeds);
            frame_size += 4;
            if (self->nativetargetneeds)
                frame_size += strlen (self->nativetargetneeds);
            frame_size += 4;
            if (self->crosshostneeds)
                frame_size += strlen (self->crosshostneeds);
            frame_size += 4;
            if (self->crosstargetneeds)
                frame_size += strlen (self->crosstargetneeds);
            frame_size += 4;
            if (self->provides)
                frame_size += strlen (self->provides);
            frame_size += 1;            //  cancross
            frame_size += 1;            //  broken
            frame_size += 1;            //  bootstrap
            frame_size += 1;            //  restricted
            break;
        case PKGIMPORT_MSG_PKGDEL:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->pkgname);
            break;
        case PKGIMPORT_MSG_STABLE:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->commithash);
            break;
        case PKGIMPORT_MSG_UNSTABLE:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen ("");
            break;
        case PKGIMPORT_MSG_WESEEHASH:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->commithash);
            break;
        case PKGIMPORT_MSG_HEREVIRTUALPKGS:
            frame_size += 1 + strlen ("READ00");
            frame_size += 4;
            if (self->virtualpkgs)
                frame_size += strlen (self->virtualpkgs);
            break;
        case PKGIMPORT_MSG_VIRTPKGINFO:
            frame_size += 1 + strlen ("READ00");
            frame_size += 1 + strlen (self->pkgname);
            frame_size += 1 + strlen (self->version);
            frame_size += 1 + strlen (self->arch);
            frame_size += 1 + strlen (self->depname);
            frame_size += 1 + strlen (self->deparch);
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
        case PKGIMPORT_MSG_HELLO:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_ROGER:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_IFORGOTU:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_RUREADY:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_READY:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_RESET:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_INVALID:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_RDY2WRK:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_PING:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_PLZREADALL:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_KTHNKSBYE:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_STOP:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_TURNONTHENEWS:
            PUT_STRING ("READ00");
            break;

        case PKGIMPORT_MSG_PLZREAD:
            PUT_STRING ("READ00");
            PUT_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_DIDREAD:
            PUT_STRING ("READ00");
            PUT_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_PKGINFO:
            PUT_STRING ("READ00");
            PUT_STRING ("00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            if (self->nativehostneeds) {
                PUT_LONGSTR (self->nativehostneeds);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            if (self->nativetargetneeds) {
                PUT_LONGSTR (self->nativetargetneeds);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            if (self->crosshostneeds) {
                PUT_LONGSTR (self->crosshostneeds);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            if (self->crosstargetneeds) {
                PUT_LONGSTR (self->crosstargetneeds);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            if (self->provides) {
                PUT_LONGSTR (self->provides);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            PUT_NUMBER1 (self->cancross);
            PUT_NUMBER1 (self->broken);
            PUT_NUMBER1 (self->bootstrap);
            PUT_NUMBER1 (self->restricted);
            break;

        case PKGIMPORT_MSG_PKGDEL:
            PUT_STRING ("READ00");
            PUT_STRING (self->pkgname);
            break;

        case PKGIMPORT_MSG_STABLE:
            PUT_STRING ("READ00");
            PUT_STRING (self->commithash);
            break;

        case PKGIMPORT_MSG_UNSTABLE:
            PUT_STRING ("READ00");
            PUT_STRING ("");
            break;

        case PKGIMPORT_MSG_WESEEHASH:
            PUT_STRING ("READ00");
            PUT_STRING (self->commithash);
            break;

        case PKGIMPORT_MSG_HEREVIRTUALPKGS:
            PUT_STRING ("READ00");
            if (self->virtualpkgs) {
                PUT_LONGSTR (self->virtualpkgs);
            }
            else
                PUT_NUMBER4 (0);    //  Empty string
            break;

        case PKGIMPORT_MSG_VIRTPKGINFO:
            PUT_STRING ("READ00");
            PUT_STRING (self->pkgname);
            PUT_STRING (self->version);
            PUT_STRING (self->arch);
            PUT_STRING (self->depname);
            PUT_STRING (self->deparch);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
pkgimport_msg_print (pkgimport_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGIMPORT_MSG_HELLO:
            zsys_debug ("PKGIMPORT_MSG_HELLO:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_ROGER:
            zsys_debug ("PKGIMPORT_MSG_ROGER:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_IFORGOTU:
            zsys_debug ("PKGIMPORT_MSG_IFORGOTU:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_RUREADY:
            zsys_debug ("PKGIMPORT_MSG_RUREADY:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_READY:
            zsys_debug ("PKGIMPORT_MSG_READY:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_RESET:
            zsys_debug ("PKGIMPORT_MSG_RESET:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_INVALID:
            zsys_debug ("PKGIMPORT_MSG_INVALID:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_RDY2WRK:
            zsys_debug ("PKGIMPORT_MSG_RDY2WRK:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_PING:
            zsys_debug ("PKGIMPORT_MSG_PING:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_PLZREADALL:
            zsys_debug ("PKGIMPORT_MSG_PLZREADALL:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            zsys_debug ("PKGIMPORT_MSG_IAMTHEGRAPHER:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_KTHNKSBYE:
            zsys_debug ("PKGIMPORT_MSG_KTHNKSBYE:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            zsys_debug ("PKGIMPORT_MSG_STABLESTATUSPLZ:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_STOP:
            zsys_debug ("PKGIMPORT_MSG_STOP:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_TURNONTHENEWS:
            zsys_debug ("PKGIMPORT_MSG_TURNONTHENEWS:");
            zsys_debug ("    proto_version=read00");
            break;

        case PKGIMPORT_MSG_PLZREAD:
            zsys_debug ("PKGIMPORT_MSG_PLZREAD:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            break;

        case PKGIMPORT_MSG_DIDREAD:
            zsys_debug ("PKGIMPORT_MSG_DIDREAD:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            break;

        case PKGIMPORT_MSG_PKGINFO:
            zsys_debug ("PKGIMPORT_MSG_PKGINFO:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    info_revision=00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            if (self->nativehostneeds)
                zsys_debug ("    nativehostneeds='%s'", self->nativehostneeds);
            else
                zsys_debug ("    nativehostneeds=");
            if (self->nativetargetneeds)
                zsys_debug ("    nativetargetneeds='%s'", self->nativetargetneeds);
            else
                zsys_debug ("    nativetargetneeds=");
            if (self->crosshostneeds)
                zsys_debug ("    crosshostneeds='%s'", self->crosshostneeds);
            else
                zsys_debug ("    crosshostneeds=");
            if (self->crosstargetneeds)
                zsys_debug ("    crosstargetneeds='%s'", self->crosstargetneeds);
            else
                zsys_debug ("    crosstargetneeds=");
            if (self->provides)
                zsys_debug ("    provides='%s'", self->provides);
            else
                zsys_debug ("    provides=");
            zsys_debug ("    cancross=%ld", (long) self->cancross);
            zsys_debug ("    broken=%ld", (long) self->broken);
            zsys_debug ("    bootstrap=%ld", (long) self->bootstrap);
            zsys_debug ("    restricted=%ld", (long) self->restricted);
            break;

        case PKGIMPORT_MSG_PKGDEL:
            zsys_debug ("PKGIMPORT_MSG_PKGDEL:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            break;

        case PKGIMPORT_MSG_STABLE:
            zsys_debug ("PKGIMPORT_MSG_STABLE:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    commithash='%s'", self->commithash);
            break;

        case PKGIMPORT_MSG_UNSTABLE:
            zsys_debug ("PKGIMPORT_MSG_UNSTABLE:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    commithash=");
            break;

        case PKGIMPORT_MSG_WESEEHASH:
            zsys_debug ("PKGIMPORT_MSG_WESEEHASH:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    commithash='%s'", self->commithash);
            break;

        case PKGIMPORT_MSG_HEREVIRTUALPKGS:
            zsys_debug ("PKGIMPORT_MSG_HEREVIRTUALPKGS:");
            zsys_debug ("    proto_version=read00");
            if (self->virtualpkgs)
                zsys_debug ("    virtualpkgs='%s'", self->virtualpkgs);
            else
                zsys_debug ("    virtualpkgs=");
            break;

        case PKGIMPORT_MSG_VIRTPKGINFO:
            zsys_debug ("PKGIMPORT_MSG_VIRTPKGINFO:");
            zsys_debug ("    proto_version=read00");
            zsys_debug ("    pkgname='%s'", self->pkgname);
            zsys_debug ("    version='%s'", self->version);
            zsys_debug ("    arch='%s'", self->arch);
            zsys_debug ("    depname='%s'", self->depname);
            zsys_debug ("    deparch='%s'", self->deparch);
            break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
pkgimport_msg_routing_id (pkgimport_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
pkgimport_msg_set_routing_id (pkgimport_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the pkgimport_msg id

int
pkgimport_msg_id (pkgimport_msg_t *self)
{
    assert (self);
    return self->id;
}

void
pkgimport_msg_set_id (pkgimport_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
pkgimport_msg_command (pkgimport_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case PKGIMPORT_MSG_HELLO:
            return ("HELLO");
            break;
        case PKGIMPORT_MSG_ROGER:
            return ("ROGER");
            break;
        case PKGIMPORT_MSG_IFORGOTU:
            return ("IFORGOTU");
            break;
        case PKGIMPORT_MSG_RUREADY:
            return ("RUREADY");
            break;
        case PKGIMPORT_MSG_READY:
            return ("READY");
            break;
        case PKGIMPORT_MSG_RESET:
            return ("RESET");
            break;
        case PKGIMPORT_MSG_INVALID:
            return ("INVALID");
            break;
        case PKGIMPORT_MSG_RDY2WRK:
            return ("RDY2WRK");
            break;
        case PKGIMPORT_MSG_PING:
            return ("PING");
            break;
        case PKGIMPORT_MSG_PLZREADALL:
            return ("PLZREADALL");
            break;
        case PKGIMPORT_MSG_IAMTHEGRAPHER:
            return ("IAMTHEGRAPHER");
            break;
        case PKGIMPORT_MSG_KTHNKSBYE:
            return ("KTHNKSBYE");
            break;
        case PKGIMPORT_MSG_STABLESTATUSPLZ:
            return ("STABLESTATUSPLZ");
            break;
        case PKGIMPORT_MSG_STOP:
            return ("STOP");
            break;
        case PKGIMPORT_MSG_TURNONTHENEWS:
            return ("TURNONTHENEWS");
            break;
        case PKGIMPORT_MSG_PLZREAD:
            return ("PLZREAD");
            break;
        case PKGIMPORT_MSG_DIDREAD:
            return ("DIDREAD");
            break;
        case PKGIMPORT_MSG_PKGINFO:
            return ("PKGINFO");
            break;
        case PKGIMPORT_MSG_PKGDEL:
            return ("PKGDEL");
            break;
        case PKGIMPORT_MSG_STABLE:
            return ("STABLE");
            break;
        case PKGIMPORT_MSG_UNSTABLE:
            return ("UNSTABLE");
            break;
        case PKGIMPORT_MSG_WESEEHASH:
            return ("WESEEHASH");
            break;
        case PKGIMPORT_MSG_HEREVIRTUALPKGS:
            return ("HEREVIRTUALPKGS");
            break;
        case PKGIMPORT_MSG_VIRTPKGINFO:
            return ("VIRTPKGINFO");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the pkgname field

const char *
pkgimport_msg_pkgname (pkgimport_msg_t *self)
{
    assert (self);
    return self->pkgname;
}

void
pkgimport_msg_set_pkgname (pkgimport_msg_t *self, const char *value)
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
pkgimport_msg_version (pkgimport_msg_t *self)
{
    assert (self);
    return self->version;
}

void
pkgimport_msg_set_version (pkgimport_msg_t *self, const char *value)
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
pkgimport_msg_arch (pkgimport_msg_t *self)
{
    assert (self);
    return self->arch;
}

void
pkgimport_msg_set_arch (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->arch)
        return;
    strncpy (self->arch, value, 255);
    self->arch [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the nativehostneeds field

const char *
pkgimport_msg_nativehostneeds (pkgimport_msg_t *self)
{
    assert (self);
    return self->nativehostneeds;
}

void
pkgimport_msg_set_nativehostneeds (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->nativehostneeds);
    self->nativehostneeds = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the nativetargetneeds field

const char *
pkgimport_msg_nativetargetneeds (pkgimport_msg_t *self)
{
    assert (self);
    return self->nativetargetneeds;
}

void
pkgimport_msg_set_nativetargetneeds (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->nativetargetneeds);
    self->nativetargetneeds = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the crosshostneeds field

const char *
pkgimport_msg_crosshostneeds (pkgimport_msg_t *self)
{
    assert (self);
    return self->crosshostneeds;
}

void
pkgimport_msg_set_crosshostneeds (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->crosshostneeds);
    self->crosshostneeds = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the crosstargetneeds field

const char *
pkgimport_msg_crosstargetneeds (pkgimport_msg_t *self)
{
    assert (self);
    return self->crosstargetneeds;
}

void
pkgimport_msg_set_crosstargetneeds (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->crosstargetneeds);
    self->crosstargetneeds = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the provides field

const char *
pkgimport_msg_provides (pkgimport_msg_t *self)
{
    assert (self);
    return self->provides;
}

void
pkgimport_msg_set_provides (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->provides);
    self->provides = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the cancross field

byte
pkgimport_msg_cancross (pkgimport_msg_t *self)
{
    assert (self);
    return self->cancross;
}

void
pkgimport_msg_set_cancross (pkgimport_msg_t *self, byte cancross)
{
    assert (self);
    self->cancross = cancross;
}


//  --------------------------------------------------------------------------
//  Get/set the broken field

byte
pkgimport_msg_broken (pkgimport_msg_t *self)
{
    assert (self);
    return self->broken;
}

void
pkgimport_msg_set_broken (pkgimport_msg_t *self, byte broken)
{
    assert (self);
    self->broken = broken;
}


//  --------------------------------------------------------------------------
//  Get/set the bootstrap field

byte
pkgimport_msg_bootstrap (pkgimport_msg_t *self)
{
    assert (self);
    return self->bootstrap;
}

void
pkgimport_msg_set_bootstrap (pkgimport_msg_t *self, byte bootstrap)
{
    assert (self);
    self->bootstrap = bootstrap;
}


//  --------------------------------------------------------------------------
//  Get/set the restricted field

byte
pkgimport_msg_restricted (pkgimport_msg_t *self)
{
    assert (self);
    return self->restricted;
}

void
pkgimport_msg_set_restricted (pkgimport_msg_t *self, byte restricted)
{
    assert (self);
    self->restricted = restricted;
}


//  --------------------------------------------------------------------------
//  Get/set the commithash field

const char *
pkgimport_msg_commithash (pkgimport_msg_t *self)
{
    assert (self);
    return self->commithash;
}

void
pkgimport_msg_set_commithash (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->commithash)
        return;
    strncpy (self->commithash, value, 255);
    self->commithash [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the virtualpkgs field

const char *
pkgimport_msg_virtualpkgs (pkgimport_msg_t *self)
{
    assert (self);
    return self->virtualpkgs;
}

void
pkgimport_msg_set_virtualpkgs (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    free (self->virtualpkgs);
    self->virtualpkgs = strdup (value);
}


//  --------------------------------------------------------------------------
//  Get/set the depname field

const char *
pkgimport_msg_depname (pkgimport_msg_t *self)
{
    assert (self);
    return self->depname;
}

void
pkgimport_msg_set_depname (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->depname)
        return;
    strncpy (self->depname, value, 255);
    self->depname [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the deparch field

const char *
pkgimport_msg_deparch (pkgimport_msg_t *self)
{
    assert (self);
    return self->deparch;
}

void
pkgimport_msg_set_deparch (pkgimport_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->deparch)
        return;
    strncpy (self->deparch, value, 255);
    self->deparch [255] = 0;
}



//  --------------------------------------------------------------------------
//  Selftest

void
pkgimport_msg_test (bool verbose)
{
    printf (" * pkgimport_msg: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    pkgimport_msg_t *self = pkgimport_msg_new ();
    assert (self);
    pkgimport_msg_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-pkgimport_msg");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-pkgimport_msg");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = pkgimport_msg_new ();
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_HELLO);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_ROGER);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_IFORGOTU);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_RUREADY);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_READY);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_RESET);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_INVALID);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_RDY2WRK);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_PING);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_PLZREADALL);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_IAMTHEGRAPHER);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_KTHNKSBYE);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_STABLESTATUSPLZ);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_STOP);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_TURNONTHENEWS);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_PLZREAD);

    pkgimport_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_pkgname (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_DIDREAD);

    pkgimport_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_pkgname (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_PKGINFO);

    pkgimport_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_arch (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_nativehostneeds (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_nativetargetneeds (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_crosshostneeds (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_crosstargetneeds (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_provides (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_cancross (self, 123);
    pkgimport_msg_set_broken (self, 123);
    pkgimport_msg_set_bootstrap (self, 123);
    pkgimport_msg_set_restricted (self, 123);
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_arch (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_nativehostneeds (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_nativetargetneeds (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_crosshostneeds (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_crosstargetneeds (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_provides (self), "Life is short but Now lasts for ever"));
        assert (pkgimport_msg_cancross (self) == 123);
        assert (pkgimport_msg_broken (self) == 123);
        assert (pkgimport_msg_bootstrap (self) == 123);
        assert (pkgimport_msg_restricted (self) == 123);
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_PKGDEL);

    pkgimport_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_pkgname (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_STABLE);

    pkgimport_msg_set_commithash (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_commithash (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_UNSTABLE);

    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_WESEEHASH);

    pkgimport_msg_set_commithash (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_commithash (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_HEREVIRTUALPKGS);

    pkgimport_msg_set_virtualpkgs (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_virtualpkgs (self), "Life is short but Now lasts for ever"));
    }
    pkgimport_msg_set_id (self, PKGIMPORT_MSG_VIRTPKGINFO);

    pkgimport_msg_set_pkgname (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_version (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_arch (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_depname (self, "Life is short but Now lasts for ever");
    pkgimport_msg_set_deparch (self, "Life is short but Now lasts for ever");
    //  Send twice
    pkgimport_msg_send (self, output);
    pkgimport_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        pkgimport_msg_recv (self, input);
        assert (pkgimport_msg_routing_id (self));
        assert (streq (pkgimport_msg_pkgname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_version (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_arch (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_depname (self), "Life is short but Now lasts for ever"));
        assert (streq (pkgimport_msg_deparch (self), "Life is short but Now lasts for ever"));
    }

    pkgimport_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}

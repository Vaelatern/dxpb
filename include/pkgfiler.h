/*  =========================================================================
    pkgfiler - Package Filer and Repository Manager

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgfiler.xml, or
     * The code generation script that built this file: ../exec/zproto_server_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGFILER_H_INCLUDED
#define PKGFILER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with pkgfiler, use the CZMQ zactor API:
//
//  Create new pkgfiler instance, passing logging prefix:
//
//      zactor_t *pkgfiler = zactor_new (pkgfiler, "myname");
//
//  Destroy pkgfiler instance
//
//      zactor_destroy (&pkgfiler);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (pkgfiler, "VERBOSE");
//
//  Bind pkgfiler to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (pkgfiler, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (pkgfiler, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (pkgfiler, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (pkgfiler, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (pkgfiler, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (pkgfiler, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to pkgfiler:
//
//      zactor_send (pkgfiler, &msg);
//
//  Receive zmsg_t instance from pkgfiler:
//
//      zmsg_t *msg = zactor_recv (pkgfiler);
//
//  This is the pkgfiler constructor as a zactor_fn:
//
void
    pkgfiler (zsock_t *pipe, void *args);

//  Self test of this class
void
    pkgfiler_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

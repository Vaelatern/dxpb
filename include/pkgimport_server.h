/*  =========================================================================
    pkgimport_server - Package Import Ventilator

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkgimport_server.xml, or
     * The code generation script that built this file: ../exec/zproto_server_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGIMPORT_SERVER_H_INCLUDED
#define PKGIMPORT_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with pkgimport_server, use the CZMQ zactor API:
//
//  Create new pkgimport_server instance, passing logging prefix:
//
//      zactor_t *pkgimport_server = zactor_new (pkgimport_server, "myname");
//
//  Destroy pkgimport_server instance
//
//      zactor_destroy (&pkgimport_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (pkgimport_server, "VERBOSE");
//
//  Bind pkgimport_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (pkgimport_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (pkgimport_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (pkgimport_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (pkgimport_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (pkgimport_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (pkgimport_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to pkgimport_server:
//
//      zactor_send (pkgimport_server, &msg);
//
//  Receive zmsg_t instance from pkgimport_server:
//
//      zmsg_t *msg = zactor_recv (pkgimport_server);
//
//  This is the pkgimport_server constructor as a zactor_fn:
//
void
    pkgimport_server (zsock_t *pipe, void *args);

//  Self test of this class
void
    pkgimport_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

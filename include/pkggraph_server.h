/*  =========================================================================
    pkggraph_server - Distributed Xbps-src Package Build Orchestrator

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: pkggraph_server.xml, or
     * The code generation script that built this file: ../exec/zproto_server_c
    ************************************************************************
    =========================================================================
*/

#ifndef PKGGRAPH_SERVER_H_INCLUDED
#define PKGGRAPH_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with pkggraph_server, use the CZMQ zactor API:
//
//  Create new pkggraph_server instance, passing logging prefix:
//
//      zactor_t *pkggraph_server = zactor_new (pkggraph_server, "myname");
//
//  Destroy pkggraph_server instance
//
//      zactor_destroy (&pkggraph_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (pkggraph_server, "VERBOSE");
//
//  Bind pkggraph_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (pkggraph_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (pkggraph_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (pkggraph_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (pkggraph_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (pkggraph_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (pkggraph_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to pkggraph_server:
//
//      zactor_send (pkggraph_server, &msg);
//
//  Receive zmsg_t instance from pkggraph_server:
//
//      zmsg_t *msg = zactor_recv (pkggraph_server);
//
//  This is the pkggraph_server constructor as a zactor_fn:
//
void
    pkggraph_server (zsock_t *pipe, void *args);

//  Self test of this class
void
    pkggraph_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

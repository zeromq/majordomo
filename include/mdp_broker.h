/*  =========================================================================
    mdp_broker - Majordomo Broker

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mdp_broker.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of CZMQ, the high-level C binding for 0MQ:       
    http://czmq.zeromq.org.                                            
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MDP_BROKER_H_INCLUDED__
#define __MDP_BROKER_H_INCLUDED__

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with mdp_broker, use the CZMQ zactor API:
//
//  Create new mdp_broker instance, passing logging prefix:
//
//      zactor_t *mdp_broker = zactor_new (mdp_broker, "myname");
//  
//  Destroy mdp_broker instance
//
//      zactor_destroy (&mdp_broker);
//  
//  Enable verbose logging of commands and activity:
//
//      zstr_send (mdp_broker, "VERBOSE");
//
//  Bind mdp_broker to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (mdp_broker, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (mdp_broker, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (mdp_broker, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (mdp_broker, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (mdp_broker, "SET", path, value, NULL);
//    
//  Save configuration data to config file on disk:
//
//      zstr_sendx (mdp_broker, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to mdp_broker:
//
//      zactor_send (mdp_broker, &msg);
//
//  Receive zmsg_t instance from mdp_broker:
//
//      zmsg_t *msg = zactor_recv (mdp_broker);
//
//  This is the mdp_broker constructor as a zactor_fn:
//
void
    mdp_broker (zsock_t *pipe, void *args);

//  Self test of this class
void
    mdp_broker_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

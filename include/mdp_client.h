/*  =========================================================================
    mdp_client - Majordomo Client

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mdp_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of CZMQ, the high-level C binding for 0MQ:       
    http://czmq.zeromq.org.                                            
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MDP_CLIENT_H_INCLUDED__
#define __MDP_CLIENT_H_INCLUDED__

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef MDP_CLIENT_T_DEFINED
typedef struct _mdp_client_t mdp_client_t;
#define MDP_CLIENT_T_DEFINED
#endif

//  @interface
//  Create a new mdp_client
//  Connect to server endpoint. Succeed if connection is successful.                
mdp_client_t *
    mdp_client_new (const char *endpoint);

//  Destroy the mdp_client
void
    mdp_client_destroy (mdp_client_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    mdp_client_actor (mdp_client_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    mdp_client_msgpipe (mdp_client_t *self);

//  Send request to broker.                                                         
int 
    mdp_client_request (mdp_client_t *self, const char *service, zmsg_t **body_p);

//  Set mdp_client_verbose.                                                         
int 
    mdp_client_set_verbose (mdp_client_t *self);

//  Return last received status
uint8_t 
    mdp_client_status (mdp_client_t *self);

//  Return last received reason
const char *
    mdp_client_reason (mdp_client_t *self);

//  Self test of this class
void
    mdp_client_test (bool verbose);
    
//  To enable verbose tracing (animation) of mdp_client instances, set
//  this to true. This lets you trace from and including construction.
extern volatile int
    mdp_client_verbose;
//  @end

#ifdef __cplusplus
}
#endif

#endif

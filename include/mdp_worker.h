/*  =========================================================================
    mdp_worker - Majordomo Worker

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mdp_worker.xml, or
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

#ifndef __MDP_WORKER_H_INCLUDED__
#define __MDP_WORKER_H_INCLUDED__

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef MDP_WORKER_T_DEFINED
typedef struct _mdp_worker_t mdp_worker_t;
#define MDP_WORKER_T_DEFINED
#endif

//  @interface
//  Create a new mdp_worker
//  Connect to server endpoint and register service. Succeed if connection is       
//  successful.                                                                     
mdp_worker_t *
    mdp_worker_new (const char *endpoint, const char *service);

//  Destroy the mdp_worker
void
    mdp_worker_destroy (mdp_worker_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    mdp_worker_actor (mdp_worker_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    mdp_worker_msgpipe (mdp_worker_t *self);

//  Send WORKER PARTIAL.                                                            
int 
    mdp_worker_send_partial (mdp_worker_t *self, zframe_t **address_p, zmsg_t **reply_body_p);

//  Send WORKER FINAL.                                                              
int 
    mdp_worker_send_final (mdp_worker_t *self, zframe_t **address_p, zmsg_t **reply_body_p);

//  Send HEARTBEAT.                                                                 
int 
    mdp_worker_send_heartbeat (mdp_worker_t *self);

//  Set mdp_worker_verbose.                                                         
int 
    mdp_worker_set_verbose (mdp_worker_t *self);

//  Return last received status
uint8_t 
    mdp_worker_status (mdp_worker_t *self);

//  Return last received reason
const char *
    mdp_worker_reason (mdp_worker_t *self);

//  Self test of this class
void
    mdp_worker_test (bool verbose);
    
//  To enable verbose tracing (animation) of mdp_worker instances, set
//  this to true. This lets you trace from and including construction.
extern volatile int
    mdp_worker_verbose;
//  @end

#ifdef __cplusplus
}
#endif

#endif

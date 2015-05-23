/*  =========================================================================
    mdp_msg - Majordomo Protocol
    
    Codec header for mdp_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mdp_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of CZMQ, the high-level C binding for 0MQ:       
    http://czmq.zeromq.org.                                            
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MDP_MSG_H_INCLUDED__
#define __MDP_MSG_H_INCLUDED__

/*  These are the mdp_msg messages:

    CLIENT_REQUEST - Client sends a request.
        version             string      MDP/Client v0.2
        messageid           number 1    REQUEST message id
        service             string      Service name
        body                msg         Request body

    CLIENT_PARTIAL - Broker sends a partial response from a worker.
        version             string      MDP/Client v0.2
        messageid           number 1    PARTIAL message id
        service             string      Service name
        body                msg         Response body

    CLIENT_FINAL - This is Majordomo Protocol version 0.2
        version             string      MDP/Client v0.2
        messageid           number 1    PARTIAL message id
        service             string      Service name
        body                msg         Response body

    READY - Worker signals the broker that it is ready to receive requests.
        version             string      MDP/Worker v0.2
        messageid           number 1    PARTIAL message id
        service             string      Service name

    WORKER_REQUEST - Broker sends a request from a client.
        version             string      MDP/Worker v0.2
        messageid           number 1    PARTIAL message id
        address             frame       Client address (envelope stack)
        empty               frame       Empty frame
        body                msg         Reply body

    WORKER_PARTIAL - Worker sends a partial reply.
        version             string      MDP/Worker v0.2
        messageid           number 1    PARTIAL message id
        address             frame       Client address (envelope stack)
        empty               frame       Empty frame
        body                msg         Reply body

    WORKER_FINAL - Worker sends a final reply.
        version             string      MDP/Worker v0.2
        messageid           number 1    FINAL message id
        address             frame       Client address (envelope stack)
        empty               frame       Empty frame
        body                msg         Reply body

    HEARTBEAT - Worker or broker sends an asynchronous heartbeat.
        version             string      MDP/Worker v0.2
        messageid           number 1    HEARTBEAT message id

    DISCONNECT - Worker or broker sends an asynchronous disconnect.
        version             string      MDP/Worker v0.2
        messageid           number 1    DISCONNECT message id
*/

#define MDP_MSG_VERSION                     2

#define MDP_MSG_CLIENT_REQUEST              1
#define MDP_MSG_CLIENT_PARTIAL              2
#define MDP_MSG_CLIENT_FINAL                3
#define MDP_MSG_READY                       4
#define MDP_MSG_WORKER_REQUEST              5
#define MDP_MSG_WORKER_PARTIAL              6
#define MDP_MSG_WORKER_FINAL                7
#define MDP_MSG_HEARTBEAT                   8
#define MDP_MSG_DISCONNECT                  9

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef MDP_MSG_T_DEFINED
typedef struct _mdp_msg_t mdp_msg_t;
#define MDP_MSG_T_DEFINED
#endif

//  @interface
//  Create a new empty mdp_msg
mdp_msg_t *
    mdp_msg_new (void);

//  Destroy a mdp_msg instance
void
    mdp_msg_destroy (mdp_msg_t **self_p);

//  Receive a mdp_msg from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.
int
    mdp_msg_recv (mdp_msg_t *self, zsock_t *input);

//  Send the mdp_msg to the output socket, does not destroy it
int
    mdp_msg_send (mdp_msg_t *self, zsock_t *output);
    
//  Print contents of message to stdout
void
    mdp_msg_print (mdp_msg_t *self);

//  Get/set the message routing id
zframe_t *
    mdp_msg_routing_id (mdp_msg_t *self);
void
    mdp_msg_set_routing_id (mdp_msg_t *self, zframe_t *routing_id);

//  Get the mdp_msg id and printable command
int
    mdp_msg_id (mdp_msg_t *self);
void
    mdp_msg_set_id (mdp_msg_t *self, int id);
const char *
    mdp_msg_command (mdp_msg_t *self);

//  Get/set the service field
const char *
    mdp_msg_service (mdp_msg_t *self);
void
    mdp_msg_set_service (mdp_msg_t *self, const char *value);

//  Get a copy of the body field
zmsg_t *
    mdp_msg_body (mdp_msg_t *self);
//  Get the body field and transfer ownership to caller
zmsg_t *
    mdp_msg_get_body (mdp_msg_t *self);
//  Set the body field, transferring ownership from caller
void
    mdp_msg_set_body (mdp_msg_t *self, zmsg_t **msg_p);

//  Get a copy of the address field
zframe_t *
    mdp_msg_address (mdp_msg_t *self);
//  Get the address field and transfer ownership to caller
zframe_t *
    mdp_msg_get_address (mdp_msg_t *self);
//  Set the address field, transferring ownership from caller
void
    mdp_msg_set_address (mdp_msg_t *self, zframe_t **frame_p);

//  Get a copy of the empty field
zframe_t *
    mdp_msg_empty (mdp_msg_t *self);
//  Get the empty field and transfer ownership to caller
zframe_t *
    mdp_msg_get_empty (mdp_msg_t *self);
//  Set the empty field, transferring ownership from caller
void
    mdp_msg_set_empty (mdp_msg_t *self, zframe_t **frame_p);

//  Self test of this class
int
    mdp_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define mdp_msg_dump        mdp_msg_print

#ifdef __cplusplus
}
#endif

#endif

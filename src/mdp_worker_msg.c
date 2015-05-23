/*  =========================================================================
    mdp_worker_msg - Majordomo Worker Protocol

    Codec class for mdp_worker_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mdp_worker_msg.xml, or
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

/*
@header
    mdp_worker_msg - Majordomo Worker Protocol
@discuss
@end
*/

#include "../include/mdp_worker_msg.h"

//  Structure of our class

struct _mdp_worker_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  mdp_worker_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    /* Service name   */
    char service [256];
    /* Client address (envelope stack)  */
    zframe_t *address;
    /* Empty frame    */
    zframe_t *empty;
    /* Request body   */
    zmsg_t *body;
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
        zsys_warning ("mdp_worker_msg: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
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
        zsys_warning ("mdp_worker_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("mdp_worker_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("mdp_worker_msg: GET_NUMBER4 failed"); \
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
        zsys_warning ("mdp_worker_msg: GET_NUMBER8 failed"); \
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
        zsys_warning ("mdp_worker_msg: GET_STRING failed"); \
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
        zsys_warning ("mdp_worker_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new mdp_worker_msg

mdp_worker_msg_t *
mdp_worker_msg_new (void)
{
    mdp_worker_msg_t *self = (mdp_worker_msg_t *) zmalloc (sizeof (mdp_worker_msg_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the mdp_worker_msg

void
mdp_worker_msg_destroy (mdp_worker_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mdp_worker_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zframe_destroy (&self->address);
        zframe_destroy (&self->empty);
        zmsg_destroy (&self->body);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive a mdp_worker_msg from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.

int
mdp_worker_msg_recv (mdp_worker_msg_t *self, zsock_t *input)
{
    assert (input);
    
    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("mdp_worker_msg: no routing ID");
            return -1;          //  Interrupted or malformed
        }
    }
    zmq_msg_t frame;
    zmq_msg_init (&frame);
    int size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("mdp_worker_msg: interrupted");
        goto malformed;         //  Interrupted
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);
    
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 4)) {
        zsys_warning ("mdp_worker_msg: invalid signature");
        //  TODO: discard invalid messages and loop, and return
        //  -1 only on interrupt
        goto malformed;         //  Interrupted
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case MDP_WORKER_MSG_READY:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 1) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            GET_STRING (self->service);
            break;

        case MDP_WORKER_MSG_WORKER_REQUEST:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 2) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: address is missing");
                goto malformed;
            }
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: empty is missing");
                goto malformed;
            }
            zframe_destroy (&self->empty);
            self->empty = zframe_recv (input);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->body);
            if (zsock_rcvmore (input))
                self->body = zmsg_recv (input);
            else
                self->body = zmsg_new ();
            break;

        case MDP_WORKER_MSG_WORKER_PARTIAL:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 3) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: address is missing");
                goto malformed;
            }
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: empty is missing");
                goto malformed;
            }
            zframe_destroy (&self->empty);
            self->empty = zframe_recv (input);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->body);
            if (zsock_rcvmore (input))
                self->body = zmsg_recv (input);
            else
                self->body = zmsg_new ();
            break;

        case MDP_WORKER_MSG_WORKER_FINAL:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 4) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: address is missing");
                goto malformed;
            }
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("mdp_worker_msg: empty is missing");
                goto malformed;
            }
            zframe_destroy (&self->empty);
            self->empty = zframe_recv (input);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->body);
            if (zsock_rcvmore (input))
                self->body = zmsg_recv (input);
            else
                self->body = zmsg_new ();
            break;

        case MDP_WORKER_MSG_HEARTBEAT:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 5) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            break;

        case MDP_WORKER_MSG_DISCONNECT:
            {
                char version [256];
                GET_STRING (version);
                if (strneq (version, "MDPW02")) {
                    zsys_warning ("mdp_worker_msg: version is invalid");
                    goto malformed;
                }
            }
            {
                byte messageid;
                GET_NUMBER1 (messageid);
                if (messageid != 6) {
                    zsys_warning ("mdp_worker_msg: messageid is invalid");
                    goto malformed;
                }
            }
            break;

        default:
            zsys_warning ("mdp_worker_msg: bad message ID");
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return 0;

    //  Error returns
    malformed:
        zsys_warning ("mdp_worker_msg: mdp_worker_msg malformed message, fail");
        zmq_msg_close (&frame);
        return -1;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the mdp_worker_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
mdp_worker_msg_send (mdp_worker_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case MDP_WORKER_MSG_READY:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            frame_size += 1 + strlen (self->service);
            break;
        case MDP_WORKER_MSG_WORKER_REQUEST:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            break;
        case MDP_WORKER_MSG_WORKER_PARTIAL:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            break;
        case MDP_WORKER_MSG_WORKER_FINAL:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            break;
        case MDP_WORKER_MSG_HEARTBEAT:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            break;
        case MDP_WORKER_MSG_DISCONNECT:
            frame_size += 1 + strlen ("MDPW02");
            frame_size += 1;            //  messageid
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 4);
    PUT_NUMBER1 (self->id);
    bool send_body = false;
    size_t nbr_frames = 1;              //  Total number of frames to send
    
    switch (self->id) {
        case MDP_WORKER_MSG_READY:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (1);
            PUT_STRING (self->service);
            break;

        case MDP_WORKER_MSG_WORKER_REQUEST:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (2);
            nbr_frames++;
            nbr_frames++;
            nbr_frames += self->body? zmsg_size (self->body): 1;
            send_body = true;
            break;

        case MDP_WORKER_MSG_WORKER_PARTIAL:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (3);
            nbr_frames++;
            nbr_frames++;
            nbr_frames += self->body? zmsg_size (self->body): 1;
            send_body = true;
            break;

        case MDP_WORKER_MSG_WORKER_FINAL:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (4);
            nbr_frames++;
            nbr_frames++;
            nbr_frames += self->body? zmsg_size (self->body): 1;
            send_body = true;
            break;

        case MDP_WORKER_MSG_HEARTBEAT:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (5);
            break;

        case MDP_WORKER_MSG_DISCONNECT:
            PUT_STRING ("MDPW02");
            PUT_NUMBER1 (6);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);
    
    //  Now send any frame fields, in order
    if (self->id == MDP_WORKER_MSG_WORKER_REQUEST) {
        //  If address isn't set, send an empty frame
        if (self->address)
            zframe_send (&self->address, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
        //  If empty isn't set, send an empty frame
        if (self->empty)
            zframe_send (&self->empty, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send any frame fields, in order
    if (self->id == MDP_WORKER_MSG_WORKER_PARTIAL) {
        //  If address isn't set, send an empty frame
        if (self->address)
            zframe_send (&self->address, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
        //  If empty isn't set, send an empty frame
        if (self->empty)
            zframe_send (&self->empty, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send any frame fields, in order
    if (self->id == MDP_WORKER_MSG_WORKER_FINAL) {
        //  If address isn't set, send an empty frame
        if (self->address)
            zframe_send (&self->address, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
        //  If empty isn't set, send an empty frame
        if (self->empty)
            zframe_send (&self->empty, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send the body if necessary
    if (send_body) {
        if (self->body) {
            zframe_t *frame = zmsg_first (self->body);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->body);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
mdp_worker_msg_print (mdp_worker_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case MDP_WORKER_MSG_READY:
            zsys_debug ("MDP_WORKER_MSG_READY:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=1");
            if (self->service)
                zsys_debug ("    service='%s'", self->service);
            else
                zsys_debug ("    service=");
            break;
            
        case MDP_WORKER_MSG_WORKER_REQUEST:
            zsys_debug ("MDP_WORKER_MSG_WORKER_REQUEST:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=2");
            zsys_debug ("    address=");
            if (self->address)
                zframe_print (self->address, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    empty=");
            if (self->empty)
                zframe_print (self->empty, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    body=");
            if (self->body)
                zmsg_print (self->body);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MDP_WORKER_MSG_WORKER_PARTIAL:
            zsys_debug ("MDP_WORKER_MSG_WORKER_PARTIAL:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=3");
            zsys_debug ("    address=");
            if (self->address)
                zframe_print (self->address, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    empty=");
            if (self->empty)
                zframe_print (self->empty, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    body=");
            if (self->body)
                zmsg_print (self->body);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MDP_WORKER_MSG_WORKER_FINAL:
            zsys_debug ("MDP_WORKER_MSG_WORKER_FINAL:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=4");
            zsys_debug ("    address=");
            if (self->address)
                zframe_print (self->address, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    empty=");
            if (self->empty)
                zframe_print (self->empty, NULL);
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    body=");
            if (self->body)
                zmsg_print (self->body);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MDP_WORKER_MSG_HEARTBEAT:
            zsys_debug ("MDP_WORKER_MSG_HEARTBEAT:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=5");
            break;
            
        case MDP_WORKER_MSG_DISCONNECT:
            zsys_debug ("MDP_WORKER_MSG_DISCONNECT:");
            zsys_debug ("    version=mdpw02");
            zsys_debug ("    messageid=6");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
mdp_worker_msg_routing_id (mdp_worker_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
mdp_worker_msg_set_routing_id (mdp_worker_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the mdp_worker_msg id

int
mdp_worker_msg_id (mdp_worker_msg_t *self)
{
    assert (self);
    return self->id;
}

void
mdp_worker_msg_set_id (mdp_worker_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
mdp_worker_msg_command (mdp_worker_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case MDP_WORKER_MSG_READY:
            return ("READY");
            break;
        case MDP_WORKER_MSG_WORKER_REQUEST:
            return ("WORKER_REQUEST");
            break;
        case MDP_WORKER_MSG_WORKER_PARTIAL:
            return ("WORKER_PARTIAL");
            break;
        case MDP_WORKER_MSG_WORKER_FINAL:
            return ("WORKER_FINAL");
            break;
        case MDP_WORKER_MSG_HEARTBEAT:
            return ("HEARTBEAT");
            break;
        case MDP_WORKER_MSG_DISCONNECT:
            return ("DISCONNECT");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the service field

const char *
mdp_worker_msg_service (mdp_worker_msg_t *self)
{
    assert (self);
    return self->service;
}

void
mdp_worker_msg_set_service (mdp_worker_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->service)
        return;
    strncpy (self->service, value, 255);
    self->service [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get the address field without transferring ownership

zframe_t *
mdp_worker_msg_address (mdp_worker_msg_t *self)
{
    assert (self);
    return self->address;
}

//  Get the address field and transfer ownership to caller

zframe_t *
mdp_worker_msg_get_address (mdp_worker_msg_t *self)
{
    zframe_t *address = self->address;
    self->address = NULL;
    return address;
}

//  Set the address field, transferring ownership from caller

void
mdp_worker_msg_set_address (mdp_worker_msg_t *self, zframe_t **frame_p)
{
    assert (self);
    assert (frame_p);
    zframe_destroy (&self->address);
    self->address = *frame_p;
    *frame_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the empty field without transferring ownership

zframe_t *
mdp_worker_msg_empty (mdp_worker_msg_t *self)
{
    assert (self);
    return self->empty;
}

//  Get the empty field and transfer ownership to caller

zframe_t *
mdp_worker_msg_get_empty (mdp_worker_msg_t *self)
{
    zframe_t *empty = self->empty;
    self->empty = NULL;
    return empty;
}

//  Set the empty field, transferring ownership from caller

void
mdp_worker_msg_set_empty (mdp_worker_msg_t *self, zframe_t **frame_p)
{
    assert (self);
    assert (frame_p);
    zframe_destroy (&self->empty);
    self->empty = *frame_p;
    *frame_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the body field without transferring ownership

zmsg_t *
mdp_worker_msg_body (mdp_worker_msg_t *self)
{
    assert (self);
    return self->body;
}

//  Get the body field and transfer ownership to caller

zmsg_t *
mdp_worker_msg_get_body (mdp_worker_msg_t *self)
{
    zmsg_t *body = self->body;
    self->body = NULL;
    return body;
}

//  Set the body field, transferring ownership from caller

void
mdp_worker_msg_set_body (mdp_worker_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->body);
    self->body = *msg_p;
    *msg_p = NULL;
}



//  --------------------------------------------------------------------------
//  Selftest

int
mdp_worker_msg_test (bool verbose)
{
    printf (" * mdp_worker_msg: ");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    mdp_worker_msg_t *self = mdp_worker_msg_new ();
    assert (self);
    mdp_worker_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-mdp_worker_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-mdp_worker_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    self = mdp_worker_msg_new ();
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_READY);

    mdp_worker_msg_set_service (self, "Life is short but Now lasts for ever");
    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
        assert (streq (mdp_worker_msg_service (self), "Life is short but Now lasts for ever"));
    }
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_WORKER_REQUEST);

    zframe_t *worker_request_address = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_address (self, &worker_request_address);
    zframe_t *worker_request_empty = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_empty (self, &worker_request_empty);
    zmsg_t *worker_request_body = zmsg_new ();
    mdp_worker_msg_set_body (self, &worker_request_body);
    zmsg_addstr (mdp_worker_msg_body (self), "Hello, World");
    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
        assert (zframe_streq (mdp_worker_msg_address (self), "Captcha Diem"));
        assert (zframe_streq (mdp_worker_msg_empty (self), "Captcha Diem"));
        assert (zmsg_size (mdp_worker_msg_body (self)) == 1);
    }
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_WORKER_PARTIAL);

    zframe_t *worker_partial_address = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_address (self, &worker_partial_address);
    zframe_t *worker_partial_empty = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_empty (self, &worker_partial_empty);
    zmsg_t *worker_partial_body = zmsg_new ();
    mdp_worker_msg_set_body (self, &worker_partial_body);
    zmsg_addstr (mdp_worker_msg_body (self), "Hello, World");
    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
        assert (zframe_streq (mdp_worker_msg_address (self), "Captcha Diem"));
        assert (zframe_streq (mdp_worker_msg_empty (self), "Captcha Diem"));
        assert (zmsg_size (mdp_worker_msg_body (self)) == 1);
    }
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_WORKER_FINAL);

    zframe_t *worker_final_address = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_address (self, &worker_final_address);
    zframe_t *worker_final_empty = zframe_new ("Captcha Diem", 12);
    mdp_worker_msg_set_empty (self, &worker_final_empty);
    zmsg_t *worker_final_body = zmsg_new ();
    mdp_worker_msg_set_body (self, &worker_final_body);
    zmsg_addstr (mdp_worker_msg_body (self), "Hello, World");
    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
        assert (zframe_streq (mdp_worker_msg_address (self), "Captcha Diem"));
        assert (zframe_streq (mdp_worker_msg_empty (self), "Captcha Diem"));
        assert (zmsg_size (mdp_worker_msg_body (self)) == 1);
    }
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_HEARTBEAT);

    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
    }
    mdp_worker_msg_set_id (self, MDP_WORKER_MSG_DISCONNECT);

    //  Send twice
    mdp_worker_msg_send (self, output);
    mdp_worker_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        mdp_worker_msg_recv (self, input);
        assert (mdp_worker_msg_routing_id (self));
    }

    mdp_worker_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}

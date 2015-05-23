/*  =========================================================================
    mdp_client - Majordomo Client

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of FileMQ, a C implemenation of the protocol:    
    https://github.com/danriegsecker/filemq2.                          
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

//  TODO: Change these to match your project's needs
#include "../include/mdp_client_msg.h"
#include "../include/mdp_client.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine. The cmdpipe gets
    //  messages sent to the actor; the msgpipe may be used for
    //  faster asynchronous message flows.
    zsock_t *cmdpipe;           //  Command pipe to/from caller API
    zsock_t *msgpipe;           //  Message pipe to/from caller API
    zsock_t *dealer;            //  Socket to talk to server
    mdp_client_msg_t *message;  //  Message to/from server
    client_args_t *args;        //  Arguments from methods
    
    //  TODO: Add specific properties for your application
} client_t;

//  Include the generated client engine
#include "mdp_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}


//  ---------------------------------------------------------------------------
//  Selftest

void
mdp_client_test (bool verbose)
{
    printf (" * mdp_client: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    zactor_t *client = zactor_new (mdp_client, NULL);
    if (verbose)
        zstr_send (client, "VERBOSE");
    zactor_destroy (&client);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  connect_to_server
//

static void
connect_to_server (client_t *self)
{
    if (zsock_connect(self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception(self, connect_error_event);
        zsys_warning("could not connect to %s", self->args->endpoint);
        zsock_send (self->cmdpipe, "si", "FAILURE", 0);
    }
    else
    {
        zsys_debug("connected to %s", self->args->endpoint);
        zsock_send (self->cmdpipe, "si", "SUCCESS", 0);
    }
}


//  ---------------------------------------------------------------------------
//  signal_connection_success
//

static void
signal_connection_success (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  send_request_to_broker
//

static void
send_request_to_broker (client_t *self)
{
    mdp_client_msg_t *msg;

    msg = mdp_client_msg_new();
    assert(msg);
    mdp_client_msg_set_id(msg, MDP_CLIENT_MSG_CLIENT_REQUEST);
    mdp_client_msg_set_service(msg, self->args->service);
    mdp_client_msg_set_body(msg, &self->args->body);
    mdp_client_msg_send(msg, self->dealer);
    mdp_client_msg_destroy(&msg);
}


//  ---------------------------------------------------------------------------
//  disconnect_from_broker
//

static void
disconnect_from_broker (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  send_partial_response
//

static void
send_partial_response (client_t *self)
{
    zmsg_t *body = mdp_client_msg_get_body(self->message);
    zsock_send(self->msgpipe, "sm", "PARTIAL", body);
}


//  ---------------------------------------------------------------------------
//  send_final_response
//

static void
send_final_response (client_t *self)
{
    zmsg_t *body = mdp_client_msg_get_body(self->message);
    zsock_send(self->msgpipe, "sm", "FINAL", body);
}


//  ---------------------------------------------------------------------------
//  log_protocol_error
//

static void
log_protocol_error (client_t *self)
{
    zsys_error("** protocol error **");
    mdp_client_msg_print(self->message);
}


//  ---------------------------------------------------------------------------
//  signal_connect_error
//

static void
signal_connect_error (client_t *self)
{
    zsock_send(self->cmdpipe, "si", "CONNECT ERROR", 0);
}


//  ---------------------------------------------------------------------------
//  handle_set_verbose
//

static void
handle_set_verbose (client_t *self)
{
    mdp_client_verbose = true;
}

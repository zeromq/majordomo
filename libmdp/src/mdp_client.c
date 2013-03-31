/*  =========================================================================
    mdp_client.h - client API

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the Majordomo Project: http://majordomo.zeromq.org,
    an implementation of rfc.zeromq.org/spec:18/MDP (MDP/0.2) in C.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include "../include/mdp_common.h"
#include "../include/mdp_client.h"

//  Structure of our class
//  We access these properties only via class methods

struct _mdp_client_t {
    zctx_t *ctx;                //  Our context
    char *broker;
    void *client;               //  Socket to broker
    int verbose;                //  Print activity to stdout
    int timeout;                //  Request timeout
};


//  ---------------------------------------------------------------------
//  Connect or reconnect to broker

void s_mdp_client_connect_to_broker (mdp_client_t *self)
{
    if (self->client)
        zsocket_destroy (self->ctx, self->client);
    self->client = zsocket_new (self->ctx, ZMQ_DEALER);
    zmq_connect (self->client, self->broker);
    if (self->verbose)
        zclock_log ("I: connecting to broker at %s...", self->broker);
}


//  ---------------------------------------------------------------------
//  Constructor

mdp_client_t *
mdp_client_new (char *broker, int verbose)
{
    assert (broker);

    mdp_client_t *self = (mdp_client_t *) zmalloc (sizeof (mdp_client_t));
    self->ctx = zctx_new ();
    self->broker = strdup (broker);
    self->verbose = verbose;
    self->timeout = 2500;           //  msecs

    s_mdp_client_connect_to_broker (self);
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor

void
mdp_client_destroy (mdp_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mdp_client_t *self = *self_p;
        zctx_destroy (&self->ctx);
        free (self->broker);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Set request timeout

void
mdp_client_set_timeout (mdp_client_t *self, int timeout)
{
    assert (self);
    self->timeout = timeout;
}


//  ---------------------------------------------------------------------
//  Set client socket option

int
mdp_client_setsockopt (mdp_client_t *self, int option, const void *optval, size_t optvallen)
{
    assert (self);
    assert (self->client);
    return zmq_setsockopt (self->client, option, optval, optvallen);
}


//  ---------------------------------------------------------------------
//  Get client socket option

int
mdp_client_getsockopt (mdp_client_t *self, 	int option, void *optval, size_t *optvallen)
{
    assert (self);
    assert (self->client);
    return zmq_getsockopt (self->client, option, optval, optvallen);
}


//  Here is the send method. It sends a request to the broker.
//  It takes ownership of the request message, and destroys it when sent.

void
mdp_client_send (mdp_client_t *self, char *service, zmsg_t **request_p)
{
    assert (self);
    assert (request_p);
    zmsg_t *request = *request_p;

    //  Prefix request with protocol frames
    //  Frame 1: empty frame (delimiter)
    //  Frame 2: "MDPCxy" (six bytes, MDP/Client x.y)
    //  Frame 3: Service name (printable string)
    zmsg_pushstr (request, service);
    zmsg_pushstr (request, MDPC_CLIENT);
    zmsg_pushstr (request, "");
    if (self->verbose) {
        zclock_log ("I: send request to '%s' service:", service);
        zmsg_dump (request);
    }
    zmsg_send (request_p, self->client);
}

//  Receive report from the broker.
//  The caller is responsible for destroying the received message.
//  If service is not NULL, it is filled in with a pointer
//  to service string. It is caller's responsibility to free it.

zmsg_t *
mdp_client_recv (mdp_client_t *self, char **command_p, char **service_p)
{
    assert (self);

    zmsg_t *msg = zmsg_recv (self->client);
    if (msg == NULL)
        //  Interrupt
        return NULL;

    if (self->verbose) {
        zclock_log ("I: received reply:");
        zmsg_dump (msg);
    }

    //  Message format:
    //  Frame 1: empty frame (delimiter)
    //  Frame 2: "MDPCxy" (six bytes, MDP/Client x.y)
    //  Frame 3: REPORT|NAK
    //  Frame 4: Service name (printable string)
    //  Frame 5..n: Application frames

    //  We would handle malformed replies better in real code
    assert (zmsg_size (msg) >= 5);

    zframe_t *empty = zmsg_pop (msg);
    assert (zframe_streq (empty, ""));
    zframe_destroy (&empty);

    zframe_t *header = zmsg_pop (msg);
    assert (zframe_streq (header, MDPC_CLIENT));
    zframe_destroy (&header);

    zframe_t *command = zmsg_pop (msg);
    assert (zframe_streq (command, MDPC_REPORT) ||
            zframe_streq (command, MDPC_NAK));
    if (command_p)
        *command_p = zframe_strdup (command);
    zframe_destroy (&command);

    zframe_t *service = zmsg_pop (msg);
    if (service_p)
        *service_p = zframe_strdup (service);
    zframe_destroy (&service);

    return msg;     //  Success
}

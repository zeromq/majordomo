/*  =========================================================================
    mdp_worker.h - worker API

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
#include "../include/mdp_worker.h"

//  Reliability parameters
#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable

//  This is the structure of a worker API instance. We use a pseudo-OO
//  approach in a lot of the C examples, as well as the CZMQ binding:

//  Structure of our class
//  We access these properties only via class methods

struct _mdp_worker_t {
    zctx_t *ctx;                //  Our context
    bool local_ctx;             //  Indicates if the Context belongs to us
    char *broker;
    char *service;
    void *worker;               //  Socket to broker
    int verbose;                //  Print activity to stdout

    //  Heartbeat management
    uint64_t heartbeat_at;      //  When to send HEARTBEAT
    size_t liveness;            //  How many attempts left
    int heartbeat;              //  Heartbeat delay, msecs
    int reconnect;              //  Reconnect delay, msecs
};


//  We have two utility functions; to send a message to the broker and
//  to (re-)connect to the broker.

//  ---------------------------------------------------------------------
//  Send message to broker
//  If no msg is provided, creates one internally

static void
s_mdp_worker_send_to_broker (mdp_worker_t *self, char *command, char *option,
                        zmsg_t *msg)
{
    msg = msg? zmsg_dup (msg): zmsg_new ();

    //  Stack protocol envelope to start of message
    if (option)
        zmsg_pushstr (msg, option);
    zmsg_pushstr (msg, command);
    zmsg_pushstr (msg, MDPW_WORKER);
    zmsg_pushstr (msg, "");

    if (self->verbose) {
        zclock_log ("I: sending %s to broker",
            mdpw_commands [(int) *command]);
        zmsg_dump (msg);
    }
    zmsg_send (&msg, self->worker);
}


//  ---------------------------------------------------------------------
//  Connect or reconnect to broker

void s_mdp_worker_connect_to_broker (mdp_worker_t *self)
{
    if (self->worker)
        zsocket_destroy (self->ctx, self->worker);
    self->worker = zsocket_new (self->ctx, ZMQ_DEALER);
    zmq_connect (self->worker, self->broker);
    if (self->verbose)
        zclock_log ("I: connecting to broker at %s...", self->broker);

    //  Register service with broker
    s_mdp_worker_send_to_broker (self, MDPW_READY, self->service, NULL);

    //  If liveness hits zero, worker is considered disconnected
    self->liveness = HEARTBEAT_LIVENESS;
    self->heartbeat_at = zclock_time () + self->heartbeat;
}


//  Here we have the constructor and destructor for our mdp_worker class

//  ---------------------------------------------------------------------
//  Constructor

mdp_worker_t *
mdp_worker_new (zctx_t *ctx, char *broker,char *service, int verbose)
{
    assert (broker);
    assert (service);

    mdp_worker_t *self = (mdp_worker_t *) zmalloc (sizeof (mdp_worker_t));

    if (ctx) {
        self->ctx = ctx;
        self->local_ctx = false;
    }
    else {
        self->ctx = zctx_new ();
        self->local_ctx = true;
    }

    self->broker = strdup (broker);
    self->service = strdup (service);
    self->verbose = verbose;
    self->heartbeat = 2500;     //  msecs
    self->reconnect = 2500;     //  msecs

    // A non-zero linger value is required for DISCONNECT to be sent
    // when the worker is destroyed.  100 is arbitrary but chosen to be
    // sufficient for common cases without significant delay in broken ones.
    if (self->local_ctx) {
        zctx_set_linger (self->ctx, 100);
    }

    s_mdp_worker_connect_to_broker (self);
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor

void
mdp_worker_destroy (mdp_worker_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mdp_worker_t *self = *self_p;

        s_mdp_worker_send_to_broker (self, MDPW_DISCONNECT, NULL, NULL);

        if (self->local_ctx) {
            zctx_destroy (&self->ctx);
        }

        free (self->broker);
        free (self->service);
        free (self);
        *self_p = NULL;
    }
}


//  We provide two methods to configure the worker API. You can set the
//  heartbeat interval and retries to match the expected network performance.

//  ---------------------------------------------------------------------
//  Set heartbeat delay

void
mdp_worker_set_heartbeat (mdp_worker_t *self, int heartbeat)
{
    self->heartbeat = heartbeat;
}


//  ---------------------------------------------------------------------
//  Set linger time

void
mdp_worker_set_linger (mdp_worker_t *self, int linger)
{
    if (self->local_ctx) {
        zctx_set_linger (self->ctx, linger);
    }
}


//  ---------------------------------------------------------------------
//  Set reconnect delay

void
mdp_worker_set_reconnect (mdp_worker_t *self, int reconnect)
{
    self->reconnect = reconnect;
}


//  ---------------------------------------------------------------------
//  Set worker socket option

int
mdp_worker_setsockopt (mdp_worker_t *self, int option, const void *optval, size_t optvallen)
{
    assert (self);
    assert (self->worker);
    return zmq_setsockopt (self->worker, option, optval, optvallen);
}


//  ---------------------------------------------------------------------
//  Get worker socket option

int
mdp_worker_getsockopt (mdp_worker_t *self, 	int option, void *optval, size_t *optvallen)
{
    assert (self);
    assert (self->worker);
    return zmq_getsockopt (self->worker, option, optval, optvallen);
}


//  This is the recv method; it receives a new request from a client.
//  If reply_to_p is not NULL, a pointer to client's address is filled in.

//  ---------------------------------------------------------------------
//  Wait for a new request.

zmsg_t *
mdp_worker_recv (mdp_worker_t *self, zframe_t **reply_to_p)
{
    while (true) {
        zmq_pollitem_t items [] = {
            { self->worker,  0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, self->heartbeat * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (self->worker);
            if (!msg)
                break;          //  Interrupted
            if (self->verbose) {
                zclock_log ("I: received message from broker:");
                zmsg_dump (msg);
            }
            self->liveness = HEARTBEAT_LIVENESS;

            //  Don't try to handle errors, just assert noisily
            assert (zmsg_size (msg) >= 3);

            zframe_t *empty = zmsg_pop (msg);
            assert (zframe_streq (empty, ""));
            zframe_destroy (&empty);

            zframe_t *header = zmsg_pop (msg);
            assert (zframe_streq (header, MDPW_WORKER));
            zframe_destroy (&header);

            zframe_t *command = zmsg_pop (msg);
            if (zframe_streq (command, MDPW_REQUEST)) {
                //  We should pop and save as many addresses as there are
                //  up to a null part, but for now, just save one...
                zframe_t *reply_to = zmsg_unwrap (msg);
                if (reply_to_p)
                    *reply_to_p = reply_to;
                else
                    zframe_destroy (&reply_to);

                zframe_destroy (&command);
                //  Here is where we actually have a message to process; we
                //  return it to the caller application
                return msg;     //  We have a request to process
            }
            else
            if (zframe_streq (command, MDPW_HEARTBEAT))
                ;               //  Do nothing for heartbeats
            else
            if (zframe_streq (command, MDPW_DISCONNECT))
                s_mdp_worker_connect_to_broker (self);
            else {
                zclock_log ("E: invalid input message");
                zmsg_dump (msg);
            }
            zframe_destroy (&command);
            zmsg_destroy (&msg);
        }
        else
        if (--self->liveness == 0) {
            if (self->verbose)
                zclock_log ("W: disconnected from broker - retrying...");
            zclock_sleep (self->reconnect);
            s_mdp_worker_connect_to_broker (self);
        }
        //  Send HEARTBEAT if it's time
        if (zclock_time () > self->heartbeat_at) {
            s_mdp_worker_send_to_broker (self, MDPW_HEARTBEAT, NULL, NULL);
            self->heartbeat_at = zclock_time () + self->heartbeat;
        }
    }
    if (zctx_interrupted)
        printf ("W: interrupt received, killing worker...\n");
    return NULL;
}


//  ---------------------------------------------------------------------
//  Send a report to the client.

void
mdp_worker_send (mdp_worker_t *self, zmsg_t **report_p, zframe_t *reply_to)
{
    assert (report_p);
    zmsg_t *report = *report_p;
    assert (report);
    assert (reply_to);
    // Add client address
    zmsg_wrap (report, zframe_dup (reply_to));
    s_mdp_worker_send_to_broker (self, MDPW_REPORT, NULL, report);
    zmsg_destroy (report_p);
}

/*  =========================================================================
    mdp_broker.c - simple Majordomo Protocol broker

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the Majordomo Project: http://majordomo.zeromq.org,
    an implementation of rfc.zeromq.org/spec:18/MDP (MDP/0.2) in C.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.
    =========================================================================
*/


//
//  Majordomo Protocol broker
//  A minimal C implementation of the Majordomo Protocol as defined in
//  http://rfc.zeromq.org/spec:7 and http://rfc.zeromq.org/spec:8.
//
#include <unistd.h>
#include "../include/mdp_common.h"

//  We'd normally pull these from config data

#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500    //  msecs
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

//  The broker class defines a single broker instance

typedef struct {
    zctx_t *ctx;                //  Our context
    void *socket;               //  Socket for clients & workers
    int verbose;                //  Print activity to stdout
    char *endpoint;             //  Broker binds to this endpoint
    zhash_t *services;          //  Hash of known services
    zhash_t *workers;           //  Hash of known workers
    zlist_t *waiting;           //  List of waiting workers
    uint64_t heartbeat_at;      //  When to send HEARTBEAT
} broker_t;

static broker_t *
    s_broker_new (int verbose);
static void
    s_broker_destroy (broker_t **self_p);
static void
    s_broker_bind (broker_t *self, char *endpoint);
static void
    s_broker_worker_msg (broker_t *self, zframe_t *sender, zmsg_t *msg);
static void
    s_broker_client_msg (broker_t *self, zframe_t *sender, zmsg_t *msg);
static void
    s_broker_purge (broker_t *self);

//  The service class defines a single service instance

typedef struct {
    broker_t *broker;           //  Broker instance
    char *name;                 //  Service name
    zlist_t *requests;          //  List of client requests
    zlist_t *waiting;           //  List of waiting workers
    size_t workers;             //  How many workers we have
    zlist_t *blacklist;
} service_t;

static service_t *
    s_service_require (broker_t *self, zframe_t *service_frame);
static void
    s_service_destroy (void *argument);
static void
    s_service_dispatch (service_t *service);
static void
    s_service_enable_command (service_t *self, const char *command);
static void
    s_service_disable_command (service_t *self, const char *command);
static int
    s_service_is_command_enabled (service_t *self, const char *command);


//  The worker class defines a single worker, idle or active

typedef struct {
    broker_t *broker;           //  Broker instance
    char *identity;             //  Identity of worker
    zframe_t *address;          //  Address frame to route to
    service_t *service;         //  Owning service, if known
    int64_t expiry;             //  Expires at unless heartbeat
} worker_t;

static worker_t *
    s_worker_require (broker_t *self, zframe_t *address);
static void
    s_worker_delete (worker_t *self, int disconnect);
static void
    s_worker_destroy (void *argument);
static void
    s_worker_send (worker_t *self, char *command, char *option,
                   zmsg_t *msg);
static void
    s_worker_waiting (worker_t *self);

//  Here are the constructor and destructor for the broker

static broker_t *
s_broker_new (int verbose)
{
    broker_t *self = (broker_t *) zmalloc (sizeof (broker_t));

    //  Initialize broker state
    self->ctx = zctx_new ();
    self->socket = zsocket_new (self->ctx, ZMQ_ROUTER);
    self->verbose = verbose;
    self->services = zhash_new ();
    self->workers = zhash_new ();
    self->waiting = zlist_new ();
    self->heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;
    return self;
}

static void
s_broker_destroy (broker_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        broker_t *self = *self_p;
        zctx_destroy (&self->ctx);
        zhash_destroy (&self->services);
        zhash_destroy (&self->workers);
        zlist_destroy (&self->waiting);
        free (self);
        *self_p = NULL;
    }
}

//  The bind method binds the broker instance to an endpoint. We can call
//  this multiple times. Note that MDP uses a single socket for both clients 
//  and workers.

void
s_broker_bind (broker_t *self, char *endpoint)
{
    zsocket_bind (self->socket, endpoint);
    zclock_log ("I: MDP broker/0.2.0 is active at %s", endpoint);
}

//  The worker_msg method processes one READY, REPORT, HEARTBEAT or
//  DISCONNECT message sent to the broker by a worker

static void
s_broker_worker_msg (broker_t *self, zframe_t *sender, zmsg_t *msg)
{
    assert (zmsg_size (msg) >= 1);     //  At least, command

    zframe_t *command = zmsg_pop (msg);
    char *identity = zframe_strhex (sender);
    int worker_ready = (zhash_lookup (self->workers, identity) != NULL);
    free (identity);
    worker_t *worker = s_worker_require (self, sender);

    if (zframe_streq (command, MDPW_READY)) {
        if (worker_ready)               //  Not first command in session
            s_worker_delete (worker, 1);
        else
        if (zframe_size (sender) >= 4  //  Reserved service name
        &&  memcmp (zframe_data (sender), "mmi.", 4) == 0)
            s_worker_delete (worker, 1);
        else {
            //  Attach worker to service and mark as idle
            zframe_t *service_frame = zmsg_pop (msg);
            worker->service = s_service_require (self, service_frame);
            zlist_append (self->waiting, worker);
            zlist_append (worker->service->waiting, worker);
            worker->service->workers++;
            worker->expiry = zclock_time () + HEARTBEAT_EXPIRY;
            s_service_dispatch (worker->service);
            zframe_destroy (&service_frame);
            zclock_log ("worker created");
        }
    }
    else
    if (zframe_streq (command, MDPW_REPORT)) {
        if (worker_ready) {
            //  Remove & save client return envelope and insert the
            //  protocol header and service name, then rewrap envelope.
            zframe_t *client = zmsg_unwrap (msg);
            zmsg_pushstr (msg, worker->service->name);
            zmsg_pushstr (msg, MDPC_REPORT);
            zmsg_pushstr (msg, MDPC_CLIENT);
            zmsg_wrap (msg, client);
            zmsg_send (&msg, self->socket);
        }
        else
            s_worker_delete (worker, 1);
    }
    else
    if (zframe_streq (command, MDPW_HEARTBEAT)) {
        if (worker_ready) {
            if (zlist_size (self->waiting) > 1) {
                // Move worker to the end of the waiting queue,
                // so s_broker_purge will only check old worker(s)
                zlist_remove (self->waiting, worker);
                zlist_append (self->waiting, worker);
            }
            worker->expiry = zclock_time () + HEARTBEAT_EXPIRY;
        }
        else
            s_worker_delete (worker, 1);
    }
    else
    if (zframe_streq (command, MDPW_DISCONNECT))
        s_worker_delete (worker, 0);
    else {
        zclock_log ("E: invalid input message");
        zmsg_dump (msg);
    }
    zframe_destroy (&command);
    zmsg_destroy (&msg);
}

//  Process a request coming from a client. We implement MMI requests
//  directly here (at present, we implement only the mmi.service request)

static void
s_broker_client_msg (broker_t *self, zframe_t *sender, zmsg_t *msg)
{
    assert (zmsg_size (msg) >= 2);     //  Service name + body

    zframe_t *service_frame = zmsg_pop (msg);
    service_t *service = s_service_require (self, service_frame);

    //  If we got a MMI service request, process that internally
    if (zframe_size (service_frame) >= 4
    &&  memcmp (zframe_data (service_frame), "mmi.", 4) == 0) {
        char *return_code;
        if (zframe_streq (service_frame, "mmi.service")) {
            char *name = zframe_strdup (zmsg_last (msg));
            service_t *service =
                (service_t *) zhash_lookup (self->services, name);
            return_code = service && service->workers? "200": "404";
            free (name);
        }
        else
        // The filter service that can be used to manipulate
        // the command filter table.
        if (zframe_streq (service_frame, "mmi.filter")
        && zmsg_size (msg) == 3) {
            zframe_t *operation = zmsg_pop (msg);
            zframe_t *service_frame = zmsg_pop (msg);
            zframe_t *command_frame = zmsg_pop (msg);
            char *command_str = zframe_strdup (command_frame);

            if (zframe_streq (operation, "enable")) {
                service_t *service = s_service_require (self, service_frame);
                s_service_enable_command (service, command_str);
                return_code = "200";
            }
            else
            if (zframe_streq (operation, "disable")) {
                service_t *service = s_service_require (self, service_frame);
                s_service_disable_command (service, command_str);
                return_code = "200";
            }
            else
                return_code = "400";

            zframe_destroy (&operation);
            zframe_destroy (&service_frame);
            zframe_destroy (&command_frame);
            free (command_str);
            //  Add an empty frame; it will be replaced by the return code.
            zmsg_pushstr (msg, "");
        }
        else
            return_code = "501";

        zframe_reset (zmsg_last (msg), return_code, strlen (return_code));

        //  Insert the protocol header and service name, then rewrap envelope.
        zmsg_push (msg, zframe_dup (service_frame));
        zmsg_pushstr (msg, MDPC_REPORT);
        zmsg_pushstr (msg, MDPC_CLIENT);
        zmsg_wrap (msg, zframe_dup (sender));
        zmsg_send (&msg, self->socket);
    }
    else {
        int enabled = 1;
        if (zmsg_size (msg) >= 1) {
            zframe_t *cmd_frame = zmsg_first (msg);
            char *cmd = zframe_strdup (cmd_frame);
            enabled = s_service_is_command_enabled (service, cmd);
            free (cmd);
        }

        //  Forward the message to the worker.
        if (enabled) {
            zmsg_wrap (msg, zframe_dup (sender));
            zlist_append (service->requests, msg);
            s_service_dispatch (service);
        }
        //  Send a NAK message back to the client.
        else {
            zmsg_push (msg, zframe_dup (service_frame));
            zmsg_pushstr (msg, MDPC_NAK);
            zmsg_pushstr (msg, MDPC_CLIENT);
            zmsg_wrap (msg, zframe_dup (sender));
            zmsg_send (&msg, self->socket);
        }
    }

    zframe_destroy (&service_frame);
}

//  The purge method deletes any idle workers that haven't pinged us in a
//  while. We hold workers from oldest to most recent, so we can stop
//  scanning whenever we find a live worker. This means we'll mainly stop
//  at the first worker, which is essential when we have large numbers of
//  workers (since we call this method in our critical path)

static void
s_broker_purge (broker_t *self)
{
    worker_t *worker = (worker_t *) zlist_first (self->waiting);
    while (worker) {
        if (zclock_time () < worker->expiry)
            break;                  //  Worker is alive, we're done here
        if (self->verbose)
            zclock_log ("I: deleting expired worker: %s",
                        worker->identity);

        s_worker_delete (worker, 0);
        worker = (worker_t *) zlist_first (self->waiting);
    }
}

//  Here is the implementation of the methods that work on a service.
//  Lazy constructor that locates a service by name, or creates a new
//  service if there is no service already with that name.

static service_t *
s_service_require (broker_t *self, zframe_t *service_frame)
{
    assert (service_frame);
    char *name = zframe_strdup (service_frame);

    service_t *service =
        (service_t *) zhash_lookup (self->services, name);
    if (service == NULL) {
        service = (service_t *) zmalloc (sizeof (service_t));
        service->broker = self;
        service->name = name;
        service->requests = zlist_new ();
        service->waiting = zlist_new ();
        service->blacklist = zlist_new ();
        zhash_insert (self->services, name, service);
        zhash_freefn (self->services, name, s_service_destroy);
        if (self->verbose)
            zclock_log ("I: added service: %s", name);
    }
    else
        free (name);

    return service;
}

//  Service destructor is called automatically whenever the service is
//  removed from broker->services.

static void
s_service_destroy (void *argument)
{
    service_t *service = (service_t *) argument;
    while (zlist_size (service->requests)) {
        zmsg_t *msg = (zmsg_t*)zlist_pop (service->requests);
        zmsg_destroy (&msg);
    }
    //  Free memory keeping  blacklisted commands.
    char *command = (char *) zlist_first (service->blacklist);
    while (command) {
        zlist_remove (service->blacklist, command);
        free (command);
    }
    zlist_destroy (&service->requests);
    zlist_destroy (&service->waiting);
    zlist_destroy (&service->blacklist);
    free (service->name);
    free (service);
}

//  The dispatch method sends request to the worker.
static void
s_service_dispatch (service_t *self)
{
    assert (self);

    s_broker_purge (self->broker);
    if (zlist_size (self->waiting) == 0)
        return;

    while (zlist_size (self->requests) > 0) {
        worker_t *worker = (worker_t*)zlist_pop (self->waiting);
        zlist_remove (self->waiting, worker);
        zmsg_t *msg = (zmsg_t*)zlist_pop (self->requests);
        s_worker_send (worker, MDPW_REQUEST, NULL, msg);
        //  Workers are scheduled in the round-robin fashion
        zlist_append (self->waiting, worker);
        zmsg_destroy (&msg);
    }
}

static void
s_service_enable_command (service_t *self, const char *command)
{
    char *item = (char *) zlist_first (self->blacklist);
    while (item && !streq (item, command))
        item = (char *) zlist_next (self->blacklist);
    if (item) {
        zlist_remove (self->blacklist, item);
        free (item);
    }
}

static void
s_service_disable_command (service_t *self, const char *command)
{
    char *item = (char *) zlist_first (self->blacklist);
    while (item && !streq (item, command))
        item = (char *) zlist_next (self->blacklist);
    if (!item)
        zlist_push (self->blacklist, strdup (command));
}

static int
s_service_is_command_enabled (service_t *self, const char *command)
{
    char *item = (char *) zlist_first (self->blacklist);
    while (item && !streq (item, command))
        item = (char *) zlist_next (self->blacklist);
    return item? 0: 1;
}

//  Here is the implementation of the methods that work on a worker.
//  Lazy constructor that locates a worker by identity, or creates a new
//  worker if there is no worker already with that identity.

static worker_t *
s_worker_require (broker_t *self, zframe_t *address)
{
    assert (address);

    //  self->workers is keyed off worker identity
    char *identity = zframe_strhex (address);
    worker_t *worker =
        (worker_t *) zhash_lookup (self->workers, identity);

    if (worker == NULL) {
        worker = (worker_t *) zmalloc (sizeof (worker_t));
        worker->broker = self;
        worker->identity = identity;
        worker->address = zframe_dup (address);
        zhash_insert (self->workers, identity, worker);
        zhash_freefn (self->workers, identity, s_worker_destroy);
        if (self->verbose)
            zclock_log ("I: registering new worker: %s", identity);
    }
    else
        free (identity);
    return worker;
}

//  The delete method deletes the current worker.

static void
s_worker_delete (worker_t *self, int disconnect)
{
    assert (self);
    if (disconnect)
        s_worker_send (self, MDPW_DISCONNECT, NULL, NULL);

    if (self->service) {
        zlist_remove (self->service->waiting, self);
        self->service->workers--;
    }
    zlist_remove (self->broker->waiting, self);
    //  This implicitly calls s_worker_destroy
    zhash_delete (self->broker->workers, self->identity);
}

//  Worker destructor is called automatically whenever the worker is
//  removed from broker->workers.

static void
s_worker_destroy (void *argument)
{
    worker_t *self = (worker_t *) argument;
    zframe_destroy (&self->address);
    free (self->identity);
    free (self);
}

//  The send method formats and sends a command to a worker. The caller may
//  also provide a command option, and a message payload.

static void
s_worker_send (worker_t *self, char *command, char *option, zmsg_t *msg)
{
    msg = msg? zmsg_dup (msg): zmsg_new ();

    //  Stack protocol envelope to start of message
    if (option)
        zmsg_pushstr (msg, option);
    zmsg_pushstr (msg, command);
    zmsg_pushstr (msg, MDPW_WORKER);

    //  Stack routing envelope to start of message
    zmsg_wrap (msg, zframe_dup (self->address));

    if (self->broker->verbose) {
        zclock_log ("I: sending %s to worker",
            mdpw_commands [(int) *command]);
        zmsg_dump (msg);
    }
    zmsg_send (&msg, self->broker->socket);
}


//  Finally here is the main task. We create a new broker instance and
//  then processes messages on the broker socket.

int main (int argc, char *argv [])
{
    int verbose = 0;
    int daemonize = 0;
    for (int i = 1; i < argc; i++)
    {
        if (streq(argv[i], "-v")) verbose = 1;
        else if (streq(argv[i], "-d")) daemonize = 1;
        else if (streq(argv[i], "-h"))
        {
            printf("%s [-h] | [-d] [-v] [broker url]\n\t-h This help message\n\t-d Daemon mode.\n\t-v Verbose output\n\tbroker url defaults to tcp://*:5555\n", argv[0]);
            return -1;
        }
    }

    if (daemonize != 0)
    {
	int rc = daemon(0, 0);
	assert (rc == 0);
    }

    broker_t *self = s_broker_new (verbose);
    /* did the user specify a bind address? */
    if (argc > 1)
    {
        s_broker_bind (self, argv[argc-1]);
        printf("Bound to %s\n", argv[argc-1]);
    }
    else
    {
        /* default */
        s_broker_bind (self, "tcp://*:5555");
        printf("Bound to tcp://*:5555\n");
    }

    //  Get and process messages forever or until interrupted
    while (true) {
        zmq_pollitem_t items [] = {
            { self->socket,  0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

        //  Process next input message, if any
        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (self->socket);
            if (!msg)
                break;          //  Interrupted
            if (self->verbose) {
                zclock_log ("I: received message:");
                zmsg_dump (msg);
            }
            zframe_t *sender = zmsg_pop (msg);
            zframe_t *empty  = zmsg_pop (msg);
            zframe_t *header = zmsg_pop (msg);

            if (zframe_streq (header, MDPC_CLIENT))
                s_broker_client_msg (self, sender, msg);
            else
            if (zframe_streq (header, MDPW_WORKER))
                s_broker_worker_msg (self, sender, msg);
            else {
                zclock_log ("E: invalid message:");
                zmsg_dump (msg);
                zmsg_destroy (&msg);
            }
            zframe_destroy (&sender);
            zframe_destroy (&empty);
            zframe_destroy (&header);
        }
        //  Disconnect and delete any expired workers
        //  Send heartbeats to idle workers if needed
        if (zclock_time () > self->heartbeat_at) {
            s_broker_purge (self);
            worker_t *worker = (worker_t *) zlist_first (self->waiting);
            while (worker) {
                s_worker_send (worker, MDPW_HEARTBEAT, NULL, NULL);
                worker = (worker_t *) zlist_next (self->waiting);
            }
            self->heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;
        }
    }
    if (zctx_interrupted)
        printf ("W: interrupt received, shutting down...\n");

    s_broker_destroy (&self);
    return 0;
}

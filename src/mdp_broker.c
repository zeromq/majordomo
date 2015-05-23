/*  =========================================================================
    mdp_broker - mdp_broker

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
#include "../include/mdp_msg.h"
#include "../include/mdp_broker.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration

    //  TODO: Add any properties you need here
    zhash_t *services;      // Hash of known services
    zhash_t *workers;       // Hash of known workers
    zlist_t *waiting;       // List of waiting workers
    zsock_t *router;        // The same socket as router in s_server_t
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    mdp_msg_t *message;         //  Message in and out

    //  TODO: Add specific properties for your application
    unsigned int timeouts;      // Number of timeouts
};

// The service class defines a single service instance.

typedef struct {
    server_t *broker;       // Broker instance
    char *name;             // Service name
    zlist_t *requests;      // List of client requests
    zlist_t *waiting;       // List of waiting workers
    size_t workers;         // How many workers we have
} service_t;

// The worker class defines a single worker, idle or active

typedef struct {
    server_t *broker;      // Broker instance
    char *identity;         // Identity or worker
    zframe_t *address;      // Address frame to route to
    service_t *service;     // Owning service, if known
    int64_t expiry;         // Expires at unless heartbeat
} worker_t; 

//  Include the generated server engine
#include "mdp_broker_engine.inc"

// Maximum number of timeouts; If this number is reached, we stop sending
// heartbeats and terminate connection.
#define MAX_TIMEOUTS 3

// Interval for sending heartbeat [ms]
#define HEARTBEAT_DELAY 1000

static void s_service_destroy(void *argument);
static void s_service_dispatch(service_t *self);

// Worker destructor is called automatically whenever the worker is
// removed from broker->workers.

static void s_worker_destroy(void *argument);
static void s_worker_delete(worker_t *self, int disconnect);

static worker_t *
s_worker_require(server_t *self, zframe_t *address)
{
    assert(address);
    
    // self->workers is keyed off worker identity.
    char *identity = zframe_strhex(address);
    worker_t *worker =
        (worker_t *) zhash_lookup(self->workers, identity);
    
    if (worker == NULL) {
        worker = (worker_t *) zmalloc(sizeof(worker_t));
        worker->broker = self;
        worker->identity = identity;
        worker->address = zframe_dup(address);

        zhash_insert(self->workers, identity, worker);
        zhash_freefn(self->workers, identity, s_worker_destroy);
    }
    else
        free(identity);
    return worker;
}

static void
s_worker_destroy(void *argument)
{
    worker_t *self = (worker_t *) argument;
    zframe_destroy(&self->address);
    free(self->identity);
    free(self);
}

static void
s_worker_delete(worker_t *self, int disconnect)
{
    assert(self);
    if (disconnect) {
        mdp_msg_t *msg = mdp_msg_new();
        assert(msg);
        mdp_msg_set_id(msg, MDP_MSG_DISCONNECT);
        mdp_msg_set_routing_id(msg, self->address);
        mdp_msg_send(msg, self->broker->router);
    }
    
    if (self->service) {
        zlist_remove(self->service->waiting, self);
        self->service->workers--;
    }
    zlist_remove(self->broker->waiting, self);
    // This implicitly calls s_worker_destroy.
    zhash_delete(self->broker->workers, self->identity);
}

static service_t *s_service_require(server_t *self, const char *service_name);

static service_t *
s_service_require(server_t *self, const char *service_name)
{
	char *name = strdup(service_name);
	service_t *service = (service_t *) zhash_lookup(self->services, name);
	if (service == NULL) {
		service = (service_t *) zmalloc(sizeof(service_t));
		service->broker = self;
		service->name = name;
		service->requests = zlist_new();
		service->waiting = zlist_new();
		zhash_insert(self->services, name, service);
		zhash_freefn(self->services, name, s_service_destroy);
	}
	return service;
}

static void
s_service_dispatch(service_t *self)
{
    printf("s_service_dispatch\n");
    while ((zlist_size(self->requests) > 0) &&
           (zlist_size(self->waiting) > 0)) {
        worker_t *worker = (worker_t *) zlist_pop(self->waiting);
        zlist_remove(self->broker->waiting, worker);
        mdp_msg_t *msg = (mdp_msg_t *) zlist_pop(self->requests);
        mdp_msg_t *worker_msg = mdp_msg_new();
        mdp_msg_set_id(worker_msg, MDP_MSG_WORKER_REQUEST);
        mdp_msg_set_routing_id(worker_msg, worker->address);
        zframe_t *address = zframe_dup(mdp_msg_routing_id(msg));
        mdp_msg_set_address(worker_msg, &address);
        zmsg_t *body = mdp_msg_get_body(msg);

        mdp_msg_set_body(worker_msg, &body);

        mdp_msg_send(worker_msg, self->broker->router);
        mdp_msg_destroy(&worker_msg);
        mdp_msg_destroy(&msg);
    }
}

// Service destructor is called automatically whenever the service is
// removed from broker->services.

static void
s_service_destroy(void *argument)
{
    service_t *service = (service_t *) argument;
    while (zlist_size(service->requests) > 0) {
        zmsg_t *msg = (zmsg_t *) zlist_pop(service->requests);
        zmsg_destroy(&msg);
    }
    zlist_destroy(&service->requests);
    zlist_destroy(&service->waiting);
    free(service->name);
    free(service);
}

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    //  Construct properties here
    self->services = zhash_new();
    self->workers = zhash_new();
    self->waiting = zlist_new();
    s_server_t *server = (s_server_t *) self;
    self->router = server->router;
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    //  Destroy properties here
    zlist_destroy(&self->waiting);
    zhash_destroy(&self->workers);
    zhash_destroy(&self->services);
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    return NULL;
}


//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
    //  Construct properties here
    self->timeouts = 0;
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}

//  ---------------------------------------------------------------------------
//  Selftest

void
mdp_broker_test (bool verbose)
{
    printf (" * mdp_broker: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    zactor_t *server = zactor_new (mdp_broker, "server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/mdp_broker", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, "ipc://@/mdp_broker");

    //  TODO: fill this out
    mdp_msg_t *request = mdp_msg_new ();
    mdp_msg_destroy (&request);
    
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  handle_request
//

static void
handle_request (client_t *self)
{
    // mdp_msg_t *msg = self->message;
    // Create a fresh instance of mdp_msg_t to append to the list of
    // requests.
    mdp_msg_t *msg = mdp_msg_new();
    // routing id, messageid, service, body
    mdp_msg_set_routing_id(msg, mdp_msg_routing_id(self->message));
    mdp_msg_set_id(msg, mdp_msg_id(self->message));
    mdp_msg_set_service(msg, mdp_msg_service(self->message));
    zmsg_t *body = mdp_msg_get_body(self->message);
    mdp_msg_set_body(msg, &body);
    const char *service_name = mdp_msg_service(msg);
	service_t *service = s_service_require(self->server, service_name);
	zlist_append(service->requests, msg);
	s_service_dispatch(service);
}


//  ---------------------------------------------------------------------------
//  handle_worker_partial
//

static void
handle_worker_partial (client_t *self)
{
    mdp_msg_t *msg = self->message;
    mdp_msg_t *client_msg = mdp_msg_new();
    // Set routing id, messageid, service, body
    zframe_t *address = mdp_msg_address(msg);

    mdp_msg_set_routing_id(client_msg, address);
    mdp_msg_set_id(client_msg, MDP_MSG_CLIENT_PARTIAL);
    mdp_msg_set_service(client_msg, mdp_msg_service(msg));
    zmsg_t *body = mdp_msg_get_body(msg);
    mdp_msg_set_body(client_msg, &body);
    mdp_msg_send(client_msg, self->server->router);
    mdp_msg_destroy(&client_msg);
}


//  ---------------------------------------------------------------------------
//  handle_worker_final
//

static void
handle_worker_final (client_t *self)
{
    mdp_msg_t *msg = self->message;
    mdp_msg_t *client_msg = mdp_msg_new();
    // Set routing id, messageid, service, body
    zframe_t *address = mdp_msg_address(msg);

    mdp_msg_set_routing_id(client_msg, address);
    mdp_msg_set_id(client_msg, MDP_MSG_CLIENT_FINAL);
    const char *service_name = mdp_msg_service(msg);
    mdp_msg_set_service(client_msg, service_name);
    zmsg_t *body = mdp_msg_get_body(msg);
    mdp_msg_set_body(client_msg, &body);
    mdp_msg_send(client_msg, self->server->router);

    // Add the worker back to the list of waiting workers.
    char *identity = zframe_strhex(mdp_msg_routing_id(msg));

    worker_t *worker =
        (worker_t *) zhash_lookup(self->server->workers, identity);
    assert(worker);
    zlist_append(self->server->waiting, worker);
    service_t *service = (service_t *) zhash_lookup(self->server->services,
        service_name);
    assert(service);
    zlist_append(service->waiting, worker);

    mdp_msg_destroy(&client_msg);
}


//  ---------------------------------------------------------------------------
//  destroy_broker
//

static void
destroy_broker (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  handle_ready
//

static void
handle_ready (client_t *self)
{
    mdp_msg_t *msg = self->message;
    const char *service_name = mdp_msg_service(msg);
    zsys_debug("handle_ready: service=%s\n", service_name);
    zframe_t *routing_id = mdp_msg_routing_id(msg);
    assert(routing_id);
    char *identity = zframe_strhex(routing_id);
    int worker_ready = (zhash_lookup(self->server->workers, identity) != NULL);
    free(identity);

    worker_t *worker = s_worker_require(self->server, routing_id);
    
    if (worker_ready)   // Not first command in session.
        s_worker_delete(worker, 1);
    else {
        service_t *service = s_service_require(self->server, service_name);
        worker->service = service;
        zlist_append(service->broker->waiting, worker);
        zlist_append(service->waiting, worker);
        worker->service->workers++;
        s_service_dispatch(service);
    }
}


//  ---------------------------------------------------------------------------
//  reset_timeouts
//

static void
reset_timeouts (client_t *self)
{
    self->timeouts = 0;
}


//  ---------------------------------------------------------------------------
//  handle_set_wakeup
//

static void
handle_set_wakeup (client_t *self)
{
    engine_set_wakeup_event(self, HEARTBEAT_DELAY, send_heartbeat_event);
}


//  ---------------------------------------------------------------------------
//  delete_worker
//

static void
delete_worker (client_t *self)
{
    mdp_msg_t *msg = self->message;
    zframe_t *routing_id = mdp_msg_routing_id(msg);
    assert(routing_id);
    char *identity = zframe_strhex(routing_id);
    worker_t *worker = (worker_t *) zhash_lookup(self->server->workers, identity);
    free(identity);
    if (worker != NULL)
        s_worker_delete(worker, 0);
}


//  ---------------------------------------------------------------------------
//  check_timeouts
//

static void
check_timeouts (client_t *self)
{
    self->timeouts++;
    if (self->timeouts == MAX_TIMEOUTS) {
        engine_set_exception(self, terminate_event);
    }
}

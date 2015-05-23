/*  =========================================================================
    mdp_client_engine - Majordomo Client engine

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


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connected_state = 2,
    wait_for_response_state = 3,
    defaults_state = 4
} state_t;

typedef enum {
    NULL_event = 0,
    constructor_event = 1,
    connect_error_event = 2,
    request_event = 3,
    destructor_event = 4,
    client_partial_event = 5,
    client_final_event = 6,
    set_verbose_event = 7
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connected",
    "wait for response",
    "defaults"
};

static char *
s_event_name [] = {
    "(NONE)",
    "constructor",
    "connect_error",
    "request",
    "destructor",
    "CLIENT_PARTIAL",
    "CLIENT_FINAL",
    "set_verbose"
};
 

//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    char *service;
    zmsg_t *body;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    mdp_client_msg_t *message;  //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool terminated;            //  True if client is shutdown
    bool fsm_stopped;           //  "terminate" action called
    size_t timeout;             //  inactivity timeout, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for timeouts
    int wakeup_timer;           //  zloop timer for alarms
    event_t wakeup_event;       //  Wake up with this event
} s_client_t;

static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_client_destroy (s_client_t **self_p);
static void
    s_client_execute (s_client_t *self, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    connect_to_server (client_t *self);
static void
    signal_connection_success (client_t *self);
static void
    signal_connect_error (client_t *self);
static void
    send_request_to_broker (client_t *self);
static void
    disconnect_from_broker (client_t *self);
static void
    send_partial_response (client_t *self);
static void
    send_final_response (client_t *self);
static void
    handle_set_verbose (client_t *self);
static void
    log_protocol_error (client_t *self);

//  Global tracing/animation indicator; we can't use a client method as
//  that only works after construction (which we often want to trace).
volatile int mdp_client_verbose = false;
    
//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *cmdpipe, zsock_t *msgpipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->cmdpipe = cmdpipe;
        self->msgpipe = msgpipe;
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = mdp_client_msg_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
            self->state = start_state;
            self->event = NULL_event;
            self->client.cmdpipe = self->cmdpipe;
            self->client.msgpipe = self->msgpipe;
            self->client.dealer = self->dealer;
            self->client.message = self->message;
            self->client.args = &self->args;
            if (client_initialize (&self->client))
                s_client_destroy (&self);
        }
        else
            s_client_destroy (&self);
    }
    s_satisfy_pedantic_compilers ();
    return self;
}

//  Destroy the client connection

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.service);
        zmsg_destroy (&self->args.body);
        client_terminate (&self->client);
        mdp_client_msg_destroy (&self->message);
        zsock_destroy (&self->msgpipe);
        zsock_destroy (&self->dealer);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  dealer socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should handle the
//  wakeup event. The alarm is cancelled on any other event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Set timeout for next protocol read. By default, will wait forever
//  or until the process is interrupted. The timeout is in milliseconds.
//  The state machine must handle the "expired" event.

static void
engine_set_timeout (client_t *client, size_t timeout)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->timeout = timeout;
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives client as arg.

static void
engine_handle_socket (client_t *client, zsock_t *sock, zloop_reader_fn handler)
{
    if (client && sock) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, sock);
        }
        else
            zloop_reader_end (self->loop, sock);
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_timeout (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (s_client_t *self, mdp_client_msg_t *message)
{
    assert (message);
    switch (mdp_client_msg_id (message)) {
        case MDP_CLIENT_MSG_CLIENT_PARTIAL:
            return client_partial_event;
            break;
        case MDP_CLIENT_MSG_CLIENT_FINAL:
            return client_final_event;
            break;
        default:
            zsys_error ("mdp_client: unknown command %s, halting", mdp_client_msg_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is stopped, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated                    //  Actor is dying
        && !self->fsm_stopped                   //  FSM has finished
        && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (mdp_client_verbose) {
            zsys_debug ("mdp_client: %s:", s_state_name [self->state]);
            zsys_debug ("mdp_client:        %s", s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == constructor_event) {
                    if (!self->exception) {
                        //  connect to server
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ connect to server");
                        connect_to_server (&self->client);
                    }
                    if (!self->exception) {
                        //  signal connection success
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ signal connection success");
                        signal_connection_success (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == connect_error_event) {
                    if (!self->exception) {
                        //  signal connect error
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ signal connect error");
                        signal_connect_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ terminate");
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == set_verbose_event) {
                    if (!self->exception) {
                        //  handle set verbose
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ handle set verbose");
                        handle_set_verbose (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  log protocol error
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ log protocol error");
                        log_protocol_error (&self->client);
                    }
                }
                break;

            case connected_state:
                if (self->event == request_event) {
                    if (!self->exception) {
                        //  send request to broker
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ send request to broker");
                        send_request_to_broker (&self->client);
                    }
                    if (!self->exception)
                        self->state = wait_for_response_state;
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  disconnect from broker
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ disconnect from broker");
                        disconnect_from_broker (&self->client);
                    }
                }
                else
                if (self->event == set_verbose_event) {
                    if (!self->exception) {
                        //  handle set verbose
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ handle set verbose");
                        handle_set_verbose (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  log protocol error
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ log protocol error");
                        log_protocol_error (&self->client);
                    }
                }
                break;

            case wait_for_response_state:
                if (self->event == client_partial_event) {
                    if (!self->exception) {
                        //  send partial response
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ send partial response");
                        send_partial_response (&self->client);
                    }
                }
                else
                if (self->event == client_final_event) {
                    if (!self->exception) {
                        //  send final response
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ send final response");
                        send_final_response (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == set_verbose_event) {
                    if (!self->exception) {
                        //  handle set verbose
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ handle set verbose");
                        handle_set_verbose (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  log protocol error
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ log protocol error");
                        log_protocol_error (&self->client);
                    }
                }
                break;

            case defaults_state:
                if (self->event == set_verbose_event) {
                    if (!self->exception) {
                        //  handle set verbose
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ handle set verbose");
                        handle_set_verbose (&self->client);
                    }
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  log protocol error
                        if (mdp_client_verbose)
                            zsys_debug ("mdp_client:            $ log protocol error");
                        log_protocol_error (&self->client);
                    }
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (mdp_client_verbose)
                zsys_debug ("mdp_client:            ! %s", s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (mdp_client_verbose)
            zsys_debug ("mdp_client:            > %s", s_state_name [self->state]);
    }
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Handle command pipe to/from calling API

static int
s_client_handle_cmdpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    char *method = zstr_recv (self->cmdpipe);
    if (!method)
        return -1;                  //  Interrupted; exit zloop
    if (mdp_client_verbose)
        zsys_debug ("mdp_client:        API command=%s", method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "CONSTRUCTOR")) {
        zstr_free (&self->args.endpoint);
        zsock_recv (self->cmdpipe, "s", &self->args.endpoint);
        s_client_execute (self, constructor_event);
    }
    else
    if (streq (method, "DESTRUCTOR")) {
        s_client_execute (self, destructor_event);
    }
    else
    if (streq (method, "REQUEST")) {
        zstr_free (&self->args.service);
        zmsg_destroy (&self->args.body);
        zsock_recv (self->cmdpipe, "sp", &self->args.service, &self->args.body);
        s_client_execute (self, request_event);
    }
    else
    if (streq (method, "SET VERBOSE")) {
        s_client_execute (self, set_verbose_event);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->cmdpipe)) {
        zsys_error ("mdp_client: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->cmdpipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  Handle message pipe to/from calling API

static int
s_client_handle_msgpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->msgpipe) & ZMQ_POLLIN) {
        char *method = zstr_recv (self->msgpipe);
        if (!method)
            return -1;              //  Interrupted; exit zloop
        if (mdp_client_verbose)
            zsys_debug ("mdp_client:        API message=%s", method);

        //  Front-end shuts down msgpipe before cmdpipe, this little
        //  handshake just ensures all traffic on the msgpipe has been
        //  flushed before the calling thread continues with destroying
        //  the actor.
        if (streq (method, "$FLUSH"))
            zsock_signal (self->cmdpipe, 0);
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->msgpipe)) {
            zsys_error ("mdp_client: trailing API message frames (%s)", method);
            zmsg_t *more = zmsg_recv (self->msgpipe);
            zmsg_print (more);
            zmsg_destroy (&more);
        }
        zstr_free (&method);
    }
    return 0;
}


//  Handle a message (a protocol reply) from the server

static int
s_client_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->dealer) & ZMQ_POLLIN) {
        if (mdp_client_msg_recv (self->message, self->dealer))
            return -1;              //  Interrupted; exit zloop

        s_client_execute (self, s_protocol_event (self, self->message));
        if (self->terminated)
            return -1;
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
mdp_client (zsock_t *cmdpipe, void *msgpipe)
{
    //  Initialize
    s_client_t *self = s_client_new (cmdpipe, (zsock_t *) msgpipe);
    if (self) {
        zsock_signal (cmdpipe, 0);
        
        //  Set up handler for the sockets the client uses
        engine_handle_socket ((client_t *) self, self->cmdpipe, s_client_handle_cmdpipe);
        engine_handle_socket ((client_t *) self, self->msgpipe, s_client_handle_msgpipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
    else
        zsock_signal (cmdpipe, -1);
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _mdp_client_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    uint8_t status;             //  Returned by actor reply
    char *reason;               //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new mdp_client
//  Connect to server endpoint. Succeed if connection is successful.                

static uint8_t 
mdp_client_constructor (mdp_client_t *self, const char *endpoint);

mdp_client_t *
mdp_client_new (const char *endpoint)
{
    mdp_client_t *self = (mdp_client_t *) zmalloc (sizeof (mdp_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        self->actor = zactor_new (mdp_client, backend);
        if (self->actor)
            self->status = mdp_client_constructor (self, endpoint);
        if (self->status == -1)
            zactor_destroy (&self->actor);
        if (!self->actor)
            mdp_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the mdp_client
//  Disconnect from server.                                                         

static uint8_t 
mdp_client_destructor (mdp_client_t *self);

void
mdp_client_destroy (mdp_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mdp_client_t *self = *self_p;
        if (self->actor && !zsys_interrupted) {
            //  Before destroying the actor we have to flush any pending
            //  traffic on the msgpipe, otherwise it gets lost in a fire and
            //  forget scenario. We do this by sending $FLUSH to the msgpipe
            //  and waiting for a signal back on the cmdpipe.
            if (zstr_send (self->msgpipe, "$FLUSH") == 0)
                zsock_wait (self->actor);
            mdp_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->reason);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.

zactor_t *
mdp_client_actor (mdp_client_t *self)
{
    assert (self);
    return self->actor;
}


//  ---------------------------------------------------------------------------
//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.

zsock_t *
mdp_client_msgpipe (mdp_client_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted or timed-out.

static int
s_accept_reply (mdp_client_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted or timed-out
        
        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "SUCCESS")) {
                    zsock_recv (self->actor, "1", &self->status);
                }
                else
                if (streq (reply, "FAILURE")) {
                    zstr_free (&self->reason);
                    zsock_recv (self->actor, "1s", &self->status, &self->reason);
                }
                break;
            }
            filter = va_arg (args, char *);
        }
        va_end (args);
        //  If anything was remaining on pipe, flush it
        zsock_flush (self->actor);
        if (filter) {
            zstr_free (&reply);
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted or timed-out
}


//  ---------------------------------------------------------------------------
//  Connect to server endpoint. Succeed if connection is successful.                
//  Returns >= 0 if successful, -1 if interrupted.

static uint8_t 
mdp_client_constructor (mdp_client_t *self, const char *endpoint)
{
    assert (self);
    zsock_send (self->actor, "ss", "CONSTRUCTOR", endpoint);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Disconnect from server.                                                         

uint8_t 
mdp_client_destructor (mdp_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "DESTRUCTOR");
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send request to broker.                                                         

int 
mdp_client_request (mdp_client_t *self, const char *service, zmsg_t **body_p)
{
    assert (self);
    zsock_send (self->actor, "ssp", "REQUEST", service, *body_p);
    *body_p = NULL;             //  Take ownership of body
    return 0;
}


//  ---------------------------------------------------------------------------
//  Set mdp_client_verbose.                                                         

int 
mdp_client_set_verbose (mdp_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "SET VERBOSE");
    return 0;
}


//  ---------------------------------------------------------------------------
//  Return last received status

uint8_t 
mdp_client_status (mdp_client_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received reason

const char *
mdp_client_reason (mdp_client_t *self)
{
    assert (self);
    return self->reason;
}

#include "majordomo_ext.h"

VALUE rb_cMajordomoClient;

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_mark_majordomo_client(void *ptr)
{
    rb_majordomo_client_t *client = (rb_majordomo_client_t *)ptr;
    if (client) {
        rb_gc_mark(client->broker);
        rb_gc_mark(client->timeout);
    }
}

/*
 * :nodoc:
 *  Release the GIL when closing a Majordomo client
 *
*/
static VALUE rb_nogvl_mdp_client_close(void *ptr)
{
    mdp_client_t *client = ptr;
    mdp_client_destroy(&client);
    return Qnil;
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_free_majordomo_client(void *ptr)
{
    rb_majordomo_client_t *client = (rb_majordomo_client_t *)ptr;
    if (client) {
        if (client->client) rb_thread_blocking_region(rb_nogvl_mdp_client_close, (void *)client->client, RUBY_UBF_IO, 0);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
        zlist_destroy(&(client->recv_buffer));
#endif
        xfree(client);
        client = NULL;
    }
}

/*
 * :nodoc:
 *  Release the GIL when creating a new Majordomo client
 *
*/
static VALUE rb_nogvl_mdp_client_new(void *ptr)
{
    struct nogvl_md_client_new_args *args = ptr;
    return (VALUE)mdp_client_new(args->broker, args->verbose);
}

/*
 *  call-seq:
 *     Majordomo::Client.new("tcp://0.0.0.0:5555")       =>  Majordomo::Client
 *     Majordomo::Client.new("tcp://0.0.0.0:5555", true) =>  Majordomo::Client
 *
 *  Creates a new Majordomo::Client instance. A broker URI is required and an optional verbose flag
 *  can be passed to the initializer.
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.broker                                         =>  "tcp://0.0.0.0:5555"
 *     cl.timeout                                        =>  2500
 *     cl.send("test", "request")                        =>  "reply"
 *
*/
static VALUE rb_majordomo_client_s_new(int argc, VALUE *argv, VALUE klass)
{
    rb_majordomo_client_t *client = NULL;
    struct nogvl_md_client_new_args args;
    VALUE obj, broker, verbose;
    rb_scan_args(argc, argv, "11", &broker, &verbose);
    if (verbose == Qnil)
        verbose = Qfalse;
    Check_Type(broker, T_STRING);
    obj = Data_Make_Struct(klass, rb_majordomo_client_t, rb_mark_majordomo_client, rb_free_majordomo_client, client);
    args.broker = RSTRING_PTR(broker);
    args.verbose = (verbose == Qtrue ? 1 : 0);
    client->client = (mdp_client_t *)rb_thread_blocking_region(rb_nogvl_mdp_client_new, (void *)&args, RUBY_UBF_IO, 0);
    client->broker = rb_str_new4(broker);
    client->timeout = INT2NUM(MAJORDOMO_CLIENT_TIMEOUT);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    client->recv_buffer = zlist_new();
#endif
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

/*
 *  call-seq:
 *     cl.broker                                         =>  String
 *
 *  Returns the URI of the broker this client is connected to.
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.broker                                         =>  "tcp://0.0.0.0:5555"
 *
*/
static VALUE rb_majordomo_client_broker(VALUE obj){
    GetMajordomoClient(obj);
    return client->broker;
}

/*
 *  call-seq:
 *     cl.timeout                       =>  Fixnum
 *
 *  Returns the request timeout for this client (in msecs).
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.timeout                                        =>  2500
 *
*/
static VALUE rb_majordomo_client_timeout(VALUE obj){
    GetMajordomoClient(obj);
    return client->timeout;
}

/*
 *  call-seq:
 *     cl.timeout = val                                  =>  nil
 *
 *  Sets the request timeout for this client (in msecs).
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.timeout = 100                                  =>  nil
 *     cl.timeout                                        =>  100
 *
*/
static VALUE rb_majordomo_client_timeout_equals(VALUE obj, VALUE timeout){
    GetMajordomoClient(obj);
    Check_Type(timeout, T_FIXNUM);
    mdp_client_set_timeout(client->client, FIX2INT(timeout));
    client->timeout = timeout;
    return Qnil;
}

/*
 * :nodoc:
 *  Release the GIL when sending a client message
 *
*/
static VALUE rb_nogvl_mdp_client_send(void *ptr)
{
    struct nogvl_md_client_send_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    mdp_client_send(args->client, args->service, &args->request);
#else
    uint32_t events;
    size_t evopt_len = sizeof (uint32_t);
    int fd;
    size_t fdopt_len = sizeof (int);
    if (rb_thread_alone()) {
        mdp_client_send(args->client, args->service, &args->request);
        return Qnil;
    }
try_writable:
    mdp_client_getsockopt (args->client, ZMQ_EVENTS, &events, &evopt_len);
    if ((events & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        mdp_client_send(args->client, args->service, &args->request);
    } else {
        mdp_client_getsockopt (args->client, ZMQ_FD, &fd, &fdopt_len);
        rb_thread_wait_fd(fd);
        goto try_writable;
    }
#endif
    return Qnil;
}

/*
 *  call-seq:
 *     cl.send("service", "message")                     =>  boolean
 *
 *  Send a request to the broker. Returns true if the send was successful.
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.send("service", "message")                     =>  true
 *
*/
static VALUE rb_majordomo_client_send(VALUE obj, VALUE service, VALUE message){
    struct nogvl_md_client_send_args args;
    GetMajordomoClient(obj);
    Check_Type(service, T_STRING);
    Check_Type(message, T_STRING);
    args.client = client->client;
    args.service = RSTRING_PTR(service);
    args.request = zmsg_new();
    if (!args.request)
        return Qfalse;
    if (zmsg_pushstr(args.request, RSTRING_PTR(message)) != 0){
        zmsg_destroy(&args.request);
        return Qfalse;
    }
    rb_thread_blocking_region(rb_nogvl_mdp_client_send, (void *)&args, RUBY_UBF_IO, 0);
    return Qtrue;
}

/*
 * :nodoc:
 *  Release the GIL when receiving a client message
 *
*/
static VALUE rb_nogvl_mdp_client_recv(void *ptr)
{
    struct nogvl_md_client_recv_args *args = ptr;
    rb_majordomo_client_t *client = args->client;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)mdp_client_recv(client->client, args->service);
#else
    uint32_t events;
    size_t evopt_len = sizeof (uint32_t);
    int fd;
    size_t fdopt_len = sizeof (int);
    if (zlist_size(client->recv_buffer) != 0)
       return (VALUE)zlist_pop(client->recv_buffer);
try_readable:
    mdp_client_getsockopt (client->client, ZMQ_EVENTS, &events, &evopt_len);
    if ((events & ZMQ_POLLIN) == ZMQ_POLLIN) {
        do {
            zlist_append(client->recv_buffer, mdp_client_recv(client->client, args->service));
        } while (zmq_errno() != EAGAIN && zmq_errno() != EINTR);
        return (VALUE)zlist_pop(client->recv_buffer);
     } else {
        mdp_client_getsockopt (client->client, ZMQ_FD, &fd, &fdopt_len);
        rb_thread_wait_fd(fd);
        goto try_readable;
     }
#endif
}

/*
 *  call-seq:
 *     cl.recv("service")                     =>  String or nil
 *
 *  Send a request to the broker and get a reply even if it has to retry several times. Valid replies are of type
 *  String and NilClass.
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.send("service", "message")                     =>  nil
 *     cl.recv("service")                                =>  "reply"
 *
*/
static VALUE rb_majordomo_client_recv(VALUE obj, VALUE service){
    VALUE rep;
    zmsg_t *reply = NULL;
    struct nogvl_md_client_recv_args args;
    GetMajordomoClient(obj);
    Check_Type(service, T_STRING);
    args.client = client;
    args.service = RSTRING_PTR(service);
    reply = (zmsg_t *)rb_thread_blocking_region(rb_nogvl_mdp_client_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (!reply)
        return Qnil;
    rep = MajordomoEncode(rb_str_new2(zmsg_popstr(reply)));
    zmsg_destroy(&reply);
    return rep;
}

/*
 *  call-seq:
 *     cl.close                                          =>  nil
 *
 *  Close the client connection to the broker.
 *
 * === Examples
 *     cl = Majordomo::Client.new("tcp://0.0.0.0:5555")  =>  Majordomo::Client
 *     cl.close                                          =>  nil
 *
*/
static VALUE rb_majordomo_client_close(VALUE obj){
    VALUE ret;
    GetMajordomoClient(obj);
    ret = rb_thread_blocking_region(rb_nogvl_mdp_client_close, (void *)client->client, RUBY_UBF_IO, 0);
    client->client = NULL;
    return ret;
}

void _init_majordomo_client()
{
    rb_cMajordomoClient = rb_define_class_under(rb_mMajordomo, "Client", rb_cObject);

    rb_define_singleton_method(rb_cMajordomoClient, "new", rb_majordomo_client_s_new, -1);
    rb_define_method(rb_cMajordomoClient, "broker", rb_majordomo_client_broker, 0);
    rb_define_method(rb_cMajordomoClient, "timeout", rb_majordomo_client_timeout, 0);
    rb_define_method(rb_cMajordomoClient, "timeout=", rb_majordomo_client_timeout_equals, 1);
    rb_define_method(rb_cMajordomoClient, "send", rb_majordomo_client_send, 2);
    rb_define_method(rb_cMajordomoClient, "recv", rb_majordomo_client_recv, 1);
    rb_define_method(rb_cMajordomoClient, "close", rb_majordomo_client_close, 0);
}
#include "majordomo_ext.h"

VALUE rb_cMajordomoWorker;

/*
 * :nodoc:
 *  GC mark callback
 *
*/
static void rb_mark_majordomo_worker(void *ptr)
{
    rb_majordomo_worker_t *worker = (rb_majordomo_worker_t *)ptr;
    if (worker) {
        rb_gc_mark(worker->broker);
        rb_gc_mark(worker->service);
        rb_gc_mark(worker->heartbeat);
        rb_gc_mark(worker->reconnect);
    }
}

/*
 * :nodoc:
 *  Release the GIL when closing a Majordomo worker
 *
*/
static VALUE rb_nogvl_mdp_worker_close(void *ptr)
{
    mdp_worker_t *worker = ptr;
    mdp_worker_destroy(&worker);
    return Qnil;
}

/*
 * :nodoc:
 *  GC free callback
 *
*/
static void rb_free_majordomo_worker(void *ptr)
{
    rb_majordomo_worker_t *worker = (rb_majordomo_worker_t *)ptr;
    if (worker) {
        if (worker->worker) rb_thread_blocking_region(rb_nogvl_mdp_worker_close, (void *)worker->worker, RUBY_UBF_IO, 0);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
        zlist_destroy(&(worker->recv_buffer));
#endif
        xfree(worker);
        worker = NULL;
    }
}

/*
 * :nodoc:
 *  Release the GIL when creating a new Majordomo worker
 *
*/
static VALUE rb_nogvl_mdp_worker_new(void *ptr)
{
    struct nogvl_md_worker_new_args *args = ptr;
    return (VALUE)mdp_worker_new(args->broker, args->service, args->verbose);
}

/*
 *  call-seq:
 *     Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")       =>  Majordomo::Worker
 *     Majordomo::Worker.new("tcp://0.0.0.0:5555", "service", true) =>  Majordomo::Worker
 *
 *  Creates a new Majordomo::Worker instance. A broker URI and service identifier is required and an
 *  optional verbose flag can be passed to the initializer.
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.broker                                                    =>  "tcp://0.0.0.0:5555"
 *     wk.heartbeat                                                 =>  2500
 *     wk.recv                                                      =>  "request"
 *
*/
static VALUE rb_majordomo_worker_s_new(int argc, VALUE *argv, VALUE klass)
{
    rb_majordomo_worker_t *worker = NULL;
    struct nogvl_md_worker_new_args args;
    VALUE obj, broker, service, verbose;
    rb_scan_args(argc, argv, "21", &broker, &service, &verbose);
    if (verbose == Qnil)
        verbose = Qfalse;
    Check_Type(broker, T_STRING);
    Check_Type(service, T_STRING);
    obj = Data_Make_Struct(klass, rb_majordomo_worker_t, rb_mark_majordomo_worker, rb_free_majordomo_worker, worker);

    args.broker = RSTRING_PTR(broker);
    args.service = RSTRING_PTR(service);
    args.verbose = (verbose == Qtrue ? 1 : 0);
    worker->worker = (mdp_worker_t *)rb_thread_blocking_region(rb_nogvl_mdp_worker_new, (void *)&args, RUBY_UBF_IO, 0);
    worker->broker = rb_str_new4(broker);
    worker->service = rb_str_new4(service);
    worker->heartbeat = INT2NUM(MAJORDOMO_WORKER_HEARTBEAT);
    worker->reconnect = INT2NUM(MAJORDOMO_WORKER_RECONNECT);
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    worker->recv_buffer = zlist_new();
#endif
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

/*
 *  call-seq:
 *     wk.broker                        =>  String
 *
 *  Returns the URI of the broker this worker is connected to.
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.broker                                                    =>  "tcp://0.0.0.0:5555"
 *
*/
static VALUE rb_majordomo_worker_broker(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->broker;
}

/*
 *  call-seq:
 *     wk.service                       =>  String
 *
 *  Returns the service identifier this worker implements.
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.service                                                   =>  "service"
 *
*/
static VALUE rb_majordomo_worker_service(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->service;
}

/*
 *  call-seq:
 *     wk.heartbeat                                                 =>  Fixnum
 *
 *  Returns the worker heartbeat delay (in msecs).
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.heartbeat                                                 =>  2500
 *
*/
static VALUE rb_majordomo_worker_heartbeat(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->heartbeat;
}

/*
 *  call-seq:
 *     wk.reconnect                                                 =>  Fixnum
 *
 *  Returns the worker reconnect delay (in msecs).
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.reconnect                                                 =>  2500
 *
*/
static VALUE rb_majordomo_worker_reconnect(VALUE obj){
    GetMajordomoWorker(obj);
    return worker->reconnect;
}

/*
 *  call-seq:
 *     wk.heartbeat = val                                           =>  nil
 *
 *  Sets the worker heartbeat delay (in msecs).
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.heartbeat = 100                                           =>  nil
 *     wk.heartbeat                                                 =>  100
 *
*/
static VALUE rb_majordomo_worker_heartbeat_equals(VALUE obj, VALUE heartbeat){
    GetMajordomoWorker(obj);
    Check_Type(heartbeat, T_FIXNUM);
    mdp_worker_set_heartbeat(worker->worker, FIX2INT(heartbeat));
    worker->heartbeat = heartbeat;
    return Qnil;
}

/*
 *  call-seq:
 *     wk.reconnect = 100                                           =>  nil
 *
 *  Sets the worker reconnect delay (in msecs).
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.reconnect = 100                                           =>  nil
 *     wk.reconnect                                                 =>  100
 *
*/
static VALUE rb_majordomo_worker_reconnect_equals(VALUE obj, VALUE reconnect){
    GetMajordomoWorker(obj);
    Check_Type(reconnect, T_FIXNUM);
    mdp_worker_set_reconnect(worker->worker, FIX2INT(reconnect));
    worker->reconnect = reconnect;
    return Qnil;
}

/*
 * :nodoc:
 *  Release the GIL when receiving a worker message
 *
*/
static VALUE rb_nogvl_mdp_worker_recv(void *ptr)
{
    struct nogvl_md_worker_recv_args *args = ptr;
    rb_majordomo_worker_t *worker = args->worker;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    return (VALUE)mdp_worker_recv(worker->worker, &args->reply);
#else
    uint32_t events;
    size_t evopt_len = sizeof (uint32_t);
    int fd;
    size_t fdopt_len = sizeof (int);
    if (zlist_size(worker->recv_buffer) != 0)
       return (VALUE)zlist_pop(worker->recv_buffer);
try_readable:
    mdp_worker_getsockopt (worker->worker, ZMQ_EVENTS, &events, &evopt_len);
    if ((events & ZMQ_POLLIN) == ZMQ_POLLIN) {
        do {
            zlist_append(worker->recv_buffer, mdp_worker_recv(worker->worker, &args->reply));
        } while (zmq_errno() != EAGAIN && zmq_errno() != EINTR);
        return (VALUE)zlist_pop(worker->recv_buffer);
     } else {
        mdp_worker_getsockopt (worker->worker, ZMQ_FD, &fd, &fdopt_len);
        rb_thread_wait_fd(fd);
        goto try_readable;
     }
#endif
}

/*
 *  call-seq:
 *     wk.recv                                                      =>  String or nil
 *
 *  Receives a client request form the broker. Valid requests are of type String and NilClass
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.recv                                                      =>  ["request", "reply"]
 *
*/
static VALUE rb_majordomo_worker_recv(VALUE obj){
    VALUE req, reply;
    struct nogvl_md_worker_recv_args args;
    GetMajordomoWorker(obj);
    args.worker = worker;
    args.reply = NULL;
    zmsg_t *request = (zmsg_t *)rb_thread_blocking_region(rb_nogvl_mdp_worker_recv, (void *)&args, RUBY_UBF_IO, 0);
    if (!request)
        return Qnil;
    req = MajordomoEncode(rb_str_new2(zmsg_popstr(request)));
    zmsg_destroy(&request);
    reply = rb_str_new(zframe_data(args.reply), zframe_size(args.reply));
    zframe_destroy(&args.reply);
    return rb_ary_new3(2, req, reply);
}

/*
 * :nodoc:
 *  Release the GIL when sending a worker message
 *
*/
static VALUE rb_nogvl_mdp_worker_send(void *ptr)
{
    struct nogvl_md_worker_send_args *args = ptr;
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
    mdp_worker_send(args->worker, &args->progress, args->reply_to);
#else
    uint32_t events;
    size_t evopt_len = sizeof (uint32_t);
    int fd;
    size_t fdopt_len = sizeof (int);
    if (rb_thread_alone()) {
        mdp_worker_send(args->worker, &args->progress, args->reply_to);
        return Qnil;
    }
try_writable:
    mdp_worker_getsockopt (args->worker, ZMQ_EVENTS, &events, &evopt_len);
    if ((events & ZMQ_POLLOUT) == ZMQ_POLLOUT) {
        mdp_worker_send(args->worker, &args->progress, args->reply_to);
    } else {
        mdp_worker_getsockopt (args->worker, ZMQ_FD, &fd, &fdopt_len);
        rb_thread_wait_fd(fd);
        goto try_writable;
    }
#endif
    return Qnil;
}

/*
 *  call-seq:
 *     wk.send(message, reply_to)                                   =>  boolean
 *
 *  Send a reply to a client request. Returns true if the send was succfessful.
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     req, reply_to = wk.recv                                      =>  ["request", "reply"]
 *     wk.send("reply", reply_to)                                   =>  true
 *
*/
static VALUE rb_majordomo_worker_send(VALUE obj, VALUE message, VALUE reply_to){
    struct nogvl_md_worker_send_args args;
    GetMajordomoWorker(obj);
    args.worker = worker->worker;
    args.progress = zmsg_new();
    if (!args.progress)
        return Qfalse;
    if (zmsg_pushmem(args.progress, RSTRING_PTR(message), RSTRING_LEN(message)) == -1) {
        zmsg_destroy(&args.progress);
        return Qfalse;
    }
    args.reply_to = zframe_new(RSTRING_PTR(reply_to), RSTRING_LEN(reply_to));
    if (!args.reply_to) {
        zmsg_destroy(&args.progress);
        return Qfalse;
    }
    rb_thread_blocking_region(rb_nogvl_mdp_worker_send, (void *)&args, RUBY_UBF_IO, 0);
    zframe_destroy(&args.reply_to);
    return Qtrue;
}

/*
 *  call-seq:
 *     wk.close                                                     =>  nil
 *
 *  Close the worker connection to the broker.
 *
 * === Examples
 *     wk = Majordomo::Worker.new("tcp://0.0.0.0:5555", "service")  =>  Majordomo::Worker
 *     wk.close                                                     =>  nil
 *
*/
static VALUE rb_majordomo_worker_close(VALUE obj){
    VALUE ret;
    GetMajordomoWorker(obj);
    ret = rb_thread_blocking_region(rb_nogvl_mdp_worker_close, (void *)worker->worker, RUBY_UBF_IO, 0);
    worker->worker = NULL;
    return ret;
}

void _init_majordomo_worker()
{
    rb_cMajordomoWorker = rb_define_class_under(rb_mMajordomo, "Worker", rb_cObject);

    rb_define_singleton_method(rb_cMajordomoWorker, "new", rb_majordomo_worker_s_new, -1);
    rb_define_method(rb_cMajordomoWorker, "broker", rb_majordomo_worker_broker, 0);
    rb_define_method(rb_cMajordomoWorker, "service", rb_majordomo_worker_service, 0);
    rb_define_method(rb_cMajordomoWorker, "heartbeat", rb_majordomo_worker_heartbeat, 0);
    rb_define_method(rb_cMajordomoWorker, "reconnect", rb_majordomo_worker_reconnect, 0);
    rb_define_method(rb_cMajordomoWorker, "heartbeat=", rb_majordomo_worker_heartbeat_equals, 1);
    rb_define_method(rb_cMajordomoWorker, "reconnect=", rb_majordomo_worker_reconnect_equals, 1);
    rb_define_method(rb_cMajordomoWorker, "recv", rb_majordomo_worker_recv, 0);
    rb_define_method(rb_cMajordomoWorker, "send", rb_majordomo_worker_send, 2);
    rb_define_method(rb_cMajordomoWorker, "close", rb_majordomo_worker_close, 0);
}
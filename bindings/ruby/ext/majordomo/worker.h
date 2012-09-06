#ifndef MAJORDOMO_WORKER_H
#define MAJORDOMO_WORKER_H

typedef struct {
    mdp_worker_t *worker;
    VALUE broker;
    VALUE service;
    VALUE heartbeat;
    VALUE reconnect;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    zlist_t *recv_buffer;
#endif
} rb_majordomo_worker_t;

#define MAJORDOMO_WORKER_HEARTBEAT 2500
#define MAJORDOMO_WORKER_RECONNECT 2500

#define GetMajordomoWorker(obj) \
    rb_majordomo_worker_t *worker = NULL; \
    Data_Get_Struct(obj, rb_majordomo_worker_t, worker); \
    if (!worker) rb_raise(rb_eTypeError, "uninitialized Majordomo worker!"); \
    if (!worker->worker) rb_raise(rb_eRuntimeError, "Majordomo worker has already been closed!");

struct nogvl_md_worker_new_args {
    char *broker;
    char *service;
    int verbose;
};

struct nogvl_md_worker_recv_args {
    rb_majordomo_worker_t *worker;
    zframe_t *reply;
};

struct nogvl_md_worker_send_args {
    mdp_worker_t *worker;
    zmsg_t *progress;
    zframe_t *reply_to;
};

void _init_majordomo_worker();

#endif
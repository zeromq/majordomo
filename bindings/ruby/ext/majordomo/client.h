#ifndef MAJORDOMO_CLIENT_H
#define MAJORDOMO_CLIENT_H

typedef struct {
    mdp_client_t *client;
    VALUE broker;
    VALUE timeout;
#ifndef HAVE_RB_THREAD_BLOCKING_REGION
    zlist_t *recv_buffer;
#endif
} rb_majordomo_client_t;

#define MAJORDOMO_CLIENT_TIMEOUT 2500

#define GetMajordomoClient(obj) \
    rb_majordomo_client_t *client = NULL; \
    Data_Get_Struct(obj, rb_majordomo_client_t, client); \
    if (!client) rb_raise(rb_eTypeError, "uninitialized Majordomo client!"); \
    if (!client->client) rb_raise(rb_eRuntimeError, "Majordomo client has already been closed!");

struct nogvl_md_client_new_args {
    char *broker;
    int verbose;
};

struct nogvl_md_client_send_args {
    mdp_client_t *client;
    char *service;
    zmsg_t *request;
};

struct nogvl_md_client_recv_args {
    rb_majordomo_client_t *client;
    char *service;
};

void _init_majordomo_client();

#endif
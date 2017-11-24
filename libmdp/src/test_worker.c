//
//  Majordomo Protocol worker example

#include "../include/mdp.h"

int main (int argc, char *argv [])
{
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    mdp_worker_t *session = mdp_worker_new (
        "tcp://localhost:5555", "echo", verbose);

    while (1) {
        zframe_t *reply_to;
        zmsg_t *request = mdp_worker_recv (session, &reply_to);
        if (request == NULL)
            break;              //  Worker was interrupted
        //  Echo message
        mdp_worker_send (session, &request, reply_to);
        zframe_destroy (&reply_to);
    }
    mdp_worker_destroy (&session);
    return 0;
}

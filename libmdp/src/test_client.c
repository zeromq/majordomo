//
//  Majordomo Protocol client example
//

#include "../include/mdp.h"

int main (int argc, char *argv [])
{
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    mdp_client_t *session = mdp_client_new ("tcp://localhost:5555", verbose);

    int count;
    for (count = 0; count < 100000; count++) {
        zmsg_t *request = zmsg_new ();
        zmsg_pushstr (request, "Hello world");
        mdp_client_send (session, "echo", &request);
        zmsg_t *reply = mdp_client_recv (session, NULL, NULL);
        if (reply)
            zmsg_destroy (&reply);
        else
            break;              //  Interrupt or failure
    }
    printf ("%d requests/replies processed\n", count);
    mdp_client_destroy (&session);
    return 0;
}

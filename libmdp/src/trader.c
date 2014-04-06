//
//  Simple trading example.
//  It is purpose is to demonstate how to use mdp_client interface.

#include "mdp_common.h"
#include "mdp_client.h"

int main (int argc, char *argv [])
{
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    mdp_client_t *client = mdp_client_new ("tcp://localhost:5555", verbose);

    //  Send 100 sell orders
    int count;
    for (count = 0; count < 5; count++) {
        zmsg_t *request = zmsg_new ();
        zmsg_pushstr (request, "8");                // volume
        zmsg_pushstrf (request, "%d", count + 1000); // price
        zmsg_pushstr (request, "SELL");
        mdp_client_send (client, "NYSE", &request);
    }

    //  Send 1 buy order.
    //  This order will match all sell orders.
    zmsg_t *request = zmsg_new ();
    zmsg_pushstr (request, "800");      // volume
    zmsg_pushstr (request, "2000");     // price
    zmsg_pushstr (request, "BUY");
    mdp_client_send (client, "NYSE", &request);

    //  Wait for all trading reports
    while (1) {
        char *command = NULL;
        char *service = NULL;
        zmsg_t *report = mdp_client_recv (client, &command, &service);
        if (report == NULL)
            break;
        assert (zmsg_size (report) >= 2);
        zframe_t *report_type = zmsg_pop (report);
        char *report_type_str = zframe_strdup (report_type);
        zframe_destroy (&report_type);
        zframe_t *volume = zmsg_pop (report);
        char *volume_str = zframe_strdup (volume);
        zframe_destroy (&volume);

        printf ("%s: %s %s shares\n", service, report_type_str, volume_str);

        free (command);
        free (service);
        free (report_type_str);
        free (volume_str);
        zmsg_destroy (&report);
    }

    mdp_client_destroy (&client);
    return 0;
}

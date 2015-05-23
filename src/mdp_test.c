#include <stdio.h>
#include <czmq.h>
#include "../include/mdp_client.h"
#include "../include/mdp_worker.h"
#include "../include/mdp_broker.h"
#include "../include/mdp_worker_msg.h"

int
main()
{
    int res;

    printf("hello mdp_test\n");
    int verbose = 1;
    char *endpoint = "tcp://localhost:6002";
    char *endpoint_bind = "tcp://*:6002";
    mdp_client_t *client = mdp_client_new(endpoint);
    assert(client);
    char *service = "MAKE COFFEE";
    mdp_worker_t *worker = mdp_worker_new(endpoint, service);
    assert(worker);
    zactor_t *broker = zactor_new(mdp_broker, "server");
    if (verbose)
    {
        zstr_send(broker, "VERBOSE");
        // zstr_send(client, "VERBOSE");
        // zstr_send(worker, "VERBOSE");
        mdp_worker_set_verbose(worker);
        mdp_client_set_verbose(client);
    }

    zstr_sendx(broker, "BIND", endpoint_bind, NULL);

    zmsg_t *msg = zmsg_new();
    assert(msg);
    res = zmsg_addstr(msg, "Message");
    assert(res == 0);

    res = mdp_client_request(client, service, &msg);
//    msg = zmsg_recv(worker);
    zsock_t *worker_sock = mdp_worker_msgpipe(worker);
    char *cmd = zstr_recv(worker_sock);
    printf("Got command: %s\n", cmd);
    
    zframe_t *address;
    zmsg_t *message;
    res = zsock_recv(worker_sock, "fm",
        &address, &message);
    
    // Process the message.
    zframe_t *first = zmsg_first(message);
    char *first_str = zframe_strdup(first);
    printf("Got message: %s\n", first_str);
    char response[64];
    sprintf(response, "Partial response to %s", first_str);
    zmsg_t *msg_response = zmsg_new();
    zmsg_addstr(msg_response, response);

    // Make a copy of address, because mdp_worker_send_partial will destroy it.
    zframe_t *address2 = zframe_dup(address);
    mdp_worker_send_partial(worker, &address2, &msg_response);

    // Wait for partial reponse.
    zsock_t *client_sock = mdp_client_msgpipe(client);
    res = zsock_recv(client_sock, "sm", &cmd, &message);
    printf("Client (2): got command %s\n", cmd);
    printf(" Response body:\n");
    zmsg_print(message);
    zmsg_destroy(&message);

    sprintf(response, "Final response to %s", first_str);
    msg_response = zmsg_new();
    zmsg_addstr(msg_response, response);

    mdp_worker_send_final(worker, &address, &msg_response);

    // Wait for final response.
    res = zsock_recv(client_sock, "sm", &cmd, &message);
    printf("Client (2): got command %s\n", cmd);
    printf(" Response body:\n");
    zmsg_print(message);

    printf("Press Enter to stop");
    getchar();

    zactor_destroy(&broker);
    mdp_worker_destroy(&worker);
    mdp_client_destroy(&client);
    return 0;
}

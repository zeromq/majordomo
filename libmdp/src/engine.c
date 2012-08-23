//
//  Simple matching engine

#include "mdp_common.h"
#include "mdp_worker.h"

struct _order_t {
    mdp_worker_t *worker;
    int price;
    int volume;
    zframe_t *reply_to;
};

typedef struct _order_t order_t;

static order_t *
s_order_new (mdp_worker_t *worker, int price, int volume, zframe_t *reply_to)
{
    assert (worker);
    assert (price >= 0);
    assert (volume >= 0);

    order_t *self = (order_t *) zmalloc (sizeof *self);

    //  Initialize sell_order
    self->worker = worker;
    self->price = price;
    self->volume = volume;
    self->reply_to = reply_to;
    return self;
}

static void
s_order_destroy (order_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        order_t *self = *self_p;
        zframe_destroy (&self->reply_to);
        free (self);
        *self_p = NULL;
    }
}

static void
s_order_update (order_t *self, int volume)
{
    assert (self);
    assert (volume <= self->volume);
    self->volume -= volume;

    //  Prepare and send report to the client
    zmsg_t *report = zmsg_new ();
    zmsg_pushstr (report, "%d", volume);

    if (self->volume == 0)
        zmsg_pushstr (report, "FILL");
    else
        zmsg_pushstr (report, "PARTIAL_FILL");

    mdp_worker_send (self->worker, &report, self->reply_to);
}

struct _engine_t {
    mdp_worker_t *worker;
    zlist_t *sell_orders;
    zlist_t *buy_orders;
};

typedef struct _engine_t engine_t;

static engine_t *
s_engine_new (char *broker, char *service, int verbose)
{
    engine_t *self = (engine_t *) zmalloc (sizeof *self);

    //  Initialize engine state
    self->worker = mdp_worker_new (broker, service, verbose);
    self->sell_orders = zlist_new ();
    self->buy_orders = zlist_new ();
    return self;
}

static void
s_engine_destroy (engine_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        engine_t *self = *self_p;
        mdp_worker_destroy (&self->worker);

        //  Destroy remaining sell orders
        while (zlist_size (self->sell_orders) > 0) {
            order_t *order = (order_t *) zlist_pop (self->sell_orders);
            s_order_destroy(&order);
        }
        zlist_destroy (&self->sell_orders);

        //  Destroy remaining buy orders
        while (zlist_size (self->buy_orders) > 0) {
            order_t *order = (order_t *) zlist_pop (self->buy_orders);
            s_order_destroy(&order);
        }
        zlist_destroy (&self->buy_orders);

        free (self);
        *self_p = NULL;
    }
}

static order_t *
s_engine_match_buy_order (engine_t *self)
{
    assert (self);

    order_t *order = (order_t *) zlist_first (self->buy_orders);
    order_t *best_order = order;

    while (order) {
        if (order->price > best_order->price)
            best_order = order;
        order = (order_t *) zlist_next (self->buy_orders);
    }

    return best_order;
}

static order_t *
s_engine_match_sell_order (engine_t *self)
{
    assert (self);

    order_t *order = (order_t *) zlist_first (self->sell_orders);
    order_t *best_order = order;

    while (order) {
        if (order->price < best_order->price)
            best_order = order;
        order = (order_t *) zlist_next (self->sell_orders);
    }

    return best_order;
}

static void
s_engine_handle_buy_request (engine_t *self, zframe_t *price,
        zframe_t *volume, zframe_t *reply_to)
{
    char *price_str = zframe_strdup (price);
    char *volume_str = zframe_strdup (volume);

    order_t *buy_order = s_order_new (self->worker,
            atoi (price_str), atoi (volume_str), reply_to);

    free (price_str);
    free (volume_str);

    while (buy_order->volume) {
      order_t *sell_order = s_engine_match_sell_order (self);
      if (sell_order == NULL || sell_order->price > buy_order->price)
          break;
      int volume = MIN (buy_order->volume, sell_order->volume);
      s_order_update (buy_order, volume);
      s_order_update (sell_order, volume);

      if (sell_order->volume == 0) {
          zlist_remove (self->sell_orders, sell_order);
          s_order_destroy (&sell_order);
      }
    }

    if (buy_order->volume == 0)
        s_order_destroy (&buy_order);
    else
        zlist_append (self->buy_orders, buy_order);
}

static void
s_engine_handle_sell_request (engine_t *self, zframe_t *price,
        zframe_t *volume, zframe_t *reply_to)
{
    char *price_str = zframe_strdup (price);
    char *volume_str = zframe_strdup (volume);

    order_t *sell_order = s_order_new (self->worker,
            atoi (price_str), atoi (volume_str), reply_to);

    free (price_str);
    free (volume_str);

    while (sell_order->volume) {
        order_t *buy_order = s_engine_match_buy_order (self);
        if (buy_order == NULL || buy_order->price < sell_order->price)
            break;
        int volume = MIN (sell_order->volume, buy_order->volume);

        //  Perform trade
        s_order_update (sell_order, volume);
        s_order_update (buy_order, volume);

        if (buy_order->volume == 0) {
            zlist_remove (self->buy_orders, buy_order);
            s_order_destroy (&buy_order);
        }
    }

    if (sell_order->volume == 0)
        s_order_destroy (&sell_order);
    else
        zlist_append (self->sell_orders, sell_order);
}

static void
s_engine_handle_request (engine_t *self, zmsg_t *request, zframe_t *reply_to)
{
    assert (zmsg_size (request) >= 3);

    zframe_t *operation = zmsg_pop (request);
    zframe_t *price     = zmsg_pop (request);
    zframe_t *volume    = zmsg_pop (request);

    if (zframe_streq (operation, "SELL"))
        s_engine_handle_sell_request (self, price, volume, reply_to);
    else
    if (zframe_streq (operation, "BUY"))
        s_engine_handle_buy_request (self, price, volume, reply_to);
    else {
        zclock_log ("E: invalid message: ");
        zmsg_dump (request);
    }

    zframe_destroy (&operation);
    zframe_destroy (&price);
    zframe_destroy (&volume);

    zmsg_destroy (&request);
}

int main(int argc, char *argv [])
{
    int verbose = (argc > 1 && streq (argv [1], "-v"));
    engine_t *engine = s_engine_new (
        "tcp://localhost:5555", "NYSE", verbose);

    while (1) {
        zframe_t *reply_to = NULL;
        zmsg_t *request = mdp_worker_recv (engine->worker, &reply_to);
        if (request == NULL)
            break;          // Worker has been interrupted
        s_engine_handle_request (engine, request, reply_to);
    }
    s_engine_destroy (&engine);
    return 0;
}

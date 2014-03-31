/*  =========================================================================
    mdp_worker.h - worker API

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the Majordomo Project: http://majordomo.zeromq.org,
    an implementation of rfc.zeromq.org/spec:18/MDP (MDP/0.2) in C.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#ifndef __MDWRKAPI_H_INCLUDED__
#define __MDWRKAPI_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _mdp_worker_t mdp_worker_t;

//  @interface
CZMQ_EXPORT mdp_worker_t *
    mdp_worker_new (zctx_t *ctx, char *broker,char *service, int verbose);
CZMQ_EXPORT void
    mdp_worker_destroy (mdp_worker_t **self_p);
CZMQ_EXPORT void
    mdp_worker_set_heartbeat (mdp_worker_t *self, int heartbeat);
CZMQ_EXPORT void
    mdp_worker_set_reconnect (mdp_worker_t *self, int reconnect);
CZMQ_EXPORT int
    mdp_worker_setsockopt (mdp_worker_t *self, int option, const void *optval,
    size_t optvallen);
CZMQ_EXPORT int
    mdp_worker_getsockopt (mdp_worker_t *self, int option, void *optval,
    size_t *optvallen);
CZMQ_EXPORT zmsg_t *
    mdp_worker_recv (mdp_worker_t *self, zframe_t **reply_p);
CZMQ_EXPORT void
    mdp_worker_send (mdp_worker_t *self, zmsg_t **progress_p,
                     zframe_t *reply_to);
//  @end

#ifdef __cplusplus
}
#endif

#endif

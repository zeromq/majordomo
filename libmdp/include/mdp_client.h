/*  =========================================================================
    mdp_client.h - client API

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

#ifndef __MDCLIAPI_H_INCLUDED__
#define __MDCLIAPI_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _mdp_client_t mdp_client_t;

//  @interface
CZMQ_EXPORT mdp_client_t *
    mdp_client_new (char *broker, int verbose);
CZMQ_EXPORT void
    mdp_client_destroy (mdp_client_t **self_p);
CZMQ_EXPORT void
    mdp_client_set_timeout (mdp_client_t *self, int timeout);
CZMQ_EXPORT int
    mdp_client_setsockopt (mdp_client_t *self, int option, const void *optval,
    size_t optvallen);
CZMQ_EXPORT int
    mdp_client_getsockopt (mdp_client_t *self, int option, void *optval,
    size_t *optvallen);
CZMQ_EXPORT void
    mdp_client_send (mdp_client_t *self, char *service, zmsg_t **request_p);
CZMQ_EXPORT zmsg_t *
    mdp_client_recv (mdp_client_t *self, char **command_p, char **service_p);
//  @end

#ifdef __cplusplus
}
#endif

#endif

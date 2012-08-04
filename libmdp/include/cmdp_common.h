/*  =========================================================================
    cmdp.h - CMDP wrapper

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of the Majordomo Project: http://majordomo.zeromq.org.

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

#ifndef __CMDP_H_INCLUDED__
#define __CMDP_H_INCLUDED__

//  Set up environment for the application
//
#include <czmq.h>

//  Classes listed in alphabetical order

#include <cmdp_client.h>
#include <cmdp_worker.h>

#endif


//
//  mdp.h
//  Majordomo Protocol definitions
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__

//  This is the version of MDP/Client we implement
#define MDPC_CLIENT         "MDPC01"

//  This is the version of MDP/Worker we implement
#define MDPW_WORKER         "MDPW01"

//  MDP/Server commands, as strings
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPLY          "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

static char *mdps_commands [] = {
    NULL, "READY", "REQUEST", "REPLY", "HEARTBEAT", "DISCONNECT"
};

#endif


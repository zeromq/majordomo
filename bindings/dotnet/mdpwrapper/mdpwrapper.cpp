// mdpwrapper.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "mdpwrapper.h"




mdp_client_t * client_new (char *broker, int verbose)
{
	return mdp_client_new( broker, verbose );
}

void client_destroy (mdp_client_t **self_p)
{
	mdp_client_destroy( self_p );
}

void client_set_timeout (mdp_client_t *self, int timeout)
{
	mdp_client_set_timeout( self, timeout );
}


int client_setsockopt (mdp_client_t *self, int option, const void *optval,    size_t optvallen)
{
	return mdp_client_setsockopt( self, option, optval, optvallen );
}

int client_getsockopt (mdp_client_t *self, int option, void *optval,    size_t *optvallen)
{
	return mdp_client_getsockopt( self, option, optval, optvallen );
}

void client_send_data (mdp_client_t *self, char *service, char *data, int size )
{
	zmsg_t* zmsg = zmsg_new();
	zmsg_addmem( zmsg, data, size );
	mdp_client_send( self, service, &zmsg );
}


void client_send_string( mdp_client_t *self, char *service, char *msg )
{
	zmsg_t* zmsg = zmsg_new();
	zmsg_addstr( zmsg, msg );
	mdp_client_send( self, service, &zmsg );
}


zmsg_t* client_recv (mdp_client_t *self, char **service_p  )
{
	return mdp_client_recv( self, service_p );
}

void client_send( mdp_client_t *self, char *service, zmsg_t **msg_p )
{
	mdp_client_send( self, service, msg_p );
}

zmsg_t* msg_new()
{
	return zmsg_new();
}

int push_str( zmsg_t* msg, char* str )
{
	return zmsg_pushstr( msg, str );
}

int push_mem( zmsg_t* msg, const void* buffer, int length )
{
	return zmsg_pushmem( msg, buffer, length );
}

void msg_destroy( zmsg_t **msg_p )
{
	zmsg_destroy( msg_p );
}
void frame_destroy( zframe_t **frame_p )
{
	zframe_destroy( frame_p );
}

int pop_mem( zmsg_t* msg, void** buffer ) 
{
	int buffer_size = 0;
	*buffer = NULL;
	zframe_t* frame = zmsg_pop( msg );
	buffer_size = zframe_size( frame );
	*buffer = malloc( buffer_size );
	memcpy( *buffer, zframe_data( frame ), buffer_size );
	zframe_destroy( &frame );
	return buffer_size;
}
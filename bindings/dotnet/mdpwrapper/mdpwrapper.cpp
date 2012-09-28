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


void client_recv (mdp_client_t *self, char **service_p, char **response_p )
{
	*response_p = NULL;
	zmsg_t* reply = mdp_client_recv( self, service_p );
	if( reply ) {
		size_t buffer_size = zmsg_content_size( reply ) + 1;
		*response_p = (char*)malloc( buffer_size );
		memset( *response_p, 0, buffer_size );

		char* p = *response_p;
		
		while(true) {
			zframe_t* frame = zmsg_pop( reply );
			if( frame ) {
				size_t frame_size = zframe_size(frame);
				memcpy( p, zframe_data(frame), frame_size );
				p += frame_size;
				zframe_destroy(&frame);
			} else {
				break;
			}
		}
	}
}

// This is the main DLL file.

#include "stdafx.h"
//#include "mdp_wrapper.h"
#include <mdp.h>

#include "zeromq.majordomo.h"

using namespace zeromq::majordomo;

#pragma unmanaged
void* client_new( char* broker, int verbose ) {
	mdp_client_t* handle = mdp_client_new( broker, verbose );
	return handle;
}

void client_destroy( void* handle_ptr ) {
	if( handle_ptr ) {
		mdp_client_t* handle = static_cast<mdp_client_t*>(handle_ptr);
		mdp_client_destroy( &handle );
	}
}

void client_set_timeout( void* handle_ptr, int timeout ) {
	assert( handle_ptr != NULL );
	assert( timeout >= 0 );
	mdp_client_t* handle = static_cast<mdp_client_t*>(handle_ptr);
	mdp_client_set_timeout( handle, timeout );

}

void client_send( void* handle_ptr, char* service, char* message ) {
	assert( handle_ptr != NULL );
	mdp_client_t* handle = static_cast<mdp_client_t*>(handle_ptr);
	zmsg_t *request = zmsg_new();
	zmsg_pushstr( request, message );
	mdp_client_send( handle, service, &request );
}

void extract_string_from_zmsg( zmsg_t* msg, char** contents ) {
	size_t message_size = zmsg_content_size( msg );
	*contents = static_cast<char*>(malloc(message_size + 1)); // extra byte for null
	char* p = *contents;
	while(true) {
		char* msg_part = zmsg_popstr( msg ) ;
		if( msg_part ) {
			strcpy( p, msg_part );
			p += strlen( msg_part ); // set pointer to empty part of the buffer
			free( msg_part );
		} else {
			break;
		}
	}
}

void client_recv( void* handle_ptr, char** message, char** service ) {
	assert( handle_ptr != NULL );
	mdp_client_t* handle = static_cast<mdp_client_t*>(handle_ptr);
	zmsg_t* response = mdp_client_recv( handle, service );
	if( response ) { 
		extract_string_from_zmsg( response, message );
		zmsg_destroy( &response );
	}

}

///////////////////////////
// Worker API
//////////////////////////

void* worker_new( char* broker, char* service, bool verbose ) {
	mdp_worker_t* handle = mdp_worker_new( broker, service, verbose );
	return handle;
}

void worker_destroy( void* handle_ptr ) {
	if( handle_ptr ) {
		mdp_worker_t* handle = static_cast<mdp_worker_t*>(handle_ptr);
		mdp_worker_destroy( &handle );
	}
}

void worker_set_heartbeat( void* handle_ptr, int heartbeat ) {
	assert( handle_ptr != NULL );
	mdp_worker_set_heartbeat( static_cast<mdp_worker_t*>(handle_ptr), heartbeat );
}

void worker_set_reconnect( void* handle_ptr, int reconnect ) {
	assert( handle_ptr != NULL );
	mdp_worker_set_reconnect( static_cast<mdp_worker_t*>(handle_ptr), reconnect );
}

void worker_recv( void* handle_ptr, char** message, char** reply_to ) {
	assert( handle_ptr != NULL );
	zframe_t* reply_p = NULL;
	zmsg_t* response = mdp_worker_recv( static_cast<mdp_worker_t*>(handle_ptr), &reply_p ); 
	if( reply_p ) {
		*reply_to = zframe_strdup( reply_p );
		zframe_destroy( &reply_p );
	}

	if( response ) {
		extract_string_from_zmsg( response, message );
		zmsg_destroy( &response );
	}

}

void worker_send( void* handle_ptr, char* message, char* reply_to ) {
	assert( handle_ptr != NULL );
	mdp_worker_t* handle = static_cast<mdp_worker_t*>(handle_ptr);
	zmsg_t* msg_p = zmsg_new();
	zmsg_pushstr( msg_p, message );
	zframe_t* reply_p = zframe_new( reply_to, strlen( reply_to ) + 1  );
	mdp_worker_send( handle, &msg_p, reply_p );
	zframe_destroy( &reply_p );

}



#pragma managed

Client::Client( System::String^ BrokerBinding ) {
	_brokerBinding = BrokerBinding;
	IntPtr ip = Marshal::StringToHGlobalAnsi( BrokerBinding );
	char* str = static_cast<char*>(ip.ToPointer() );

	_mdpClientHandle = gcnew IntPtr( client_new( str, 0 ) );

	Marshal::FreeHGlobal( ip );
}

Client::Client( System::String^ BrokerBinding, bool verbose ) {
	_brokerBinding = BrokerBinding;
	IntPtr ip = Marshal::StringToHGlobalAnsi( BrokerBinding );
	char* str = static_cast<char*>(ip.ToPointer() );

	_mdpClientHandle = gcnew IntPtr( client_new( str, verbose ? 1 : 0 ) );

	Marshal::FreeHGlobal( ip );
}

Client::~Client() {
	client_destroy(_mdpClientHandle->ToPointer());
}

void Client::Send( System::String^ Service, System::String^ Message ) {
	 IntPtr servicePtr = Marshal::StringToHGlobalAnsi( Service );
	 IntPtr messagePtr = Marshal::StringToHGlobalAnsi( Message );
	 char* service = static_cast<char*>(servicePtr.ToPointer() );
	 char* message = static_cast<char*>(messagePtr.ToPointer());
	 client_send( _mdpClientHandle->ToPointer(), service, message );
	 Marshal::FreeHGlobal( servicePtr );
	 Marshal::FreeHGlobal( messagePtr );	
}

System::String^ Client::Recv( System::String^% Service ) {

	char* service = 0;
	char* message = 0;
	client_recv( _mdpClientHandle->ToPointer(), &service, &message );
	if( service ) {
		Service = Marshal::PtrToStringAnsi( static_cast<IntPtr>(service) );
		free(service);
	}

	System::String^ Result = Marshal::PtrToStringAnsi( static_cast<IntPtr>(message) );
	free( message );

	return Result;


}


System::String^ Client::BrokerBinding::get() {
	return _brokerBinding;
}

void Client::Timeout::set( int timeout ) {
	client_set_timeout( _mdpClientHandle->ToPointer(), timeout );
}



Worker::Worker( System::String^ BrokerBinding, System::String^ Service ) {
	_brokerBinding = BrokerBinding ;
	IntPtr brokerPtr = Marshal::StringToHGlobalAnsi( BrokerBinding );
	IntPtr servicePtr = Marshal::StringToHGlobalAnsi( Service );
	_mdpWorkerHandle = gcnew IntPtr( worker_new( static_cast<char*>(brokerPtr.ToPointer()), static_cast<char*>(servicePtr.ToPointer()), 0 ) );
	Marshal::FreeHGlobal( brokerPtr );
	Marshal::FreeHGlobal( servicePtr );

}

Worker::Worker( System::String^ BrokerBinding, System::String^ Service, bool verbose ) {
	_brokerBinding = BrokerBinding ;
	_service = Service;
	IntPtr brokerPtr = Marshal::StringToHGlobalAnsi( BrokerBinding );
	IntPtr servicePtr = Marshal::StringToHGlobalAnsi( Service );
	_mdpWorkerHandle = gcnew IntPtr( worker_new( static_cast<char*>(brokerPtr.ToPointer()), static_cast<char*>(servicePtr.ToPointer()), verbose ? 1 : 0 ) );
	Marshal::FreeHGlobal( brokerPtr );
	Marshal::FreeHGlobal( servicePtr );
}

Worker::~Worker() {
	worker_destroy( _mdpWorkerHandle->ToPointer() );
}


System::String^ Worker::Recv( System::String^% ReplyTo ) {
	char* message = 0;
	char* reply_to = 0;
	worker_recv( _mdpWorkerHandle->ToPointer(), &message, &reply_to );
	if( reply_to ) {
		ReplyTo = Marshal::PtrToStringAnsi( static_cast<IntPtr>(reply_to) );
		free(reply_to);
	}
	System::String^ Result = Marshal::PtrToStringAnsi( static_cast<IntPtr>(message) );
	free(message);
	return Result;
}

void Worker::Send( System::String^ Report, System::String^ ReplyTo ) {
	IntPtr reportPtr = Marshal::StringToHGlobalAnsi( Report );
	IntPtr replyToPtr = Marshal::StringToHGlobalAnsi( ReplyTo );
	char* report = static_cast<char*>(reportPtr.ToPointer() );
	char* reply_to = static_cast<char*>(replyToPtr.ToPointer());
	worker_send( _mdpWorkerHandle->ToPointer(), report, reply_to );
	Marshal::FreeHGlobal( reportPtr );
	Marshal::FreeHGlobal( replyToPtr );	
}

System::String^ Worker::BrokerBinding::get() {
	return _brokerBinding;
}

System::String^ Worker::Service::get() {
	return _service;
}

void Worker::Heartbeat::set( int heartbeat ) {
	worker_set_heartbeat( _mdpWorkerHandle->ToPointer(), heartbeat );
}

void Worker::Reconnect::set( int reconnect ) {
	worker_set_reconnect( _mdpWorkerHandle->ToPointer(), reconnect );

}



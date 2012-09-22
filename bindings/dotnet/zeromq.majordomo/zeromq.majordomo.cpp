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

void client_destroy( void* ptr ) {
	if( ptr ) {
		mdp_client_t* handle = static_cast<mdp_client_t*>(ptr);
		mdp_client_destroy( &handle );
	}
}

void client_set_timeout( void* ptr, int timeout ) {
	assert( ptr != NULL );
	assert( timeout >= 0 );
	mdp_client_t* handle = static_cast<mdp_client_t*>(ptr);
	mdp_client_set_timeout( handle, timeout );

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

System::String^ Client::BrokerBinding::get() {
	return _brokerBinding;
}

void Client::Timeout::set( int timeout ) {
	client_set_timeout( _mdpClientHandle->ToPointer(), timeout );
}


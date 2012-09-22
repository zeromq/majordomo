// zeromq.majordomo.h

#pragma once



using namespace System;
using namespace System::Runtime::InteropServices;

namespace zeromq { namespace majordomo {

	public ref class Client
	{
		IntPtr^ _mdpClientHandle;
		System::String^ _brokerBinding;
	public:
		Client( System::String^ BrokerBinding ); 
		Client( System::String^ BrokerBinding, bool verbose );
		~Client();
		property System::String^ BrokerBinding  {
			System::String^ get();

		}
		property int Timeout {

			void set( int timeout ) ;
		}
	};
} }

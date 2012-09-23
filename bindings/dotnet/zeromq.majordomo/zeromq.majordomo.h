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

		void Send( System::String^ Service, System::String^ Message );
		System::String^ Recv( System::String^% Service );
		

		property System::String^ BrokerBinding  {
			System::String^ get();

		}
		property int Timeout {

			void set( int timeout ) ;
		}
	};

	public ref class Worker {
		IntPtr^ _mdpWorkerHandle;
		System::String^ _brokerBinding;
		System::String^ _service;


	public:
		Worker( System::String^ BrokerBinding, System::String^ Service );
		Worker( System::String^ BrokerBinding, System::String^ Service, bool verbose );
		~Worker();

		System::String^ Recv( System::String^% ReplyTo );
		void Send( System::String^ Report, System::String^ ReplyTo );

		property System::String^ BrokerBinding {
			System::String^ get();
		}

		property System::String^ Service {
			System::String^ get();
		}

		property int Heartbeat {
			void set( int heartbeat );
		}

		property int Reconnect {
			void set( int reconnect );
		}
		
	};
} }

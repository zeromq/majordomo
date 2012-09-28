.NET Bindings for MajorDomo Client

Dependencies
-------------------------------
mdpwrapper depends on zeromq, libmdp and czmq.  The libraries and header files for these dependencies
are in the deps directory under the solution directory. 

Projects
------------------------------
mdpwrapper is a Windows DLL that wraps libmdp so that we can use some of libmdp's opaque functions
that would not be available to us in the managed environment.

zeromq.majordomo.csharp is the assembly that provides a facade of the majordomo client.  You would
create an application include a reference to zeromq.majordomo.csharp and go to town.  Note that
libzmq.dll libmdp.dll and mdpwrapper.dll have to be in your applications search path.  

Usage Example:

// instantiates client, sends a message and cleans up
using( Client c = new Client( "tcp://192.168.1.7:5555" ) )
{
	c.Send( "my service", "I like cheese" );
	Response r = c.Recv();

	if( r != null )
	{
		r.Print();
	}

}

zeromq.majordomo.test.clientconsole is an application to test the zeromq.majordomo.csharp assembly


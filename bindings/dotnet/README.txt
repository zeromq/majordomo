.NET Bindings for MajorDomo Client

Dependencies
-------------------------------
mdpwrapper depends on zeromq 3, libmdp and czmq.  The libraries and header files for these dependencies
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
using (Client client = new Client("tcp://192.168.1.7:5555", true))
{
	Request request = client.CreateRequest("echo") ;
	request.PushMem(Encoding.Unicode.GetBytes("foo bar baz")); 
	request.PushString("Tasty cheese!");
	request.Send();

	// reads response, using calls Dispose to clean up resources when
	// response goes out of scope
    using (Response response = client.Recv())
    {
        string stringcontents = response.PopString();
        byte[] buff = response.PopMem();
        string buffcontents = Encoding.Unicode.GetString(buff);
    }
                

                 
}

zeromq.majordomo.test.clientconsole is an application to test the zeromq.majordomo.csharp assembly


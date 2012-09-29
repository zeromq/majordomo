using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using zeromq.majordomo.csharp;




namespace zeromq.majordomo.testclient
{


    class Program
    {
        static void Main(string[] args)
        {
            using (Client client = new Client("tcp://192.168.1.7:5555", true))
            {
                Request request = new Request("echo");
                request.PushMem(Encoding.Unicode.GetBytes("foo bar baz"));

                client.Send("echo", "hello");
                
                Response response = client.Recv();
                if (response != null)
                {
                    response.Print();
                }
                 
            }

        }
    }
}

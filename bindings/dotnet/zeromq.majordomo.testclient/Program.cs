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
                string membuff = "foo bar baz";
                string stringbuff = "123 abcdi";
                Request request = client.CreateRequest("echo") ;
                request.PushMem(Encoding.Unicode.GetBytes("foo bar baz")); 
                request.PushString(stringbuff);
                request.Send();
                try
                {
                    request.PushString("foo");
                    Console.WriteLine("Test fail, we should never get here");
                }
                catch (Exception e)
                {
                    Console.WriteLine(string.Format("Caught expected exception - {0}", e.Message));
                }



                using (Response response = client.Recv())
                {
                    string stringcontents = response.PopString();
                    byte[] buff = response.PopMem();
                    string buffcontents = Encoding.Unicode.GetString(buff);

                    if (buffcontents != membuff)
                    {
                        Console.WriteLine(string.Format("push/pull memory test failed! Expected {0} Got {1}", membuff, buffcontents));
                           
                    }
                    else
                    {
                        Console.WriteLine("push/pull memory test passed.");
                    }


                    if (stringcontents != stringbuff)
                    {
                        Console.WriteLine(string.Format("push/pull string test failed! Expected {0} Got {1}", stringbuff, stringcontents));

                    }
                    else
                    { 
                        Console.WriteLine("push/pull string test passed.");
                    }
                }
                

                 
            }

        }
    }
}

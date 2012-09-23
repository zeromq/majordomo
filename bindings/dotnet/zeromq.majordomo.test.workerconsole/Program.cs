using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;


namespace zeromq.majordomo.test.workerconsole
{
    class Program
    {
        static void Main(string[] args)
        {
            Worker worker = new Worker("tcp://192.168.1.7:5555", "test", true);
            while (true)
            {
                string reply = "";
                string msg = worker.Recv(ref reply);
                Console.WriteLine( "Got " + msg + " reply to " + reply );
                worker.Send("pong", reply);
            }

             
        }
    }
}

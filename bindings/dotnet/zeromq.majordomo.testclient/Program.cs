using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;



namespace zeromq.majordomo.testclient
{
    class Program
    {
        static void Main(string[] args)
        {
            Client client = new Client("tcp:://192.168.1.7:5555", true );
            client.Timeout = 30;
            Console.WriteLine(client.BrokerBinding);
           
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace zeromq.majordomo.csharp
{
    public class Response
    {
        private string service ;
        private string message;

        internal Response( string service, string message )
        {
            this.service = service;
            this.message = message;
        }

        public string Service
        {
            get
            {
                return this.service;
            }
        }

        public string Message
        {
            get
            {
                return this.message;
            }
        }

        public void Print()
        {
            Console.WriteLine("Response");
            Console.WriteLine(string.Format("   Service: {0}", this.service ));
            Console.WriteLine(string.Format("   Message: {0}", this.message ));
        }


    }
}

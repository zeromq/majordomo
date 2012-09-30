using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace zeromq.majordomo.csharp
{
    public class Response :IDisposable
    {
        private string service ;
        private IntPtr msg_handle;

        internal Response( IntPtr handle, string service )
        {
            this.service = service;
            this.msg_handle = handle;

        }

        public string PopString()
        {
            return Wrapper.pop_str(msg_handle);
        }

        public byte[] PopMem()
        {
            return Wrapper.pop_mem(msg_handle);
        }

        public string Service
        {
            get
            {
                return this.service;
            }
        }


        public void Print()
        {
            Console.WriteLine("Response");
            Console.WriteLine(string.Format("   Service: {0}", this.service ));
        //    Console.WriteLine(string.Format("   Message: {0}", this.message ));
        }


        // ALWAYS call to clean up
        public void Dispose()
        {
            if (msg_handle != IntPtr.Zero)
            {
                Wrapper.msg_destroy(msg_handle);
            }
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace zeromq.majordomo.csharp
{
    public class Request 
    {
        IntPtr handle;
        IntPtr msg_handle;
        String service;
        bool sent = false;

        private void CheckSent()
        {
            if( sent ) 
            {
                throw new Exception( @"Adding information to request is not allowed once the request has been sent" );
            }
        }

        internal Request(IntPtr handle, String service)
        {
            this.service = service;
            this.handle = handle;
            msg_handle = Wrapper.msg_new();
        }

        public void PushString(String data)
        {
            CheckSent();
            Wrapper.push_str(msg_handle, data);
           
        }

        public void PushMem(byte[] buffer)
        {
            CheckSent();
            Wrapper.push_mem(msg_handle, buffer);
        }

        public void Send()
        {
            CheckSent();
            Wrapper.client_send(handle, this.service, this.msg_handle);
            sent = true;
        }






    }
}

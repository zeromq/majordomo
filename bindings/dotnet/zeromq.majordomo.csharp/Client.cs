using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace zeromq.majordomo.csharp
{
    public struct Wrapper
    {
        [DllImport("mdpwrapper.dll", CharSet=CharSet.Ansi, CallingConvention=CallingConvention.Cdecl, EntryPoint="#3")]
        static extern IntPtr _client_new(String broker, int verbose);
        [DllImport("mdpwrapper.dll", CallingConvention=CallingConvention.Cdecl, EntryPoint="#1")]
        static extern void _client_destroy(out IntPtr handle);
        [DllImport("mdpwrapper.dll", CharSet=CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "#7")]
        static extern void _client_send_string(IntPtr handle, String service, String message );
        [DllImport("mdpwrapper.dll",  CallingConvention = CallingConvention.Cdecl, EntryPoint = "#4")]
        static extern IntPtr _client_recv(IntPtr handle, IntPtr service );
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#8")]
        static extern void _client_set_timeout(IntPtr handle, int timeout);
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#12")]
        static extern IntPtr _msg_new();
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#14")]
        static extern int _push_mem(IntPtr msg_handle, byte[] buffer, int length );
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#5")]
        static extern void _client_send( IntPtr handle, String service, out IntPtr msg_handle );
        [DllImport("mdpwrapper.dll", CharSet=CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "#15")]
        static extern int _push_str(IntPtr msg_handle, String buffer);
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#13")]
        static extern int _pop_mem(IntPtr msg_handle, IntPtr buffer );
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#11")]
        static extern void _msg_destroy(out IntPtr msg_handle);

        internal static void msg_destroy(IntPtr msg_handle)
        {
            _msg_destroy(out msg_handle);
        }

        internal static byte[] pop_mem(IntPtr msg_handle)
        {
            byte[] response = null;
            IntPtr ptrptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            int size = _pop_mem(msg_handle, ptrptr);
            IntPtr buffptr = (IntPtr)Marshal.PtrToStructure(ptrptr, typeof(IntPtr));
            if (size > 0)
            {
                response = new byte[size];
                Marshal.Copy(buffptr, response, 0, size);
            }
            Marshal.FreeHGlobal(ptrptr);
            return response;

        }

        internal static String pop_str(IntPtr msg_handle)
        {
            string response = null;
            byte[] buff = pop_mem(msg_handle);
            if (buff != null)
            {
                response = System.Text.ASCIIEncoding.ASCII.GetString(buff);

            }
            return response;
                
        }

        internal static void client_send(IntPtr handle, String service, IntPtr msg_handle )
        {           
            _client_send(handle, service, out msg_handle);
        }

        internal static void push_str(IntPtr msg_handle, String buffer)
        {
            int rc = _push_str(msg_handle, buffer);
            if (rc != 0)
            {
                throw new System.Exception("Attempt to push string onto message failed.");
            }

        }

        internal static void push_mem(IntPtr msg_handle, byte[] buffer)
        {
            int rc = _push_mem(msg_handle, buffer, buffer.Length);
            if (rc != 0)
            {
                throw new System.Exception("Attempt to add buffer to message failed");
            }

        }


        internal static IntPtr msg_new()
        {
            return _msg_new();
        }

        internal static void client_set_timeout(IntPtr handle, int timeout)
        {
            _client_set_timeout(handle, timeout);
        }

        internal static IntPtr client_new(string broker, int p)
        {
            return _client_new( broker, p );
        }

        internal static void client_destroy(IntPtr handle)
        {
            _client_destroy(out handle);
        }

        internal static void client_send_string(IntPtr handle, string service, string message)
        {
            _client_send_string(handle, service, message);
        }

        
        internal static Response client_recv(IntPtr handle )
        {
           string service = string.Empty;
           IntPtr ppservice = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
           IntPtr msg_handle = _client_recv(handle, ppservice);
           IntPtr pservice = (IntPtr)Marshal.PtrToStructure(ppservice, typeof(IntPtr));
           
            if (pservice != IntPtr.Zero)
            {
                service = Marshal.PtrToStringAnsi(pservice);
            }
            Marshal.FreeHGlobal(ppservice);
            return new Response( msg_handle, service );


        }

        
    }

    public class Client : IDisposable
    {
        IntPtr handle;
        
        /*
         * Instantiate client and connect to broker. 
         * 
         * ex. Client c = new Client( "tcp://192.168.1.7:5555", true );
         */
        public Client(String broker, bool verbose)
        {
            handle = Wrapper.client_new(broker, verbose ? 1 : 0);
        }

        public Request CreateRequest(String service)
        {
            Request request = new Request(handle, service);
            return request;
        }

        /*
         * Send a message to worker identified by service argument
         * 
         * ex. c.Send( "echo", "I like cheese" );
         * 
         * Extended send functionality is provided in the Request object. 
         */
        public void Send( String service, String message )
        {
            Wrapper.client_send_string( handle, service, message );
        }


        /*
         * Handles response (if any from worker) 
         */
        public Response Recv()
        {

            return Wrapper.client_recv(handle);
        }

        public int Timeout
        {
            set
            {
                Wrapper.client_set_timeout(handle, value);   
            }
        }

      
        /*
         * Releases majordomo resources
         */ 
        public void  Dispose()
        {
            Wrapper.client_destroy(handle);
 	         
        }
  }
}

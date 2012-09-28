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
        [DllImport("mdpwrapper.dll", CharSet=CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "#6")]
        static extern void _client_send_string(IntPtr handle, String service, String message );
        [DllImport("mdpwrapper.dll",  CallingConvention = CallingConvention.Cdecl, EntryPoint = "#4")]
        static extern void _client_recv(IntPtr handle, IntPtr service, IntPtr message );
        [DllImport("mdpwrapper.dll", CallingConvention = CallingConvention.Cdecl, EntryPoint = "#6")]
        static extern void _client_set_timeout(IntPtr handle, int timeout);

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

        internal static void client_recv(IntPtr handle, ref string service, ref string message)
        {
           IntPtr ppservice = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
           IntPtr ppmessage = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr))); 

            _client_recv(handle, ppservice, ppmessage);
            IntPtr pmessage = (IntPtr)Marshal.PtrToStructure(ppmessage, typeof(IntPtr));
            message = Marshal.PtrToStringAnsi(pmessage);
            IntPtr pservice = (IntPtr)Marshal.PtrToStructure(ppservice, typeof(IntPtr));
            if (pservice != IntPtr.Zero)
            {
                service = Marshal.PtrToStringAnsi(pservice);
            }
            Marshal.FreeHGlobal(ppservice);
            Marshal.FreeHGlobal(ppmessage);


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

        /*
         * Send a message to worker identified by service argument
         * 
         * ex. c.Send( "echo", "I like cheese" );
         */
        public void Send( String service, String message )
        {
            Wrapper.client_send_string( handle, service, message );
        }

        /*
         * Handles response (if any from worker) response may be null
         */
        public Response Recv()
        {
            Response response = null;
            string service = string.Empty;
            string message = string.Empty;
            Wrapper.client_recv(handle, ref service, ref message);
            if( !string.IsNullOrEmpty( message ) )
            {
                response = new Response(service, message);
            }
            return response;
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

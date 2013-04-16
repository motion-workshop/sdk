/**
   @file    tools/sdk/java/Client.java
   @author  Luke Tokheim, luke@motionnode.com
   @version 2.0

   (C) Copyright Motion Workshop 2013. All rights reserved.

   The coded instructions, statements, computer programs, and/or related
   material (collectively the "Data") in these files contain unpublished
   information proprietary to Motion Workshop, which is protected by
   US federal copyright law and by international treaties.

   The Data may not be disclosed or distributed to third parties, in whole
   or in part, without the prior written consent of Motion Workshop.

   The Data is provided "as is" without express or implied warranty, and
   with no claim as to its suitability for any purpose.
*/
package Motion.SDK;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
   Implements a simple socket based data client and the Motion Service
   stream binary message protocol. Provide a simple interface to
   develop external applications that can read real-time Motion device
   data.

   This class only handles the socket connection and single message
   transfer. The {@link Format} class implements interfaces to the
   service specific data formats.

   <p>
   Example usage:
   </p>
   <code>
   <pre>
   // Connect to port 32078 on localhost.
   Client client = new Client(null, 32078);
   while (true) {
     if (client.waitForData()) {
       while (true) {
         // Block on the open connection until a message comes in.
         {@link ByteBuffer} data = client.{@link Client#readData};
         if (null == data) {
           break;
         }

         // Process the message. Use the {@link Format} methods, for example.
       }
     }
   }
   </pre>
   </code>
*/
public class Client
{
    /**
       A message is defined by an integer packet prefix field. Set this
       maximum mostly as a safeguard. We do not want to allocate a huge
       amount of memory at once.
    */
    private static final int MaximumMessageLength = 65535;

    /**
       Default value, in seconds, for the socket receive time out
       in the Client#waitForData method. Zero denotes blocking receive.
    */
    private static final int TimeOutWaitForData = 5;

    /**
       Default value, in seconds, for the socket receive time out
       in the Client#readData method.
    */
    private static final int TimeOutReadData = 1;

    /**
       Default value, in seconds, for the socket send time out
       in the Client#writeData method.
    */
    private static final int TimeOutSendData = 1;


    /**
       The low-level socket connection.
    */
    private Socket socket = null;

    /**
       Stream input from the socket.
    */
    private DataInputStream in = null;

    /**
       Stream output to the socket.
    */
    private DataOutputStream out = null;

    /**
       Store the remote service description string. 
    */
    private String description = null;

    /**
       Store the current value of the socket receive time out.
    */
    private int time_out_second = 0;

    /**
       Store the current value of the socket send time out.
    */
    private int time_out_second_send = 0;

    /**
       Open a Motion Service client connection.

       @param host the host name, or null for the loopback address.
       @param port the port number
       
       @see java.net.Socket#Socket(String, int)
    */
    public Client(String host, int port)
        throws UnknownHostException,
            IOException,
            SecurityException
    {
        socket = new Socket(host, port);

        in = new DataInputStream(socket.getInputStream());
        out = new DataOutputStream(socket.getOutputStream());

        // Read the first message from the service. It is a 
        // string description of the remote service. Blocking
        // call.
        byte[] buffer = receive();
        if (null != buffer)
        {
            description = new String(buffer);
        }
    }

    /**
       Close this connection.

       @see java.net.Socket#close()
       @see java.io.DataInputStream#close()
    */
    public void close()
        throws IOException
    {
        in.close();
        socket.close();
    }

    /**
       Returns true iff this connection is active.
     */
    public boolean isConnected()
    {
      return socket.isConnected();
    }

    /**
       Wait until there is incoming data on the open socket connection.
       This method will time out and return <code>false</code> if no
       data arrives after Client#TimeOutWaitForData seconds.
       
       @return <code>true</code> when incoming data arrives, or
       <code>false</code> on a time out.
    */
    public boolean waitForData(int time_out)
        throws IOException
    {
        boolean result = false;

        if (-1 == time_out)
        {
            time_out = TimeOutWaitForData;
        }

        if (time_out != time_out_second)
        {
            socket.setSoTimeout(TimeOutWaitForData * 1000);
            time_out_second = TimeOutWaitForData;
        }

        byte[] buffer = receive();
        if (null != buffer)
        {
            result = true;
        }

        return result;
    }

    /**
       Implements default parameter value.
    */
    public boolean waitForData()
        throws IOException
    {
        return waitForData(-1);
    }

    /**
       Read a single sample of data from the open connection. This
       method will time out and return <code>null</code> if no data
       comes in after Client.TimeOutReadData seconds.
       
       @return a single sample of data, or <code>null</code> if the
       incoming data is invalid
    */
    public ByteBuffer readData(int time_out)
        throws IOException
    {
        ByteBuffer result = null;

        if (-1 == time_out)
        {
            time_out = TimeOutReadData;
        }
        if (time_out != time_out_second)
        {
            socket.setSoTimeout(time_out * 1000);
            time_out_second = time_out;
        }

        byte[] buffer = receive();
        if (null != buffer)
        {
            // All service data is little-endian. Use the nifty
            // ByteBuffer to implement cross platform byte swapping.
            ByteBuffer nio_buffer = ByteBuffer.wrap(buffer);
            nio_buffer.order(ByteOrder.LITTLE_ENDIAN);

            result = nio_buffer;
        }

        return result;
    }

    /**
       Implements default parameter value.
    */
    public ByteBuffer readData()
        throws IOException
    {
        return readData(-1);
    }

    public boolean writeData(ByteBuffer data, int time_out)
        throws IOException
    {
        boolean result = false;

        if (-1 == time_out)
        {
            time_out = TimeOutSendData;
        }
        if (time_out != time_out_second_send)
        {
            socket.setSoTimeout(time_out * 1000);
            time_out_second_send = time_out;
        }

        data.order(ByteOrder.LITTLE_ENDIAN);
        result = send(data.array());

        return result;
    }

    /**
       Implements default parameter value.
    */
    public boolean writeData(ByteBuffer data)
        throws IOException
    {
        return writeData(data, -1);
    }

    /**
       Receive a single binary message over the open socket
       connection. Reads the message length header and then
       the number of bytes in the incoming message. For
       internal use only.

       @return a single binary message, or <code>null</code> if the
       incoming data is invalid
    */
    private byte[] receive()
        throws IOException
    {
        byte[] result = null;

        try
        {
            int length = in.readInt();
            if ((length > 0) && (length < MaximumMessageLength))
            {
                byte[] buffer = new byte[length];
                if (length == in.read(buffer))
                {
                    result = buffer;
                }
            }
        }
        catch (SocketTimeoutException e)
        {
            // Catch any socket read time outs due to the current
            // SO_TIMEOUT setting.
        }

        return result;
    }

    /**
       Send a single binary message over the open socket
       connection. For internal use only.

       @return true iff the output message was valid and
       as successfully sent.
    */
    private boolean send(byte[] buffer)
        throws IOException
    {
        boolean result = false;

        try
        {
            int length = buffer.length;
            if ((length > 0) && (length < MaximumMessageLength))
            {
                out.writeInt(length);
                out.write(buffer);

                result = true;
            }
        }
        catch (SocketTimeoutException e)
        {
            // Catch any socket read time outs due to the current
            // SO_TIMEOUT setting.
        }

        return result;
    }


    /**
       Example usage and test function for the Client class.
    */
    public static void main(String[] args)
        throws UnknownHostException,
            IOException,
            SecurityException
    {
        // Connect to port 32079 on localhost.
        String host = "";
        int port = 32079;

        // Parse a single command line argument that defines
        // the remote host and port.
        // For example, "www.google.com:80" or "10.0.0.1:32079"
        //
        // The port argument is optional and defaults to 32079.
        //
        if (args.length > 0)
        {
            String[] token_list = args[0].split(":");
            if (token_list.length > 0)
            {
                host = token_list[0];
            }
            if (token_list.length > 1)
            {
                port = Integer.parseInt(token_list[1]);
            }
        }

        Client client = new Client(host, port);

        System.out.println("Connected to " + host + ":" + port);

        while (true)
        {
            if (client.waitForData())
            {
                while (true)
                {
                    ByteBuffer data = client.readData();
                    if (null == data)
                    {
                        break;
                    }

                    System.out.println("Incoming message, remaining bytes = " + data.remaining());
                }
            }
        }
    }
} // class Client

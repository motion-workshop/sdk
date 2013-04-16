/**
  Implementation of the Client class. Handle socket message protocol
  for connections to Motion Service data streams.

  @file    tools/sdk/cs/Client.cs
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
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Motion
{
  namespace SDK
  {
    /**
      Implements a simple socket based data client and the Motion Service
      stream binary message protocol. Provide a simple interface to
      develop external applications that can read real-time Motion device
      data.

      This class only handles the socket connection and single message
      transfer. The @ref Format class implements interfaces to the
      service specific data formats.

      Example usage:
      @code
      //using Motion.SDK
      // ...
      
      try {
        // Open connection to a Motion Service on the localhost,
        // port 32079.
        Client client = new Client("", 32079);

        // This is application dependent. Use a loop to keep trying
        // to read data even after time outs.
        while (true) {
          // Wait until there is incoming data on the open connection,
          // timing out after 5 seconds.
          if (client.waitForData()) {
            // Read data samples in a loop. This will block by default
            // for 1 second. We can simply wait on an open connection
            // until a data sample comes in, or fall back to the
            // wait loop.
            while (true) {
              byte[] data = client.readData(data));
              if (null == data) {
                break;       
              }
    
              // Do something useful with the current real-time sample.
              // Probably use the Format class to generate a IDictionary
              // object of [int] => Format.(Preview|Sensor|Raw)Element
              // elements.
            }
          }
        }

      } catch (Exception e) {
        // The Client class with throw exceptions for any error
        // conditions. Even if the connection to the remote host fails.
      }
      @endcode
    */
    public class Client
    {
      /**
        @param   host IP address of remote host, the empty string
                 defaults to "127.0.0.1"
        @param   port port name of remote host
        @pre     a service is accepting connections on the socket
                 described by the host, port pair
        @post    this client is connected to the remote service described
                 by the host, port pair
        @throws  std::runtime_error if the client connection fails for
                 any reason
       */
      public Client(String host, int port)
      {
        if (0 == host.Length)
        {
          host = DefaultAddress;
        }
        if (port < 0)
        {
          port = 0;
        }


        Socket socket = null;
        IPHostEntry ipHostInfo = Dns.GetHostEntry(host);
        foreach (IPAddress ipAddress in ipHostInfo.AddressList)
        {
          IPEndPoint ipe = new IPEndPoint(ipAddress, port);
          if (ProtocolFamily.InterNetworkV6.ToString() == ipe.AddressFamily.ToString())
          {
            continue;
          }

          Socket tempSocket = new Socket(
            ipe.AddressFamily,
            SocketType.Stream,
            ProtocolType.Tcp);

          tempSocket.Connect(ipe);
          if (tempSocket.Connected)
          {
            socket = tempSocket;
            break;
          }
        }

        if ((null != socket) && socket.Connected)
        {
          socket.NoDelay = true;
          m_socket = socket;

          // Read the first message. This is an XML
          // description of the service.
          m_socket.ReceiveTimeout = TimeOutWaitForData * 1000;
          byte[] message = receive();
          if (message.Length > 0)
          {
            m_description = Encoding.UTF8.GetString(message, 0, message.Length);
          }
        }
      }

      /**
        Does not throw any exceptions. Close this client connection
        if it is open.
      */
      ~Client()
      {
        try
        {
          close();
        }
        catch (Exception)
        {
        }
      }

      /**
        Close the current client connection.

        @throws  Exception if this client is not connected
                 or the close procedure fails for any reason
      */
      public void close()
      {
        if ((null != m_socket) && m_socket.Connected)
        {
          m_socket.Shutdown(SocketShutdown.Both);
          m_socket.Close();

          m_socket = null;
          m_xml_string = "";
        }
      }

      /**
        Returns true if the current connection is active.
      */
      public bool isConnected()
      {
        if ((null != m_socket) && m_socket.Connected) {
          return true;
        } else {
          return false;
        }
      }

      /**
         Read a single sample of data from the open connection. This
         method will time out and return null if no data comes in
         after time_out_second seconds.
       
         @return a single sample of data, or null if the
         incoming data is invalid
      */
      public byte[] readData(int time_out_second)
      {
        byte[] data = null;
        if (isConnected()) {
          // Set receive time out for this set of calls.
          if (time_out_second < 0)
          {
            if (TimeOutReadData * 1000 != m_socket.ReceiveTimeout)
            {
              m_socket.ReceiveTimeout = TimeOutReadData * 1000;
            }
          }
          else
          {
            m_socket.ReceiveTimeout = time_out_second * 1000;
          }

          byte[] message = receive();

          // Consume any XML messages. Store the most recent one.
          if ((null != message)
            && (message.Length >= XMLMagic.Length)
            && (XMLMagic == Encoding.UTF8.GetString(message, 0, XMLMagic.Length)))
          {
            m_xml_string = Encoding.UTF8.GetString(message, 0, message.Length);

            message = receive();
          }

          if ((null != message) && (message.Length > 0))
          {
            data = message;
          }

        }

        return data;
      }

      /**
         @ref Client#readData(-1)
      */
      public byte[] readData()
      {
        return readData(-1);
      }

      /**
        Wait until there is incoming data on this client
        connection and then returns true.

        @param   time_out_second time out and return false after
                 this many seconds, 0 value specifies no time out,
                 negative value specifies default time out
        @pre     this object has an open socket connection
      */
      public bool waitForData(int time_out_second)
      {
        bool result = false;

        if (isConnected())
        {
          // Set receive time out for this set of calls.
          if (time_out_second < 0)
          {
            if (TimeOutReadData * 1000 != m_socket.ReceiveTimeout)
            {
              m_socket.ReceiveTimeout = TimeOutWaitForData * 1000;
            }
          }
          else
          {
            m_socket.ReceiveTimeout = time_out_second * 1000;
          }

          byte[] message = receive();
          if ((null != message) && (message.Length > 0))
          {
            if ((message.Length >= XMLMagic.Length) && (XMLMagic == Encoding.UTF8.GetString(message, 0, XMLMagic.Length)))
            {
              m_xml_string = Encoding.UTF8.GetString(message, 0, message.Length);
            }

            result = true;
          }
        }

        return result;
      }
      
      /**
        @ref Client#waitForData(-1)
      */
      public bool waitForData()
      {
        return waitForData(-1);
      }

      /**
        Write a variable length binary message to the
        socket link.

        @param   time_out_second time out and return false after
                 this many seconds, 0 value specifies no time out,
                 negative value specifies default time out
        @pre     this object has an open socket connection
      */
      public bool writeData(byte[] data, int time_out_second)
      {
        bool result = false;

        if (isConnected() && (null != data) && (data.Length > 0))
        {
          // Set receive time out for this set of calls.
          if (time_out_second < 0)
          {
            if (TimeOutWriteData * 1000 != m_socket.SendTimeout)
            {
              m_socket.SendTimeout = TimeOutWriteData * 1000;
            }
          }
          else
          {
            m_socket.SendTimeout = time_out_second * 1000;
          }

          byte[] header = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(data.Length));
          if (null != header)
          {
            byte[] buffer = new byte[header.Length + data.Length];
            {
              System.Array.Copy(header, 0, buffer, 0, header.Length);
              System.Array.Copy(data, 0, buffer, header.Length, data.Length);
            }

            if (buffer.Length == m_socket.Send(buffer))
            {
              result = true;
            }
          }
        }

        return result;
      }

      /**
        @ref Client#writeData(byte[], int)
      */
      public bool writeData(byte[] data)
      {
        return writeData(data, -1);
      }

      /**
        Create a byte[] from the input string and pass it
        along to the writeData method.
       
        @ref Client#writeData(byte[], int)
      */
      public bool writeData(String message, int time_out_second)
      {
        return writeData(Encoding.ASCII.GetBytes(message), -1);
      }

      /**
        @ref Client#writeData(String, int)
      */
      public bool writeData(String message)
      {
        return writeData(message, -1);
      }

      /**
       Implements the actual message reading. First get the message length
       header integer = N. Then read N bytes of raw data and return them.
       */
      private byte[] receive()
      {
        byte[] data = null;

        if (isConnected())
        {
          byte[] buffer = new byte[2048];
          int message_length = 0;
          if (sizeof(int) == m_socket.Receive(buffer, sizeof(int), SocketFlags.None))
          {
            // Network order message length integer header.
            message_length = IPAddress.NetworkToHostOrder(BitConverter.ToInt32(buffer, 0));
          }

          if ((message_length > 0) && (message_length < buffer.Length))
          {
            // Read the binary message.
            if (message_length == m_socket.Receive(buffer, message_length, SocketFlags.None))
            {
              data = new byte[message_length];
              System.Array.Copy(buffer, 0, data, 0, message_length);
            }
          }
        }

        return data;
      }

      /** Built in Socket class. Does most of the work. */
      private Socket m_socket = null;

      /** String description of the remote service. */
      private String m_description = "";

      /** Last XML message received. */
      private String m_xml_string = "";

      /**
        Default value, in seconds, for the socket receive time out
        in the Client#readData method.
      */
      private const int TimeOutReadData = 1;

      /**
        Default value, in seconds, for the socket send time out
        in the Client#writeData method.
      */
      private const int TimeOutWriteData = 1;

      /**
        Default value, in seconds, for the socket receive time out
        in the Client#waitForData method. Zero denotes blocking receive.
      */
      private const int TimeOutWaitForData = 5;

      /**
        Set the address to this value if we get an empty string.
      */
      private const String DefaultAddress = "127.0.0.1";

      /**
        Detect XML message with the following header bytes.
      */
      private const String XMLMagic = "<?xml";
    } // class Client

  } // namespace SDK
} // namespace Motion

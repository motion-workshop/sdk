/**
   @file    tools/sdk/java/LuaConsole.java
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

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;


/**
   The LuaConsole class sends a Lua chunk to the Motion Service console,
   parses the result code, and returns the printed output. Requires an
   existing Client connection to the Motion Service console.

   <p>
   Example usage:
   </p>
   <code>
   <pre>
   // Send this chunk. Print something out.
   String chunk = "print('Hello World')";

   // Connect to the Motion Service console.
   Client client = new Client(null, 32075);

   // Create a LuaConsole object with our existing connection.
   LuaConsole console = new LuaConsole(client);

   LuaConsole.Result result = console.SendChunk(chunk, 5);
   if (LuaConsole.ResultCode.Success == result.code) {
     // This should be "Hello World\n"
     System.out.println(result.string);
   } else if (LuaConsole.ResultCode.Continue == result.code) {
     System.err.println("incomplete Lua chunk: " + result.string);
   } else {
     System.err.println("command failed: " + result.string);
   }
   </pre>
   </code>
   @endcode
*/
public class LuaConsole
{
    /**
       Defines the possible result codes from the console service.
    */
    public enum ResultCode
    {
        // The Lua chunk was successfully parsed and executed. The
        // printed results are in the result string.
        Success,
        // The Lua chunk failed due to a compile time or execution
        // time error. An error description is in the result string.
        Failure,
        // The Lua chunk was incomplete. The Console service is waiting
        // for a complete chunk before it executes.
        // For example, "if x > 1 then" is incomplete since it requires
        // and "end" token to close the "if" control statement.
        Continue
    }

    /**
       Simple container class for the result code and printed results
       from a Lua chunk.
    */
    public class Result
    {
        public ResultCode code = ResultCode.Failure;
        public String string = null;
    }; // class Result

    /**
      Store the open Client as a class member.
    */
    private Client client = null;

    /**
       Create a console object with an existing console Client. 

       @param console_client a {@link Client} with an existing
              connection to the Motion Service console.
    */
    public LuaConsole(Client console_client)
    {
        client = console_client;
    }

    public Result SendChunk(String chunk, int time_out_second)
        throws Exception, IOException
    {
        Result result = new Result();

        ByteBuffer data = ByteBuffer.wrap(chunk.getBytes());
        if (client.writeData(data, time_out_second))
        {
            data = client.readData(time_out_second);
            if (null != data)
            {
                byte code = data.get();
                if (0 == code)
                {
                    result.code = ResultCode.Success;
                }
                else if (1 == code)
                {
                    result.code = ResultCode.Continue;
                }
                else if (2 == code)
                {
                    result.code = ResultCode.Failure;
                }
                else
                {
                    throw new Exception("unknown return code from Console service");
                }

                if (data.hasRemaining())
                {
                    result.string = new String(data.array(), data.position(), data.remaining());
                }
            }
        }

        return result;
    }

    /**
       Example usage and test function for the Client class.
    */
    public static void main(String[] args)
        throws Exception, UnknownHostException, IOException, SecurityException
    {
        // Connect to port 32075 on localhost.
        String host = "";
        int port = 32075;

        // Parse a single command line argument that defines
        // the remote host and port.
        // For example, "www.google.com:80" or "10.0.0.1:32075"
        //
        // The port argument is optional and defaults to 32075.
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


        // Make a Lua chunk, which is just a number of Lua commands
        // that are compiled as one unit. We can only receive the
        // printed results as feedback.
        String chunk =
          "if not node.is_reading() then" +
          "   node.close()" +
          "   node.scan()" +
          "   node.start()" +
          " end" +
          " if node.is_reading() then" +
          "   print('Reading from ' .. node.num_reading() .. ' device(s)')" +
          " else" +
          "   print('Failed to start reading')" +
          " end";

        LuaConsole console = new LuaConsole(client);

        LuaConsole.Result result = console.SendChunk(chunk, 5);
        if (LuaConsole.ResultCode.Success == result.code)
        {
            System.out.println(result.string);
        }
        else if (LuaConsole.ResultCode.Continue == result.code)
        {
            System.err.println("incomplete Lua chunk: " + result.string);
        }
        else
        {
            System.err.println("command failed: " + result.string);
        }

    }
} // class LuaConsole

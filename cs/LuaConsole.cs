/**
  @file    tools/sdk/cs/LuaConsole.cs
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
using System.Text;

namespace Motion
{
  namespace SDK
  {
    /**
    The LuaConsole class sends a Lua chunk to the Motion Serivce console, parses
    the result code, and returns the printed output. Requires an existing Client
    connection to the Motion Service console.

    Example usage:
    @code
    //using Motion.SDK;
    // ...
    
    try {
      Client client = new Client("", 32075);

      LuaConsole.ResultType result =
        LuaConsole.SendChunk(
          client,
          "node.close() node.set_gselect(6) node.start()" +
          " print('node.is_reading() == ', node.is_reading())"
        );

      if (LuaConsole.ResultCode.Success == result.first)
      {
        // Could be "node.is_reading = true\n" or
        // "node.is_reading = false\n".
        Console.WriteLine(result.second);
      }
      else if (LuaConsole.ResultCode.Continue == result.first)
      {
        Console.WriteLine("incomplete Lua chunk: " + result.second);
      }
      else
      {
        Console.WriteLine("command failed: " + result.second);
      }

      client.close();

    } catch (Exception e) {
      // The Client and LuaConsole class with throw Exception objects for
      // any unrecoverable conditions.
    }
    @endcode
    */
    public static class LuaConsole
    {
      public enum ResultCode
      {
        // The Lua chunk was successfully parsed and executed. The
        // printed results are in the result string.
        Success = 0,
        // The Lua chunk failed due to a compile time or execution
        // time error. An error description is in the result string.
        Failure = 1,
        // The Lua chunk was incomplete. The Console service is waiting
        // for a complete chunk before it executes.
        // For example, "if x > 1 then" is incomplete since it requires
        // and "end" token to close the "if" control statement.
        Continue = 2
      }; // enum ResultCode

      public class ResultType
      {
        public ResultCode first = ResultCode.Failure;
        public String second = "";
      } // class ResultType

      /**
        Write a general Lua chunk to the open Console service
        socket and read back the results.
      */
      public static ResultType SendChunk(
        Client client,
        String chunk,
        int time_out_second)
      {
        ResultType result = new ResultType();

        if (client.writeData(chunk, time_out_second))
        {
          byte[] response = client.readData(time_out_second);
          if ((null != response) && (response.Length > 0))
          {
            // First character is the response code.
            byte code = response[0];
            if ((code >= (byte)ResultCode.Success)
              && (code <= (byte)ResultCode.Continue))
            {
              result.first = (ResultCode)code;
              // The rest of the message is any printed output from the
              // Lua environment.
              if (response.Length > 1)
              {
                result.second =
                  Encoding.ASCII.GetString(
                    response,
                    1,
                    response.Length - 1);
              }
            }
          }
        }

        return result;
      }

      public static ResultType SendChunk(Client client, String chunk)
      {
        return SendChunk(client, chunk, -1);
      }

    } // class LuaConsole

  } // namespace SDK
} // namespace Motion

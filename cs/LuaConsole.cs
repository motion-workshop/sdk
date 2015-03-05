/**
  @file    tools/sdk/cs/LuaConsole.cs
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.2

  Copyright (c) 2015, Motion Workshop
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
using System;
using System.Text;

namespace Motion {
  namespace SDK {
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

      if (LuaConsole.ResultCode.Success == result.first) {
        // Could be "node.is_reading = true\n" or
        // "node.is_reading = false\n".
        Console.WriteLine(result.second);
      } else if (LuaConsole.ResultCode.Continue == result.first) {
        Console.WriteLine("incomplete Lua chunk: " + result.second);
      } else {
        Console.WriteLine("command failed: " + result.second);
      }

      client.close();

    } catch (Exception e) {
      // The Client and LuaConsole class with throw Exception objects for
      // any unrecoverable conditions.
    }
    @endcode
    */
    public static class LuaConsole {
      public enum ResultCode {
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

      public class ResultType {
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
        int time_out_second) {
        ResultType result = new ResultType();

        if (client.writeData(chunk, time_out_second)) {
          byte[] response = client.readData(time_out_second);
          if ((null != response) && (response.Length > 0)) {
            // First character is the response code.
            byte code = response[0];
            if ((code >= (byte)ResultCode.Success)
              && (code <= (byte)ResultCode.Continue)) {
              result.first = (ResultCode)code;
              // The rest of the message is any printed output from the
              // Lua environment.
              if (response.Length > 1) {
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

      public static ResultType SendChunk(Client client, String chunk) {
        return SendChunk(client, chunk, -1);
      }

    } // class LuaConsole

  } // namespace SDK
} // namespace Motion

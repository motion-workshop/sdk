/*
  @file    tools/sdk/cpp/LuaConsole.hpp
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
#ifndef __MOTION_SDK_LUA_CONSOLE_HPP_
#define __MOTION_SDK_LUA_CONSOLE_HPP_

#include <string>
#include <utility>

#include <detail/exception.hpp>


namespace Motion { namespace SDK {

/**
   The LuaConsole class sends a Lua chunk to the Motion Service console, parses
   the result code, and returns the printed output. Requires an existing Client
   connection to the Motion Service console.

   Example usage:
   @code
   try {
     using Motion::SDK::Client;
     using Motion::SDK::LuaConsole;

     // Send this chunk. Print something out.
     std::string chunk = "print('Hello World')";

     // Connect to the Motion Service console port.
     Client client("", 32075);

     LuaConsole::result_type result =
       LuaConsole::SendChunk(client, chunk);
     if (LuaConsole::Success == result.first) {
       // This should be "Hello World\n"
       std::cout << result.second;
     } else if (LuaConsole::Continue == result.first) {
       std::cerr
         << "incomplete Lua chunk: " << result.second
         << std::endl;
     } else {
       std::cerr
         << "command failed: " << result.second
         << std::endl;
     }

   } catch (std::runtime_error &e) {
     // The Client and LuaConsole class with throw std::runtime_error
     // for any unrecoverable conditions.
   }
   @endcode
*/
class LuaConsole {
 public:
  enum ResultCode {
    // The Lua chunk was successfully parsed and executed. The
    // printed results are in the result string.
    Success  = 0,
    // The Lua chunk failed due to a compile time or execution
    // time error. An error description is in the result string.
    Failure  = 1,
    // The Lua chunk was incomplete. The Console service is waiting
    // for a complete chunk before it executes.
    // For example, "if x > 1 then" is incomplete since it requires
    // and "end" token to close the "if" control statement.
    Continue = 2
  };

  /**
    Bundle together a code and printed results, basic tuple.
  */
  typedef std::pair<
    ResultCode,
    std::string
  > result_type;

  /**
    Write a general Lua chunk to the open Console service
    socket and read back the results.
  */
  template <typename Client, typename String>
  static result_type SendChunk(Client &client,
                               const String &chunk,
                               const int &time_out_second=-1)
  {
    result_type result(Failure, result_type::second_type());

    typename Client::data_type data(chunk.begin(), chunk.end());

    if (client.writeData(data, time_out_second) &&
        client.readData(data, time_out_second) &&
        !data.empty()) {
      // First character is the response code.
      char code = data[0];
      if ((code >= Success) && (code <= Continue)) {
        result.first = static_cast<ResultCode>(code);
        // The rest of the message is any printed output from the
        // Lua environment.
        if (data.size() > 1) {
          result.second.assign(data.begin() + 1, data.end());
        }
      } else {
#if MOTION_SDK_USE_EXCEPTIONS
        throw detail::error("unknown return code from Console service");
#endif  // MOTION_SDK_USE_EXCEPTIONS
      }
    }

    return result;
  }

 private:
  /**
    Hide the constructor. There is no need to instantiate a LuaConsole object.
  */
  LuaConsole();
};  // class LuaConsole

}}  // namespace Motion::SDK

#endif  // __MOTION_SDK_LUA_CONSOLE_HPP_

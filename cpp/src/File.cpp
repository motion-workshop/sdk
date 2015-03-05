/**
  Implementation of the File class. See the header file for more details.

  @file    tools/sdk/cpp/src/File.cpp
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
#include <File.hpp>

#include <detail/exception.hpp>


namespace Motion { namespace SDK {

File::File(const std::string &pathname)
  : m_input(pathname.c_str(), std::ios_base::binary | std::ios_base::in)
{
#if MOTION_SDK_USE_EXCEPTIONS
  if (!m_input.is_open()) {
    throw detail::error("failed to open input file stream");
  }
#endif  // MOTION_SDK_USE_EXCEPTIONS
}

File::~File()
{
#if MOTION_SDK_USE_EXCEPTIONS
  try {
    close();
  } catch (detail::error &) {
  }
#else
  close();
#endif  // MOTION_SDK_USE_EXCEPTIONS
}

void File::close()
{
  if (m_input.is_open()) {
    m_input.close();
  } else {
#if MOTION_SDK_USE_EXCEPTIONS
    throw detail::error("failed to close input file stream, not open");
#endif  // MOTION_SDK_USE_EXCEPTIONS
  }
}

}}  // namespace Motion::SDK

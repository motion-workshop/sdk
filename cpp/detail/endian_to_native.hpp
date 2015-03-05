/**
  @file    tools/sdk/cpp/detail/endian_to_native.hpp
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
#ifndef __MOTION_SDK_DETAIL_ENDIAN_TO_NATIVE_HPP_
#define __MOTION_SDK_DETAIL_ENDIAN_TO_NATIVE_HPP_

#include "endian.hpp"

#include <algorithm>

#if defined(BOOST_BIG_ENDIAN)
#  define MOTION_SDK_BIG_ENDIAN 1
#else
#  if !defined(BOOST_LITTLE_ENDIAN)
#    error Unsupported endian platform
#  endif
#endif


namespace Motion { namespace SDK { namespace detail {

/**
  Generic byte swapping function for little-endian data to native type.
  This is certainly not the fastest way to handle byte-swapping, but
  consider it to be a fall-back implementation with minimal dependencies.

  Example usage:
  @code
  std::vector<int> buffer;
  
  // ... Read some binary data into the int buffer ...

  // Element-wise transformation from little-endian to native byte ordering.
  std::transform(
    buffer.begin(), buffer.end(),
    buffer.begin(), &detail::little_endian_to_native<int>);
  @endcode
*/
template <typename T>
inline T little_endian_to_native(T &value)
{
#if MOTION_SDK_BIG_ENDIAN
  T result = T();

  const char *p = reinterpret_cast<const char *>(&value);

  std::reverse_copy(
    p, p + sizeof(T),
    reinterpret_cast<char *>(&result));

  return result;
#else
  return value;
#endif  // MOTION_SDK_BIG_ENDIAN
}

}}} // namespace Motion::SDK::detail

#endif // __MOTION_SDK_DETAIL_ENDIAN_TO_NATIVE_HPP_

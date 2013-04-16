/**
  @file    tools/sdk/cpp/detail/endian_to_native.hpp
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.0

  (C) Copyright Motion Workshop 2013. All rights reserved.

  The coded instructions, statements, computer programs, and/or related
  material (collectively the "Data") in these files contain unpublished
  information proprietary to Motion Workshop which is protected by
  US federal copyright law and by international treaties.

  The Data may not be disclosed or distributed to third parties, in whole
  or in part, without the prior written consent of Motion Workshop.

  The Data is provided "as is" without express or implied warranty, and
  with no claim as to its suitability for any purpose.
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

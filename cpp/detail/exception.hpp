/**
  @file    tools/sdk/cpp/detail/exception.hpp
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
#ifndef __MOTION_SDK_DETAIL_EXCEPTION_HPP_
#define __MOTION_SDK_DETAIL_EXCEPTION_HPP_

// Enable exceptions by default.
#if !defined(MOTION_SDK_USE_EXCEPTIONS)
#  if defined(MOTION_SDK_NOTHROW)
#    define MOTION_SDK_USE_EXCEPTIONS 0
#  else 
#    define MOTION_SDK_USE_EXCEPTIONS 1
#  endif // MOTION_SDK_NOTHROW
#endif // MOTION_SDK_USE_EXCEPTIONS

#if MOTION_SDK_USE_EXCEPTIONS
#  include <stdexcept>

namespace Motion { namespace SDK { namespace detail {

typedef std::runtime_error error;

}}} // namespace Motion::SDK::detail

#endif //  MOTION_SDK_USE_EXCEPTIONS

#endif //  __MOTION_SDK_DETAIL_EXCEPTION_HPP_

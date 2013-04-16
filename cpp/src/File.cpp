/**
  Implementation of the File class. See the header file for more details.

  @file    tools/sdk/cpp/src/File.cpp
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

/*
  @file    tools/sdk/cpp/File.hpp
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
#ifndef __MOTION_SDK_FILE_HPP_
#define __MOTION_SDK_FILE_HPP_

#include <Format.hpp>
#include <detail/endian_to_native.hpp>

#include <fstream>
#include <string>
#include <vector>


namespace Motion { namespace SDK {

/**
  Implements a file input stream interface for reading Motion binary take data
  files. Provide a simple interface to develop external applications that can
  read Motion take data from disk.

  This class only handles the reading of binary data and conversion to arrays of
  native data types. The @ref Format class implements interfaces to the service
  specific data formats.

  @code
  try {
    using Motion::SDK::File;
    using Motion::SDK::Format;

    File file("sensor_data.bin");
    
    Format::SensorElement::data_type data(Format::SensorElement::Length);
    while (file.readData(data)) {
      // Access the data directly.
      std::copy(
        data.begin(), data.end(),
        std::ostream_iterator<float>(std::cout, " "));
      std::cout << std::endl;


      // Or wrap the data in the associated Format class.
      Format::SensorElement element(data);

      Format::SensorElement::data_type magnetometer = 
        element.getMagnetometer();
    }

  } catch (std::runtime_error & e) {
    std::cerr << e.what() << std::endl;
  }
  @endcode
*/
class File {
public:
  /**
    Open a Motion Take data file for reading.

    @param   pathname
    @pre     the input file exists and is readable
    @post    the input file is open and ready for reading
    @throws  std::runtime_error if the input file can not be opened
  */ 
  File(const std::string &pathname);

  /**
    Does not throw any exceptions.
  */ 
  virtual ~File();

  /**
    @throws  std::runtime_error if the file is not open or the
             close preocedure fails for any reason
  */
  virtual void close();

  /**
    Read a single block of binary data from the current position
    in the input file stream. Convert the block of data into an
    array of type <tt>T</tt> elements.

    @param   data is the output array of type <tt>T</tt> elements,
    <tt>data.size()</tt> specifies the number of elements to read
    for this single sample
    @return  <tt>true</tt> iff <tt>data.size()</tt> elements are read
    from the input stream and copied into <tt>data</tt>, otherwise
    returns false and <tt>data</tt> is filled with zeros
    @pre     type <tt>T</tt> is a primitive data type
    @throws  std::runtime_error if there is any error reading from
    the file stream, not including an EOF
  */
  template <typename T>
  bool readData(std::vector<T> &data)
  {
    bool result = false;

    if (!data.empty()) {

      if (m_input.good()) {
        // Read a block of data from the input stream directly into
        // the data buffer.
        const std::streamsize data_byte_size =
          static_cast<std::streamsize>(data.size() * sizeof(T));

        if (m_input.read(reinterpret_cast<char *>(&data[0]), data_byte_size)) {
          // Motion data is store in little-endian format. Transform it
          // to the native byte-order now.
          std::transform(
            data.begin(), data.end(),
            data.begin(),
            &detail::little_endian_to_native<T>);

          result = true;
        } else {
          // EOF, close the input stream and return false.
          close();
        }
      }

    }

    if (!result) {
      // Initialize the data buffer.
      std::fill(data.begin(), data.end(), T());
    }

    return result;
  }
private:
  /**
    Input file stream. Current input state. The #readData function simply
    reads data at the current position of this stream.
  */
  std::ifstream m_input;

  /**
    Disable the copy constructor. This is already enforced by the
    std::ifstream member, but let us be pedantic.

    This is a resource object. Copy constructor semantics would be confusing
    at the very least. Disable it instead.
  */
  File(const File &rhs);

  /**
    Disable the assignment operator.

    @see File#File(const File &)
  */
  const File &operator=(const File &lhs);
}; // class File

}} // namespace Motion::SDK

#endif // __MOTION_SDK_CLIENT_HPP_

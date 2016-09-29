/**
  Implementation of the File class. Read binary take file from the
  Motion Service take. Provide streaming interface to read one sample
  at a time.

  @file    tools/sdk/cs/File.cs
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.4

  Copyright (c) 2016, Motion Workshop
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
using System.IO;

namespace Motion {
  namespace SDK {
    /**
    Implements a file input stream interface for reading Motion Service
    binary take data files. Provide a simple interface to develop
    external applications that can read Motion take data from disk.

    This class only handles the reading of binary data and conversion
    to arrays of native data types. The {@link Format} class implements
    interfaces to the service specific data formats.

    Example usage:
    @code
    File file = new File("sensor_data.bin");

    while (true) {
      float[] data = file.readSensorData();
      if (null == data) {
        break;
      }
    }
    @endcode
    */
    public class File {
      public File(String pathname) {
        m_file = new FileStream(pathname, FileMode.Open, FileAccess.Read);
        m_in = new BinaryReader(m_file);
      }

      ~File() {
        try {
          close();
        } catch (Exception) {
        }
      }

      /**
         Close this file stream.
      */
      public void close() {
        if (null != m_in) {
          m_in.Close();
          m_in = null;
        }
        if (null != m_file) {
          m_file.Close();
          m_file = null;
        }
      }

      /**
       Convenience function to read a single sample
       from an output file. An output file is a
       stream of global quaterion values.
      */
      public float[] readOutputData() {
        return readFloatData(4);
      }

      /**
       Convenience function to read a single sample
       from a sensor file. An sensor file is a
       stream of calibrated accelerometer, magnetometer,
       and gyroscope readings in real units.
      */
      public float[] readSensorData() {
        return readFloatData(9);
      }

      /**
       Convenience function to read a single sample
       from a raw file. An raw file is a
       stream of accelerometer, magnetometer,
       and gyroscope readings directly from the
       sensors. Unprocessed, [0 4095].
      */
      public short[] readRawData() {
        return readShortData(9);
      }

      /**
         Read a sample of <code>N</code> single precision float values from the
         input file stream.

         @return a float array of <code>length</code> values from the input file
         stream, or <code>null</code> if the incoming data is invalid
      */
      public float[] readFloatData(int length) {
        float[] result = null;

        if ((length > 0) && (null != m_in)) {
          try {
            float[] buffer = new float[length];
            for (int i = 0; i < length; i++) {
              buffer[i] = m_in.ReadSingle();
            }

            result = buffer;
          } catch (EndOfStreamException) {
            close();
          }
        }

        return result;
      }

      /**
         Read a sample of <code>N</code> short integer values from the
         input file stream.

         @return a short array of <code>length</code> values from the input file
         stream, or <code>null</code> if the incoming data is invalid
      */
      public short[] readShortData(int length) {
        short[] result = null;

        if ((length > 0) && (null != m_in)) {
          try {
            short[] buffer = new short[length];
            for (int i = 0; i < length; i++) {
              buffer[i] = m_in.ReadInt16();
            }

            result = buffer;
          } catch (EndOfStreamException) {
            close();
          }
        }

        return result;
      }

      private FileStream m_file = null;
      private BinaryReader m_in = null;
    } // class File

  } // namespace SDK
} // namespace Motion

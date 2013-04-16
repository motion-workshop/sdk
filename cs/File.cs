/**
  Implementation of the File class. Read binary take file from the
  Motion Service take. Provide streaming interface to read one sample
  at a time.

  @file    tools/sdk/cs/File.cs
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
using System.IO;

namespace Motion
{
  namespace SDK
  {
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
    public class File
    {
      public File(String pathname)
      {
        m_file = new FileStream(pathname, FileMode.Open, FileAccess.Read);
        m_in = new BinaryReader(m_file);
      }

      ~File()
      {
        try
        {
          close();
        }
        catch (Exception)
        {
        }
      }

      /**
         Close this file stream.
      */
      public void close()
      {
        if (null != m_in)
        {
          m_in.Close();
          m_in = null;
        }
        if (null != m_file)
        {
          m_file.Close();
          m_file = null;
        }
      }

      /**
       Convenience function to read a single sample
       from an output file. An output file is a
       stream of global quaterion values.
      */
      public float[] readOutputData()
      {
        return readFloatData(4);
      }

      /**
       Convenience function to read a single sample
       from a sensor file. An sensor file is a
       stream of calibrated accelerometer, magnetometer,
       and gyroscope readings in real units.
      */
      public float[] readSensorData()
      {
        return readFloatData(9);
      }

      /**
       Convenience function to read a single sample
       from a raw file. An raw file is a
       stream of accelerometer, magnetometer,
       and gyroscope readings directly from the
       sensors. Unprocessed, [0 4095].
      */
      public short[] readRawData()
      {
        return readShortData(9);
      }

      /**
         Read a sample of <code>N</code> single precision float values from the
         input file stream.

         @return a float array of <code>length</code> values from the input file
         stream, or <code>null</code> if the incoming data is invalid
      */
      public float[] readFloatData(int length)
      {
        float[] result = null;

        if ((length > 0) && (null != m_in))
        {
          try
          {
            float[] buffer = new float[length];
            for (int i = 0; i < length; i++)
            {
              buffer[i] = m_in.ReadSingle();
            }

            result = buffer;
          }
          catch (EndOfStreamException)
          {
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
      public short[] readShortData(int length)
      {
        short[] result = null;

        if ((length > 0) && (null != m_in))
        {
          try
          {
            short[] buffer = new short[length];
            for (int i = 0; i < length; i++)
            {
              buffer[i] = m_in.ReadInt16();
            }

            result = buffer;
          }
          catch (EndOfStreamException)
          {
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

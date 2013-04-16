/**
   @file    tools/sdk/java/File.java
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
package Motion.SDK;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Vector;


/**
  Implements a file input stream interface for reading Motion Service
  binary take data files. Provide a simple interface to develop
  external applications that can read Motion take data from disk.

  <p>
  This class only handles the reading of binary data and conversion
  to arrays of native data types. The {@link Format} class implements
  interfaces to the service specific data formats.
  </p>

  <p>Example usage:</p>
  <pre>
  <code>
  File file = new File("sensor_data.bin");

  while (true) {
    Vector<Float> data = file.readFloatData(9);
    if (null == data) {
      break;
    }

    System.out.println("Incoming message, size = " + data.size());
  }
  </code>
  </pre>
*/
public class File
{
    /**
       Stream input from the file. No need for any higher
       level interface than since. Simply reading bytes.
    */
    private FileInputStream in = null;

    /**
       Open a Motion Service take data file stream for reading.

       @param pathname
       @see java.io.FileInputStream#FileInputStream(String)
    */
    public File(String pathname)
        throws IOException
    {
        in = new FileInputStream(pathname);
    }

    /**
       Close this connection.

       @see java.io.FileInputStream#close()
    */
    public void close()
        throws IOException
    {
        in.close();
    }

    /**
       Read a sample of <code>N</code> single precision float values from the
       input file stream.

       @return a {@link java.util.Vector}<Float> collection of <code>length</code>
       values from the input file stream, or <code>null</code> if the incoming
       data is invalid
    */
    public Vector<Float> readFloatData(int length)
        throws IOException
    {
        Vector<Float> result = null;
        {
            final int value_byte_size = Float.SIZE / Byte.SIZE;

            ByteBuffer buffer = readData(length * value_byte_size);
            if ((null != buffer) && (buffer.remaining() == length * value_byte_size))
            {
                result = new Vector<Float>();
                while (buffer.hasRemaining())
                {
                    result.add(new Float(buffer.getFloat()));
                }
            }
        }

        return result;
    }

    /**
       Read a sample of <code>N</code> short integer values from the input file
       stream.

       @return a {@link java.util.Vector}<Short> collection of <code>length</code>
       values from the input file stream, or <code>null</code> if the incoming
       data is invalid
    */
    public Vector<Short> readShortData(int length)
        throws IOException
    {
        Vector<Short> result = null;
        {
            final int value_byte_size = Short.SIZE / Byte.SIZE;

            ByteBuffer buffer = readData(length * value_byte_size);
            if ((null != buffer) && (buffer.remaining() == length * value_byte_size))
            {
                result = new Vector<Short>();
                while (buffer.hasRemaining())
                {
                    result.add(new Short(buffer.getShort()));
                }
            }
        }

        return result;
    }

    /**
       Read a sample of <code>N</code> raw bytes from the input file stream.
       
       @return a single sample of data, or <code>null</code> if the incoming
       data is invalid
    */
    private ByteBuffer readData(int length)
        throws IOException
    {
        ByteBuffer result = null;

        if (length > 0)
        {
            byte[] buffer = new byte[length];
            if (length == in.read(buffer))
            {
                // All service data is little-endian. Use the nifty
                // ByteBuffer to implement cross platform byte swapping.
                ByteBuffer nio_buffer = ByteBuffer.wrap(buffer);
                nio_buffer.order(ByteOrder.LITTLE_ENDIAN);

                result = nio_buffer;
            }
            else
            {
                close();
            }
        }

        return result;
    }

    /**
       Example usage and test function for the File class.
    */
    public static void main(String[] args)
        throws IOException
    {
        File file = new File("../../test_data/sensor.bin");

        while (true)
        {
            Vector<Float> data = file.readFloatData(9);
                    if (null == data)
            {
                        break;
            }

            System.out.println("Incoming message, size = " + data.size());
        }
    }
} // class File

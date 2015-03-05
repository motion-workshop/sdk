/**
  @file    tools/sdk/java/File.java
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

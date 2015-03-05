/**
  @file    tools/sdk/java/Format.java
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

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;


/**
   The Format class methods read a single binary message from a Motion Service
   stream and return an object representation of that message. This is layered
   (by default) on top of the {@link Client} class which handles the socket
   level binary message protocol.

   <p>
   Example usage:
   </p>
   <code>
   <pre>
   // Connect to port 32078 on localhost.
   Client client = new Client(null, 32078);
   while (true) {
     if (client.waitForData()) {
       while (true) {
         // Block on the open connection until a message comes in.
         {@link ByteBuffer} data = client.{@link Client#readData};
         if (null == data) {
           break;
         }

         // Create an associative container from the binary message.
         Map<Integer,Format.PreviewElement> preview = Format.Preview(data);
         // Iterate through the container.
         for (Map.Entry<Integer,Format.PreviewElement> entry: preview.entrySet()) {
            float[] euler = entry.getValue().getEuler();
            System.out.println("Euler = [" + euler[0] + "," + euler[1] + "," + euler[2] + "]");
         }
       }
     }
   }
   </pre>
   </code>
*/
public class Format {
    /**
       Motion Service streams send a list of data elements. The {@link Format}
       functions create a map from integer id to array packed data for each
       service specific format.

       This is an abstract base class to implement a single format specific
       data element. The idea is that a child class implements a format specific
       interface (API) to access individual components of an array of packed
       data.

       For example, the {@link PreviewElement} class extends this class
       and provides a {@link PreviewElement#getEuler} method to access
       an array of <tt>{x, y, z}</tt> Euler angles.
    */
    abstract public class Element<T extends Number> {
        /**
           Array of packed binary data for this element. If <tt>data</tt> is not
           <tt>null</tt> then it contains a sample for each of the <tt>N</tt>
           channels.
        */
        private Vector<T> data = null;

        /**
           Constructor is protected. Only allow child classes to call this.
          
           @param data array of packed data values for this Format::Element
           @param length valid length of the <tt>data</tt> array
        */
        protected Element(Vector<T> data, int length)
        {
            if ((data.size() == length) || (0 == length)) {
                this.data = data;
            }
        }

        /**
           Utility function to copy portions of the packed data array into its
           component elements. Only classes extending Element<Float> should
           call this method.

           @param base starting index to copy data from the internal data array
           @param length number of data values in this component element
           @return an array of <tt>element_length</tt> values, assigned to
           <tt>{m_data[i] ... m_data[i+element_length]}</tt> if there are valid
           values available or <code>null</code> otherwise
           @see #getShortData
        */
        protected float[] getFloatData(int base, int length)
        {
            float[] result = new float[length];
            if ((null != data) && (base + length <= data.size())) {
                for (int i=0; i<length; i++) {
                    // Generics do not work for built-in types. That forces
                    // us into the run-time type cast checking here.
                    result[i] = data.get(base + i).floatValue();
                }
            }

            return result;
        }

        /**
           Utility function to copy portions of the packed data array into its
           component elements.  Only classes extending Element<Short> should
           call this method.

           @param base starting index to copy data from the internal data array
           @param length number of data values in this component element
           @return an array of <tt>element_length</tt> values, assigned to
           <tt>{m_data[i] ... m_data[i+element_length]}</tt> if there are valid
           values available or <code>null</code> otherwise
           @see #getFloatData
        */
        protected short[] getShortData(int base, int length)
        {
            short[] result = new short[length];
            if ((null != data) && (base + length <= data.size())) {
                for (int i=0; i<length; i++) {
                    // Generics do not work for built-in types. That forces
                    // us into the run-time type cast checking here.
                    result[i] = data.get(base + i).shortValue();
                }
            }

            return result;
        }

        public Vector<T> access()
        {
            return data;
        }
    } // class Element


    /**
       The Configurable service provides access to all outputs of the Motion Service
       streams. The client selects the active channels as connect time. The service
       writes N sized elements of float values.
    */
    public class ConfigurableElement extends Element<Float> {
        /**
           Set the Length field to zero to indicate that this is
           a variable length element.
        */
        public static final int Length = 0;

        /**
        */
        public ConfigurableElement(Vector<Float> data)
        {
            super(data, Length);
        }

        /**
           Get a single channel entry at specified index.
        */
        public float value(int index)
        {
            return access().get(index);
        }

        /**
           Convenience method. Size accessor.
        */
        public int size()
        {
            return access().size();
        }

        /**
          
        */
        //float[] getRange(int base, int length) const
        //{
        //    return new float[1];
        //}

    } // class ConfigurableElement


    /**
       The Preview service provides access to the current filtered orientation
       output. The Preview service sends a map of <tt>N</tt> Preview data
       elements. Use this class to wrap a single Preview data element such
       that we can access individual components through a simple API.

       <p>
       Local orientation is defined relative to the start pose. The start pose
       is defined at take intitialization or by the user.
       </p>

       <p>
       Preview element format:
       <br/>
       <code>id => [global quaternion, local quaternion, local euler, local translation]</code>
       <br/>
       <code>id => {Gqw, Gqx, Gqy, Gqz, Lqw, Lqx, Lqy, Lqz, rx, ry, rz, tx, ty, tz}</code>
       </p>
    */
    public class PreviewElement extends Element<Float> {
        /**
           Total number of channels in the packed data array. 2 quaternions, 1 set
           of Euler angles, 1 set of translation channels.
        */
        public static final int Length = 2*4 + 2*3;

        /**
           Initialize this container identifier with a packed data
           array in the Preview format.

           @param data is a packed array of global quaternion, local
           quaternion, local Euler angle, and local translation channel
           data
        */
        public PreviewElement(Vector<Float> data)
        {
            super(data, Length);
        }

        /**
           Get a set of x, y, and z Euler angles that define the current
           local orientation. Specified in radians assuming <tt>x-y-z</tt>
           rotation order. Not necessarily continuous over time, each
           angle lies on the domain <tt>[-pi, pi]</tt>.

           <p>
           Euler angles are computed on the server side based on the
           current local quaternion orientation.
           </p>

           @return a three element array <tt>{x, y, z}</tt> of Euler angles
           in radians or <tt>null</tt> if there is no available data
           @see #getQuaternion
        */
        public float[] getEuler()
        {
            return getFloatData(8, 3);
        }
        
        /**
           Get a 4-by-4 rotation matrix from the current global or local quaternion
           orientation. Specified as a sixteen element array in row-major order.

           @param local set to true get the local orientation, otherwise returns the
           global orientation
           @return a sixteen element array that defines a 4-by-4 transformation
           matrix in row-major order or the idenitity if there is no available data
           @see #getQuaternion
        */
        public float[] getMatrix(boolean local)
        {
            return quaternion_to_R3_rotation(getQuaternion(local));
        }

        /**
           Get the global or local unit quaternion that defines the current
           orientation.

           @param local set to true get the local orientation, otherwise returns the
           global orientation
           @return a four element array <code>{w, x, y, z}</code> that defines a
           unit length quaternion <code>q = w + x*i + y*j + z*k</code> or zeros
           if there is no available data
        */
        public float[] getQuaternion(boolean local)
        {
            if (local) {
            return getFloatData(4, 4);
            } else {
            return getFloatData(0, 4);
            }
        }

        /**
           Get x, y, and z of the current estimate of linear acceleration.
           Specified in g.

           @return a three element array <code>{x, y, z}</code> of linear
           acceleration channels specified in g or zeros if there is no
           available data
        */
        public float[] getAccelerate()
        {
            return getFloatData(11, 3);
        }
    } // class PreviewElement


    /**
       The Sensor service provides access to the current un-filtered sensor signals
       in real units. The Sensor service sends a map of <tt>N</tt> data elements.
       Use this class to wrap a single Sensor data element such that we can access
       individual components through a simple API.

       Sensor element format:
       id => [accelerometer, magnetometer, gyroscope]
       id => {ax, ay, az, mx, my, mz, gx, gy, gz}
    */
    public class SensorElement extends Element<Float> {
        public final static int Length = 3*3;

        /**
           Initialize this container identifier with a packed data
           array in the Sensor format.

           @param data is a packed array of accelerometer, magnetometer,
           and gyroscope un-filtered signal data.
        */
        public SensorElement(Vector<Float> data)
        {
            super(data, Length);
        }

        /**
           Get a set of x, y, and z values of the current un-filtered
           accelerometer signal. Specified in <em>g</em> where 1 <em>g</em> 
           = <code>-9.8 meters/sec^2</code>.

           Domain varies with configuration. Maximum is <code>[-6, 6]</code>
           <em>g</em>.

           @return a three element array <code>{x, y, z}</code> of acceleration
           in <em>g</em>s or zeros if there is no available data
        */
        public float[] getAccelerometer()
        {
            return getFloatData(0, 3);
        }

        /**
           Get a set of x, y, and z values of the current un-filtered
           gyroscope signal. Specified in <tt>degrees/second</tt>.

           Valid domain is <tt>[-500, 500] degress/second</tt>.

           @return a three element array <code>{x, y, z}</code> of angular velocity
           in <code>degrees/second</code> or zeros if there is no available data
        */
        public float[] getGyroscope()
        {
            return getFloatData(6, 3);
        }
      
        /**
           Get a set of x, y, and z values of the current un-filtered
           magnetometer signal. Specified in <code>uT</code> (microtesla).
           
           Domain varies with local magnetic field strength. Expect values
           on <code>[-60, 60]</code> <code>uT</code> (microtesla).
          
           @return a three element array <code>{x, y, z}</code> of magnetic field
           strength in <code>uT</code> (microtesla) or zeros if there is no
           available data
        */
        public float[] getMagnetometer()
        {
            return getFloatData(3, 3);
        }
    } // class SensorElement


    /**
       The Raw service provides access to the current uncalibrated, unprocessed
       sensor signals in signed integer format. The Raw service sends a map of
       <tt>N</tt> data elements. Use this class to wrap a single Raw data element
       such that we can access individual components through a simple API.

       <p>
       Raw element format:
       <br/>
       <code>id => [accelerometer, magnetometer, gyroscope]</code>
       </br>
       <code>id => {ax, ay, az, mx, my, mz, gx, gy, gz}</code>
       </p>

       <p>
       All sensors output 12-bit integers. Process as 16-bit short integers on
       the server side.
       </p>
    */
    public class RawElement extends Element<Short> {
        public final static int Length = 3*3;

        /**
           Initialize this container identifier with a packed data
           array in the Raw format.

           @param data is a packed array of accelerometer, magnetometer,
           and gyroscope unprocessed signal data
        */
        public RawElement(Vector<Short> data)
        {
            super(data, Length);
        }

        /**
           Get a set of x, y, and z values of the current unprocessed
           accelerometer signal.

           <p>
           Valid domain is <code>[0, 4095]</code>.
           </p>

           @return a three element array <code>{x, y, z}</code> of raw
           accelerometer output or zeros if there is no available data
        */
        public short[] getAccelerometer()
        {
            return getShortData(0, 3);
        }

        /**
           Get a set of x, y, and z values of the current unprocessed
           gyroscope signal.

           <p>
           Valid domain is <code>[0, 4095]</code>.
           </p>

           @return a three element array <code>{x, y, z}</code> of raw
           gyroscope output or zeros if there is no available data
        */
        public short[] getGyroscope()
        {
            return getShortData(6, 3);
        }
      
        /**
           Get a set of x, y, and z values of the current unprocessed
           magnetometer signal.

           <p>
           Valid domain is <code>[0, 4095]</code>.
           </p>

           @return a three element array <tt>{x, y, z}</tt> of raw
           magnetometer output or zeros if there is no available data
        */
        public short[] getMagnetometer()
        {
            return getShortData(3, 3);
        }
    } // class RawElement


    /**
       Create a map of <code>N</code> Configurable data elements from a packed binary
       message. Each of the <code>N</code> elements has an integer id and an array of
       <code>M</code> floating point values.

       @param buffer raw binary input message, for example directly from
       a call to {@link Client#readData}
       @return a {@link java.util.Map}<Integer,Format.ConfigurableElement> collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    public static Map<Integer,ConfigurableElement> Configurable(ByteBuffer buffer)
        throws BufferUnderflowException
    {
        Map<Integer,Format.ConfigurableElement> result = new TreeMap<Integer,Format.ConfigurableElement>();
        {
            // Use this to do most of the dirty work.
            Map<Integer,Vector<Float> > map = IdToFloatArray(buffer, ConfigurableElement.Length);

            if (!map.isEmpty()) {
                // Need this to instance the PreviewElement class.
                final Format format = new Format();

                for (Map.Entry<Integer,Vector<Float> > entry: map.entrySet()) {
                    result.put(entry.getKey(),
                           format.new ConfigurableElement(entry.getValue()));
                }

                if (map.size() != result.size()) {
                    result.clear();
                }
            }
        }

        return result;
    }


    /**
       Create a map of <code>N</code> Preview data elements from a packed binary input
       message. Each of the <code>N</code> elements has an integer id and an array of
       <code>M</code> floating point values.

       @param buffer raw binary input message, for example directly from
       a call to {@link Client#readData}
       @return a {@link java.util.Map}<Integer,Format.PreviewElement> collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    public static Map<Integer,PreviewElement> Preview(ByteBuffer buffer)
        throws BufferUnderflowException
    {
        Map<Integer,Format.PreviewElement> result = new TreeMap<Integer,Format.PreviewElement>();
        {
            // Use this to do most of the dirty work.
            Map<Integer,Vector<Float> > map = IdToFloatArray(buffer, PreviewElement.Length);

            if (!map.isEmpty()) {
                // Need this to instance the PreviewElement class.
                final Format format = new Format();

                for (Map.Entry<Integer,Vector<Float> > entry: map.entrySet()) {    
                    result.put(entry.getKey(),
                           format.new PreviewElement(entry.getValue()));
                }

                if (map.size() != result.size()) {
                    result.clear();
                }
            }
        }
    
        return result;
    }

    /**
       Create a map of <code>N</code> Sensor data elements from a packed binary input
       message. Each of the <code>N</code> elements has an integer id and an array of
       <code>M</code> floating point values.

       @param buffer raw binary input message, for example directly from
       a call to {@link Client#readData}
       @return a {@link java.util.Map}<Integer,Format.SensorElement> collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    public static Map<Integer,SensorElement> Sensor(ByteBuffer buffer)
        throws BufferUnderflowException
    {
        Map<Integer,SensorElement> result = new TreeMap<Integer,SensorElement>();
        {
            // Use this to do most of the dirty work.
            Map<Integer,Vector<Float> > map = IdToFloatArray(buffer, SensorElement.Length);

            if (!map.isEmpty()) {
                // Need this to instance the SensorElement class.
                final Format format = new Format();

                for (Map.Entry<Integer,Vector<Float> > entry: map.entrySet()) {    
                    result.put(entry.getKey(),
                           format.new SensorElement(entry.getValue()));
                }

                if (map.size() != result.size()) {
                    result.clear();
                }
            }
        }
        
        return result;
    }

    /**
       Create a map of <code>N</code> Sensor data elements from a packed binary input
       message. Each of the <code>N</code> elements has an integer id and an array of
       <code>M</code> floating point values.

       @param buffer raw binary input message, for example directly from
       a call to {@link Client#readData}
       @return a {@link java.util.Map}<Integer,Format.RawElement> collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    public static Map<Integer,RawElement> Raw(ByteBuffer buffer)
        throws BufferUnderflowException
    {
        Map<Integer,RawElement> result = new TreeMap<Integer,RawElement>();
        {
            // Use this to do most of the dirty work.
            Map<Integer,Vector<Short> > map = IdToShortArray(buffer, RawElement.Length);

            if (!map.isEmpty()) {
                // Need this to instance the SensorElement class.
                final Format format = new Format();

                for (Map.Entry<Integer,Vector<Short> > entry: map.entrySet()) {    
                    result.put(entry.getKey(),
                           format.new RawElement(entry.getValue()));
                }

                if (map.size() != result.size()) {
                    result.clear();
                }
            }
        }
    
        return result;
    }


    /**
       Create a map of <code>N</code> elements from a packed byte array. Each
       element has an id and a array of <code>M</code> elements. The incoming
       byte buffer must have exactly the number of bytes to complete <code>N</code>
       elements. <code>N</code> is computed based on the size of the input buffer
       and the length of each float array.

       @param buffer raw binary message input, for example directly from
       a call to {@link Client#readData}
       @param length the number of values in a single element's float array
       @return a {@link java.util.Map}<Integer,Vector<Float> > collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    private static Map<Integer,Vector<Float> > IdToFloatArray(ByteBuffer buffer, int length)
        throws BufferUnderflowException
    {
        // Use the sorted TreeMap, behaves like the C++ std::map.
        Map<Integer,Vector<Float> > result = new TreeMap<Integer,Vector<Float> >();

        while (buffer.remaining() > (Integer.SIZE / Byte.SIZE))
        {
            Integer key = new Integer(buffer.getInt());
            Vector<Float> value = new Vector<Float>();

            int element_length = length;
            if ((0 == element_length) && (buffer.remaining() > (Integer.SIZE / Byte.SIZE))) {
                element_length = buffer.getInt();
            }

            if ((element_length > 0) && (buffer.remaining() >= (element_length * (Float.SIZE / Byte.SIZE)))) {
                for (int i=0; i<element_length; i++) {
                    value.add(new Float(buffer.getFloat()));
                }

                result.put(key, value);
            }
        }

        // Fail if there are unused bytes remaining in the incoming buffer.
        if (buffer.hasRemaining()) {
            result.clear();
        }

        return result;
    }

    /**
       Create a map of <code>N</code> short integer elements from a packed
       byte array. Each element has an id and a array of <code>M</code> elements.
       The incoming byte buffer must have exactly the number of bytes to complete
       <code>N</code> elements. <code>N</code> is computed on the fly based on the
       size of the input buffer and the length of each short integer array.

       @param buffer raw binary message input, for example directly from
       a call to {@link Client#getSample}
       @param length the number of values in a single element's short
       integer array
       @return a {@link java.util.Map}<Integer,Vector<short> > collection
       representation of the input message
       @throws BufferUnderflowException if there are not enough bytes in
       the incoming buffer to complete the output object
    */
    private static Map<Integer,Vector<Short> > IdToShortArray(ByteBuffer buffer,
                                                              int length)
        throws BufferUnderflowException
    {
        // Use the sorted TreeMap, behaves like the C++ std::map.
        Map<Integer,Vector<Short> > result = new TreeMap<Integer,Vector<Short> >();

        // Compute the size in bytes of a single element. Conside the input
        // buffer to be a packed set of element entries.
        final int element_size =
            (Integer.SIZE / Byte.SIZE) + (Short.SIZE / Byte.SIZE) * length;

        // while we have enough bytes to create a complete element. 
        while (element_size <= buffer.remaining()) {
            Integer key = new Integer(buffer.getInt());
            Vector<Short> value = new Vector<Short>();
            for (int i=0; i<length; i++) {
            value.add(new Short(buffer.getShort())); 
            }

            result.put(key, value);
        }

        // Fail if there are unused bytes remaining in the incoming buffer.
        if (buffer.hasRemaining()) {
            result.clear();
        }

        return result;
    }


    /**
       Ported from the Boost.Quaternion library at:
       http://www.boost.org/libs/math/quaternion/HSO3.hpp

       @param q defines a quaternion in the format [w x y z] where
       <tt>q = w + x*i + y*j + z*k = (w, x, y, z)</tt>
       @return an array of 16 elements in row-major order that defines
       a 4-by-4 rotation matrix computed from the input quaternion or
       the identity matrix if the input quaternion has zero length
    */
    private float[] quaternion_to_R3_rotation(float[] q)
    {
        if ((null == q) || (4 != q.length)) {
            return null;
        }

        final float a = q[0];
        final float b = q[1];
        final float c = q[2];
        final float d = q[3];

        final float aa = a*a;
        final float ab = a*b;
        final float ac = a*c;
        final float ad = a*d;
        final float bb = b*b;
        final float bc = b*c;
        final float bd = b*d;
        final float cc = c*c;
        final float cd = c*d;
        final float dd = d*d;

        final float norme_carre = aa+bb+cc+dd;

        // Defaults to the identity matrix.
        float[] result = new float[16];
        for (int i=0; i<4; i++) {
            result[4*i+i] = 1f;
        }
        
        if (norme_carre > 1e-6) {
            result[0] = (aa+bb-cc-dd)/norme_carre;
            result[1] = 2*(-ad+bc)/norme_carre;
            result[2] = 2*(ac+bd)/norme_carre;
            result[4] = 2*(ad+bc)/norme_carre;
            result[5] = (aa-bb+cc-dd)/norme_carre;
            result[6] = 2*(-ab+cd)/norme_carre;
            result[8] = 2*(-ac+bd)/norme_carre;
            result[9] = 2*(ab+cd)/norme_carre;
            result[10] = (aa-bb-cc+dd)/norme_carre;
        }

        return result;
    }

    /** Hide the constructor. There is no need to instantiate the Format object. */
    private Format()
    {
    }


    /**
       Example usage and test function for the Format class.
    */
    public static void main(String [] args)
    {
        ByteBuffer preview_data = ByteBuffer.allocate(
            (Integer.SIZE / Byte.SIZE) +
            (Float.SIZE / Byte.SIZE) * PreviewElement.Length);
        {
            preview_data.putInt(1);
            for (int i=0; i<PreviewElement.Length; i++) {
                preview_data.putFloat(i);
            }
            preview_data.rewind();
        }

        Map preview = Format.Preview(preview_data);
        System.out.println("Preview (" + preview.size() + ")");
        //for (Map.Entry<Integer,Format.PreviewElement> entry: preview.entrySet()) {
        //    System.out.println("[" + entry.getKey() + "] => " + entry.getValue().getEuler()[0]);
        //}
    }
} // class Format

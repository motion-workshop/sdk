/**
  Implementation of the Format class. Convert binary messages into
  associative containers of [id] => <data1, ... dataN>.

  @file    tools/sdk/cs/Format.cs
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
using System;
using System.Collections.Generic;

namespace Motion {
  namespace SDK {
    /**
    The Format class methods read a single binary message from a Motion Service
    stream and returns an object representation of that message. This is
    layered (by default) on top of the @ref Client class which handles the
    socket level binary message protocol.

    Example usage, extends the @ref Client class example:
    @code
    try {
      // Connect to the Preview data stream.
      Client client = new Client("", 32079);

      Client::data_type data;
      while (client.readData(data)) {
        // Create an object representation of the current binary message.
        Format::preview_type preview =
          Format::Preview(data.begin(), data.end());
       
        // Iterate through the list of [id] => PreviewElement objects.
        for (Format::preview_type::iterator itr=preview.begin(); itr!=preview.end(); ++itr) {
          // Use the PreviewElement interface to access format specific data.
          const PreviewElement & element = itr->second;
        }
      }

    } catch (Exception e) {
      // The Client and Format class may throw exceptions for
      // any unrecoverable conditions.
    }
    @endcode
    */ 
    public static class Format {
      /**
        Motion Service streams send a list of data elements. The @ref Format
        functions create a map from integer id to array packed data for each
        service specific format.

        This is an abstract base class to implement a single format specific
        data element. The idea is that a child class implements a format specific
        interface (API) to access individual components of an array of packed
        data. The template parameter defines the type of the packed data
        elements.

        For example, the @ref PreviewElement class extends this class
        and provides a @ref PreviewElement#getEuler method to access
        an array of <tt>{x, y, z}</tt> Euler angles.
      */
      public abstract class Element<T> {
        /**
          Constructor is protected. Only allow child classes to call this.

          @param data array of packed data values for this Format::Element
          @param length valid length of the <tt>data</tt> array
          @pre <tt>data.Length == length</tt>
        */
        protected Element(T[] data, int length) {
          if ((data.Length == length) || (0 == length)) {
            m_data = data;
          }
        }

        /**
          Utility function to copy portions of the packed data array into its
          component elements.

          @param start starting index to copy data from the internal data array
          @param length number of data values in this component element
          @pre <tt>base + length <= m_data.Length</tt>
          @return an array of <tt>length</tt> values, assigned to
          <tt>{m_data[i] ... m_data[i+element_length]}</tt> if there are valid
          values available or zeros otherwise
        */
        protected T[] getData(int start, int length) {
          T[] result = new T[length];
          if ((null != m_data) && (start + length <= m_data.Length)) {
            System.Array.Copy(m_data, start, result, 0, length);
          }
          return result;
        }

        protected T[] m_data = null;

        public T[] access() {
          return m_data;
        }
      } // class Element

      /**
        The Configurable data services provides access to all data streams in
        a single message. The client selects channels and ordering at the
        start of the connection. The Configurable service sends a map of
        <tt>N</tt> data elements. Each data element is an array of <tt>M</tt>
        single precision floating point numbers.
      */
      public class ConfigurableElement : Element<float> {
        public const int Length = 0;

        /**
           Initialize this container identifier with a packed data
           array in the Sensor format.

           @param data is a packed array of accelerometer, magnetometer,
           and gyroscope un-filtered signal data.
        */
        public ConfigurableElement(float[] data)
          : base(data, Length)
        {
        }

        /**
          Get a single channel entry at specified index.
        */
        public float value(int index) {
          return access()[index];
        }

        /**
          Convenience method. Size accessor.
        */
        public int size() {
          return access().Length;
        }

        /**
          Range accessor. In case the client application
          wants to slice up the data into logical parts.
        */
        public float[] getRange(int start, int length) {
          return getData(start, length);
        }
      } // class ConfigurableElement

      /**
        The Preview service provides access to the current orientation output as
        a quaternion, a set of Euler angles, or a 4-by-4 rotation matrix. The Preview
        service sends a map of <tt>N</tt> Preview data elements. Use this class to wrap
        a single Preview data element such that we can access individual components
        through a simple API.

        Preview element format:
        id => [global quaternion, local quaternion, local euler, global acceleration]
        id => {Gqw, Gqx, Gqy, Gqz, Lqw, Lqx, Lqy, Lqz, rx, ry, rz, ax, ay, az}
      */
      public class PreviewElement : Element<float> {
        /**
           Total number of channels in the packed data array. 2 quaternions, 1 set
           of Euler angles, 1 set of translation channels.
        */
        public const int Length = 4 * 2 + 3 * 2;

        /**
           Initialize this container identifier with a packed data
           array in the Preview format.

           @param data is a packed array of global quaternion, local
           quaternion, local Euler angle, and local translation channel
           data
        */
        public PreviewElement(float[] data)
          : base(data, Length)
        {
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
        public float[] getEuler() {
          return getData(8, 3);
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
        public float[] getMatrix(bool local) {
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
        public float[] getQuaternion(bool local) {
          if (local) {
            return getData(4, 4);
          } else {
            return getData(0, 4);
          }
        }

        /**
           Get x, y, and z of the current estimate of linear acceleration.
           Specified in g.

           @return a three element array <code>{x, y, z}</code> of linear
           acceleration channels specified in g or zeros if there is no
           available data
        */
        public float[] getAccelerate() {
          return getData(11, 3);
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
      public class SensorElement : Element<float> {
        public const int Length = 3 * 3;

        /**
           Initialize this container identifier with a packed data
           array in the Sensor format.

           @param data is a packed array of accelerometer, magnetometer,
           and gyroscope un-filtered signal data.
        */
        public SensorElement(float[] data)
          : base(data, Length)
        {
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
        public float[] getAccelerometer() {
          return getData(0, 3);
        }

        /**
           Get a set of x, y, and z values of the current un-filtered
           gyroscope signal. Specified in <tt>degrees/second</tt>.

           Valid domain is <tt>[-500, 500] degress/second</tt>.

           @return a three element array <code>{x, y, z}</code> of angular velocity
           in <code>degrees/second</code> or zeros if there is no available data
        */
        public float[] getGyroscope() {
          return getData(6, 3);
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
        public float[] getMagnetometer() {
          return getData(3, 3);
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
      public class RawElement : Element<short> {
        public const int Length = 3 * 3;

        /**
           Initialize this container identifier with a packed data
           array in the Raw format.

           @param data is a packed array of accelerometer, magnetometer,
           and gyroscope unprocessed signal data
        */
        public RawElement(short[] data)
          : base(data, Length)
        {
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
        public short[] getAccelerometer() {
          return getData(0, 3);
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
        public short[] getGyroscope() {
          return getData(6, 3);
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
        public short[] getMagnetometer() {
          return getData(3, 3);
        }
      } // class RawElement


      public static IDictionary<int, ConfigurableElement> Configurable(byte[] data) {
        IDictionary<int, ConfigurableElement> result = null;
        {
          // Use this to do most of the dirty work.
          IDictionary<int, float[]> map = IdToFloatArray(data, ConfigurableElement.Length);
          if (map.Count > 0) {
            result = new Dictionary<int, ConfigurableElement>();
            foreach (KeyValuePair<int, float[]> itr in map) {
              result.Add(itr.Key, new ConfigurableElement(itr.Value));
            }

            if (map.Count != result.Count) {
              result.Clear();
            }
            if (result.Count <= 0) {
              result = null;
            }
          }
        }

        return result;
      }

      public static IDictionary<int, PreviewElement> Preview(byte[] data) {
        IDictionary<int, PreviewElement> result = null;
        {
          // Use this to do most of the dirty work.
          IDictionary<int, float[]> map = IdToFloatArray(data, PreviewElement.Length);
          if (map.Count > 0) {
            result = new Dictionary<int, PreviewElement>();
            foreach (KeyValuePair<int, float[]> itr in map) {
              result.Add(itr.Key, new PreviewElement(itr.Value));
            }

            if (map.Count != result.Count) {
              result.Clear();
            }
            if (result.Count <= 0) {
              result = null;
            }
          }
        }

        return result;
      }

      public static IDictionary<int, SensorElement> Sensor(byte[] data) {
        IDictionary<int, SensorElement> result = null;
        {
          // Use this to do most of the dirty work.
          IDictionary<int, float[]> map = IdToFloatArray(data, SensorElement.Length);
          if (map.Count > 0) {
            result = new Dictionary<int, SensorElement>();
            foreach (KeyValuePair<int, float[]> itr in map) {
              result.Add(itr.Key, new SensorElement(itr.Value));
            }

            if (map.Count != result.Count) {
              result.Clear();
            }
            if (result.Count <= 0) {
              result = null;
            }
          }
        }

        return result;
      }

      public static IDictionary<int, RawElement> Raw(byte[] data) {
        IDictionary<int, RawElement> result = null;
        {
          // Use this to do most of the dirty work.
          IDictionary<int, short[]> map = IdToShortArray(data, RawElement.Length);
          if (map.Count > 0) {
            result = new Dictionary<int, RawElement>();
            foreach (KeyValuePair<int, short[]> itr in map) {
              result.Add(itr.Key, new RawElement(itr.Value));
            }

            if (map.Count != result.Count) {
              result.Clear();
            }
            if (result.Count <= 0) {
              result = null;
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
         @return a IDictionary<int, List<float>> collection
         representation of the input message
      */ 
      private static IDictionary<int, float[]> IdToFloatArray(byte[] buffer, int length) {
        // Use the Dictionary interface, behaves like the C++ std::map.
        IDictionary<int, float[]> result = new Dictionary<int, float[]>();

        // Loop while we have enough bytes to create a complete element.
        int itr = 0;
        while ((itr < buffer.Length) && ((buffer.Length - itr) > sizeof(int))) {
          // Read the integer id for this element.
          int key = BitConverter.ToInt32(buffer, itr);
          itr += sizeof(int);

          int element_length = length;
          if ((0 == element_length) && ((buffer.Length - itr) > sizeof(int))) {
            element_length = BitConverter.ToInt32(buffer, itr);
            itr += sizeof(int);
          }

          if ((element_length > 0) && ((buffer.Length - itr) >= element_length*sizeof(float))) {
            // Read the array of values for this element.
            float[] value = new float[element_length];
            for (int i = 0; i < element_length; i++) {
              value[i] = BitConverter.ToSingle(buffer, itr);
              itr += sizeof(float);
            }

            // Add this element to the associative container
            // of [key] => <data> pairs.
            result.Add(key, value);
          }
        }

        // If we did not consume all of the input bytes this is an
        // invalid message.
        if (itr != buffer.Length) {
          result.Clear();
        }
        // If there are no elements in the container, set it to null
        // to indicate an invalid message.
        if (0 == result.Count) {
          result = null;
        }

        return result;
      }

      /**
        @ref see Format#IdToFloatArray
      */
      private static IDictionary<int, short[]> IdToShortArray(byte[] buffer, int length) {
        // Use the Dictionary interface, behaves like the C++ std::map.
        IDictionary<int, short[]> result = new Dictionary<int, short[]>();

        // Compute the size in bytes of a single element. Conside the input
        // buffer to be a packed set of element entries.
        int element_size = sizeof(int) + sizeof(short) * length;

        int itr = 0;
        while ((itr < buffer.Length) && (element_size <= (buffer.Length - itr))) {
          // Read the integer id for this element.
          int key = BitConverter.ToInt32(buffer, itr);
          itr += sizeof(int);

          // Read the array of values for this element.
          short[] value = new short[length];
          for (int i = 0; i < length; i++) {
            value[i] = BitConverter.ToInt16(buffer, itr);
            itr += sizeof(short);
          }

          // Add this element to the associative container
          // of [key] => <data> pairs.
          result.Add(key, value);
        }

        // If we did not consume all of the input bytes this is an
        // invalid message.
        if (itr != buffer.Length) {
          result.Clear();
        }
        // If there are no elements in the container, set it to null
        // to indicate an invalid message.
        if (0 == result.Count) {
          result = null;
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
      private static float[] quaternion_to_R3_rotation(float[] q) {
        if ((null == q) || (4 != q.Length)) {
          return null;
        }

        float a = q[0];
        float b = q[1];
        float c = q[2];
        float d = q[3];

        float aa = a * a;
        float ab = a * b;
        float ac = a * c;
        float ad = a * d;
        float bb = b * b;
        float bc = b * c;
        float bd = b * d;
        float cc = c * c;
        float cd = c * d;
        float dd = d * d;

        float norme_carre = aa + bb + cc + dd;

        // Defaults to the identity matrix.
        float[] result = new float[16];
        for (int i = 0; i < 4; i++) {
          result[4 * i + i] = 1f;
        }

        if (norme_carre > 1e-6) {
          result[0] = (aa + bb - cc - dd) / norme_carre;
          result[1] = 2 * (-ad + bc) / norme_carre;
          result[2] = 2 * (ac + bd) / norme_carre;
          result[4] = 2 * (ad + bc) / norme_carre;
          result[5] = (aa - bb + cc - dd) / norme_carre;
          result[6] = 2 * (-ab + cd) / norme_carre;
          result[8] = 2 * (-ac + bd) / norme_carre;
          result[9] = 2 * (ab + cd) / norme_carre;
          result[10] = (aa - bb - cc + dd) / norme_carre;
        }

        return result;
      }
    } // class Format

  } // namespace SDK
} // namespace Motion

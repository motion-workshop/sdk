/**
  Implementation of the Format class. See the header file for more details.

  @file    tools/sdk/cpp/src/Format.cpp
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
#include <Format.hpp>

#include <limits>

#include <detail/exception.hpp>


namespace Motion { namespace SDK {

template<typename Quaternion>
Format::data_type quaternion_to_R3_rotation(const Quaternion &q);

#if defined(__GNUC__)
const std::size_t Format::PreviewElement::Length;
const std::size_t Format::SensorElement::Length;
const std::size_t Format::RawElement::Length;
const std::size_t Format::ConfigurableElement::Length;
#endif  // __GNUC__

std::string Format::ConfigurableElement::Name("Configurable");
std::string Format::PreviewElement::Name("Preview");
std::string Format::SensorElement::Name("Sensor");
std::string Format::RawElement::Name("Raw");


Format::ConfigurableElement::ConfigurableElement(const data_type &data)
  : Element<data_type::value_type>(data, Length)
{
}
const Format::ConfigurableElement::value_type &
Format::ConfigurableElement::operator[](const size_type &index) const
{
  return access()[index];
}

Format::size_type Format::ConfigurableElement::size() const
{
  return access().size();
}

Format::ConfigurableElement::data_type
Format::ConfigurableElement::getRange(const size_type &base,
                                      const size_type &length) const
{
  return getData(base, length);
}


Format::PreviewElement::PreviewElement(const data_type &data)
  : Element<data_type::value_type>(data, Length)
{
}

Format::PreviewElement::data_type Format::PreviewElement::getEuler() const
{
  return getData(8, 3);
}

Format::PreviewElement::data_type
Format::PreviewElement::getMatrix(bool local) const
{
  return quaternion_to_R3_rotation(getQuaternion(local));
}

Format::PreviewElement::data_type
Format::PreviewElement::getQuaternion(bool local) const
{
  if (local) {
    return getData(4, 4);
  } else {
    return getData(0, 4);
  }
}

Format::PreviewElement::data_type
Format::PreviewElement::getAccelerate() const
{
  return getData(11, 3);
}


Format::SensorElement::SensorElement(const data_type &data)
  : Element<data_type::value_type>(data, Length)
{
}

Format::SensorElement::data_type
Format::SensorElement::getAccelerometer() const
{
  return getData(0, 3);
}

Format::SensorElement::data_type
Format::SensorElement::getGyroscope() const
{
  return getData(6, 3);
}

Format::SensorElement::data_type
Format::SensorElement::getMagnetometer() const
{
  return getData(3, 3);
}


Format::RawElement::RawElement(const data_type &data)
  : Element<data_type::value_type>(data, Length)
{
}

Format::RawElement::data_type
Format::RawElement::getAccelerometer() const
{
  return getData(0, 3);
}

Format::RawElement::data_type
Format::RawElement::getGyroscope() const
{
  return getData(6, 3);
}

Format::RawElement::data_type
Format::RawElement::getMagnetometer() const
{
  return getData(3, 3);
}


/**
  Ported from the Boost.Quaternion library at:
  http://www.boost.org/libs/math/quaternion/HSO3.hpp

  @param q defines a quaternion in the format [w x y z] where
  <tt>q = w + x*i + y*j + z*k = (w, x, y, z)</tt>
  @return an array of 16 elements that defines a 4-by-4 rotation
  matrix computed from the input quaternion or identity matrix if
  the input quaternion has zero length
*/
template<typename Quaternion>
Format::PreviewElement::data_type quaternion_to_R3_rotation(const Quaternion &q)
{
  typedef Format::data_type::value_type real_type;

  // Initialize the result matrix to the identity.
  Format::PreviewElement::data_type result(16);
  {
    result[0] = result[5] = result[10] = result[15] = 1;
  }

  if (4 != q.size()) {
    return result;
  }

	const real_type a = q[0];
	const real_type b = q[1];
	const real_type c = q[2];
	const real_type d = q[3];

	const real_type aa = a*a;
	const real_type ab = a*b;
	const real_type ac = a*c;
	const real_type ad = a*d;
	const real_type bb = b*b;
	const real_type bc = b*c;
	const real_type bd = b*d;
	const real_type cc = c*c;
	const real_type cd = c*d;
	const real_type dd = d*d;

	const real_type norme_carre = aa+bb+cc+dd;

  if (norme_carre > 1e-6) {
    result[0] = (aa+bb-cc-dd)/norme_carre;
    result[1] = 2*(-ad+bc)/norme_carre;
    result[2] = 2*(ac+bd)/norme_carre;
    result[4] = 2*(ad+bc)/norme_carre;
    result[5] = (aa-bb+cc-dd)/norme_carre;
    result[6] = 2*(-ab+cd)/norme_carre;
    result[7] = 0;
    result[8] = 2*(-ac+bd)/norme_carre;
    result[9] = 2*(ab+cd)/norme_carre;
    result[10] = (aa-bb-cc+dd)/norme_carre;
  }

	return result;
}

}} // namespace Motion::SDK

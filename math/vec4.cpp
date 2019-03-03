#include "vec4.h"

namespace Math
{

Vec4::Vec4(float a, float b, float c, float d)
{
	x = a;
	y = b;
	z = c;
	w = d;
}

float& Vec4::operator [](int i)
{
	return ((&x)[i]);
}

const float& Vec4::operator [](int i) const
{
	return ((&x)[i]);
}

Vec4& Vec4::operator *=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return (*this);
}

Vec4& Vec4::operator /=(float s)
{
	s = 1.0f / s;

	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return (*this);
}

Vec4& Vec4::operator +=(const Vec4& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return (*this);
}

Vec4& Vec4::operator -=(const Vec4& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return (*this);
}

} // namespace Math

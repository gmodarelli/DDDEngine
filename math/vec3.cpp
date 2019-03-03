#include "vec3.h"

namespace Math
{

Vec3::Vec3(float a, float b, float c)
{
	x = a;
	y = b;
	z = c;
}

float& Vec3::operator [](int i)
{
	return ((&x)[i]);
}

const float& Vec3::operator [](int i) const
{
	return ((&x)[i]);
}

Vec3& Vec3::operator *=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	return (*this);
}

Vec3& Vec3::operator /=(float s)
{
	s = 1.0f / s;

	x *= s;
	y *= s;
	z *= s;
	return (*this);
}

Vec3& Vec3::operator +=(const Vec3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return (*this);
}

Vec3& Vec3::operator -=(const Vec3& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return (*this);
}

} // namespace Math

#pragma once

#include "vec3.h"

namespace Math
{

// NOTE: Point3 representes a position vector with an
// implicit w coordinate of 1
struct Point3 : Vec3
{
	Point3() = default;
	Point3(float a, float b, float c);
};

inline Point3 operator +(const Point3& a, const Vec3& b)
{
	return Point3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 operator -(const Point3& a, const Point3& b)
{
	return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

} // namespace Math

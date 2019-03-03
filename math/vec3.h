#pragma once
#include <math.h>

namespace Math
{

struct Vec3
{
	float x, y, z;

	Vec3() = default;
	Vec3(float a, float b, float c);

	float& operator [](int i);
	const float& operator [](int i) const;

	Vec3& operator *=(float s);
	Vec3& operator /=(float s);
	Vec3& operator +=(const Vec3& v);
	Vec3& operator -=(const Vec3& v);
};

inline Vec3 operator *(const Vec3& v, float s)
{
	return (Vec3(v.x * s, v.y * s, v.z * s));
}

inline Vec3 operator /(const Vec3& v, float s)
{
	s = 1.0f / s;
	return (Vec3(v.x * s, v.y * s, v.z * s));
}

inline Vec3 operator -(const Vec3& v)
{
	return (Vec3(-v.x, -v.y, -v.z));
}

inline float magnitude(const Vec3& v)
{
	return (sqrtf(v.x * v.x + v.y * v.y + v.z * v.z));
}

inline Vec3 normalize(const Vec3& v)
{
	return v / magnitude(v);
}

inline Vec3 operator +(const Vec3& a, const Vec3& b)
{
	return (Vec3(a.x + b.x, a.y + b.y, a.z + b.z));
}

inline Vec3 operator -(const Vec3& a, const Vec3& b)
{
	return (Vec3(a.x - b.x, a.y - b.y, a.z - b.z));
}

inline float dot(const Vec3& a, const Vec3& b)
{
	return (a.x * b.x + a.y * b.y + a.z * a.z);
}

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
	return (Vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x));
}

inline Vec3 project(const Vec3& a, const Vec3& b)
{
	return (b * (dot(a, b) / dot(b, b)));
}

inline Vec3 reject(const Vec3& a, const Vec3& b)
{
	return (a - b * (dot(a, b) / dot(b, b)));
}

} // namespace Math

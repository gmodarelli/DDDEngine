#pragma once
#include <math.h>

namespace Math
{

struct Vec4
{
	float x, y, z, w;

	Vec4() = default;
	Vec4(float a, float b, float c, float d);

	float& operator [](int i);
	const float& operator [](int i) const;

	Vec4& operator *=(float s);
	Vec4& operator /=(float s);
	Vec4& operator +=(const Vec4& v);
	Vec4& operator -=(const Vec4& v);
};

inline Vec4 operator *(const Vec4& v, float s)
{
	return (Vec4(v.x * s, v.y * s, v.z * s, v.w * s));
}

inline Vec4 operator /(const Vec4& v, float s)
{
	s = 1.0f / s;
	return (Vec4(v.x * s, v.y * s, v.z * s, v.w * s));
}

inline Vec4 operator -(const Vec4& v)
{
	return (Vec4(-v.x, -v.y, -v.z, -v.w));
}

inline float magnitude(const Vec4& v)
{
	return (sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w));
}

inline Vec4 normalize(const Vec4& v)
{
	return v / magnitude(v);
}

inline Vec4 operator +(const Vec4& a, const Vec4& b)
{
	return (Vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w));
}

inline Vec4 operator -(const Vec4& a, const Vec4& b)
{
	return (Vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w));
}

inline float dot(const Vec4& a, const Vec4& b)
{
	return (a.x * b.x + a.y * b.y + a.z * a.z + a.w * b.w);
}

} // namespace Math

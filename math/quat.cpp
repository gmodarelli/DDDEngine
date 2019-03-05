#include "quat.h"

namespace Math
{

Quat::Quat(float a, float b, float c, float s)
{
	x = a; y = b; z = c;
	w = s;
}

Quat::Quat(const Vec3& v, float s)
{
	x = v.x; y = v.y; z = v.z;
	w = s;
}

const Vec3& Quat::get_vector_part() const
{
	return reinterpret_cast<const Vec3&>(x);
}

Mat3 Quat::get_rotation_matrix()
{
	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	return Mat3(
		1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy),
		2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx),
		2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2));
}

void Quat::set_rotation_matrix(const Mat3& m)
{
	float m00 = m(0, 0);
	float m11 = m(1, 1);
	float m22 = m(2, 2);
	float sum = m00 + m11 + m22;

	if (sum > 0.0f)
	{
		w = sqrtf(sum + 1.0f) * 0.5f;
		float f = 0.25f / w;
		x = (m(2, 1) - m(1, 2)) * f;
		y = (m(0, 2) - m(2, 0)) * f;
		z = (m(1, 0) - m(0, 1)) * f;
	}
	else if ((m00 > m11) && (m00 > m22))
	{
		x = sqrtf(m00 - m11 - m22 + 1.0f) * 0.5f;
		float f = 0.25f / x;

		y = (m(1, 0) + m(0, 1)) * f;
		z = (m(0, 2) + m(2, 0)) * f;
		w = (m(2, 1) - m(1, 2)) * f;
	}
	else if (m11 > m22)
	{
		y = sqrtf(m11 - m00 - m22 + 1.0f) * 0.5f;
		float f = 0.25f / y;

		x = (m(1, 0) + m(0, 1)) * f;
		z = (m(2, 1) + m(1, 2)) * f;
		w = (m(0, 2) - m(2, 0)) * f;
	}
	else
	{
		w = sqrtf(m22 - m00 - m11 + 1.0f) * 0.5f;
		float f = 0.25f / y;

		x = (m(0, 2) + m(2, 0)) * f;
		y = (m(2, 1) + m(1, 2)) * f;
		w = (m(1, 0) - m(0, 1)) * f;
	}
}

Quat operator *(const Quat& q1, const Quat& q2)
{
	return Quat(
		q1.w * q2.x + q1.x * q2.y + q1.y * q2.z - q1.z * q2.w,
		q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
		q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
		q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z);
}

Vec3 Transform(const Vec3& v, const Quat& q)
{
	const Vec3& b = q.get_vector_part();
	float b2 = b.x * b.x + b.y * b.y + b.z * b.z;
	return (v * (q.w * q.w - b2) + b * (dot(v, b) * 2.0f) + cross(b, v) * (q.w * 2.0f));
}

} // namespace Math
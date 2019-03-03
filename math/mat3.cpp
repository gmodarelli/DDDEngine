#include "vec3.h"
#include "mat3.h"
#include <cassert>

namespace Math
{

Mat3::Mat3(float n00, float n01, float n02,
	float n10, float n11, float n12,
	float n20, float n21, float n22)
{
	n[0][0] = n00;
	n[0][1] = n01;
	n[0][2] = n02;
	n[1][0] = n10;
	n[1][1] = n11;
	n[1][2] = n12;
	n[2][0] = n20;
	n[2][1] = n21;
	n[2][2] = n22;
}

Mat3::Mat3(const Vec3& a, const Vec3& b, const Vec3& c)
{
	n[0][0] = a.x;
	n[0][1] = a.y;
	n[0][2] = a.z;
	n[1][0] = b.x;
	n[1][1] = b.y;
	n[1][2] = b.z;
	n[2][0] = c.x;
	n[2][1] = c.y;
	n[2][2] = c.z;
}

float& Mat3::operator ()(int i, int j)
{
	return (n[j][i]);
}

const float& Mat3::operator ()(int i, int j) const
{
	return (n[j][i]);
}

Vec3& Mat3::operator [](int j)
{
	return (*reinterpret_cast<Vec3 *>(n[j]));
}

const Vec3& Mat3::operator [](int j) const
{
	return (*reinterpret_cast<const Vec3 *>(n[j]));
}

Mat3 operator *(const Mat3& a, const Mat3& b)
{
	return (Mat3(
		a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0),
		a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1),
		a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2),
		a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0),
		a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1),
		a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2),
		a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0),
		a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1),
		a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2)));
}

Vec3 operator *(const Mat3& m, const Vec3& v)
{
	return (Vec3(
		m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z,
		m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z,
		m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z));
}

float determinant(const Mat3& m)
{
	return (
		m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) +
		m(0, 1) * (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) +
		m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)));
}

Mat3 inverse(const Mat3& m)
{
	const Vec3& a = m[0];
	const Vec3& b = m[1];
	const Vec3& c = m[2];

	Vec3 r0 = cross(b, c);
	Vec3 r1 = cross(c, a);
	Vec3 r2 = cross(a, b);

	float inverse_determinant = 1.0f / dot(r2, c);

	return (Mat3(
		r0.x * inverse_determinant, r0.y * inverse_determinant, r0.z * inverse_determinant,
		r1.x * inverse_determinant, r1.y * inverse_determinant, r1.z * inverse_determinant,
		r2.x * inverse_determinant, r2.y * inverse_determinant, r2.z * inverse_determinant));
}

Mat3 make_rotation_x(float t)
{
	float c = cosf(t);
	float s = sinf(t);

	return Mat3(
		1.0f, 0.0f, 0.0f,
		0.0f, c, -s,
		0.0f, s, c);
}

Mat3 make_rotation_y(float t)
{
	float c = cosf(t);
	float s = sinf(t);

	return Mat3(
		c, 0.0f, s,
		0.0f, 1.0f, 0.0f,
		-s, 0.0f, c);
}
Mat3 make_rotation_z(float t)
{
	float c = cosf(t);
	float s = sinf(t);

	return Mat3(
		c, -s, 0.0f,
		s, c, 0.0f,
		0.0f, 0.0f, 1.0f);
}

Mat3 make_rotation(float t, const Vec3& a)
{
	// NOTE: we assume a to be a unit vector
	assert(magnitude(a) == 1.0f);

	float c = cosf(t);
	float s = sinf(t);
	float d = 1.0f - c;

	float x = a.x * d;
	float y = a.y * d;
	float z = a.z * d;

	float axay = x * a.y;
	float axaz = x * a.z;
	float ayaz = y * a.z;

	return Mat3(
		c + x * a.x, axay - s * a.z, axaz + s * a.y,
		axay + s * a.z, c + y * a.y, ayaz - s * a.x,
		axaz - s * a.y, ayaz + s * a.x, c + z * a.z);
}

Mat3 make_reflection(const Vec3& a)
{
	// NOTE: we assume a to be a unit vector
	assert(magnitude(a) == 1.0f);

	float x = a.x * -2.0f;
	float y = a.y * -2.0f;
	float z = a.z * -2.0f;
	float axay = x * a.y;
	float axaz = x * a.z;
	float ayaz = y * a.z;

	return Mat3(
		x * a.x + 1.0f, axay, axaz,
		axay, y * a.y + 1.0f, ayaz,
		axaz, ayaz, z * a.z + 1.0f);
}

Mat3 make_involution(const Vec3& a)
{
	// NOTE: we assume a to be a unit vector
	assert(magnitude(a) == 1.0f);

	float x = a.x * 2.0f;
	float y = a.y * 2.0f;
	float z = a.z * 2.0f;
	float axay = x * a.y;
	float axaz = x * a.z;
	float ayaz = y * a.z;

	return Mat3(
		x * a.x - 1.0f, axay, axaz,
		axay, y * a.y - 1.0f, ayaz,
		axaz, ayaz, z * a.z - 1.0f);
}

} // namespace Math
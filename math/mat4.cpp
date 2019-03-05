#include "mat4.h"

namespace Math
{

Mat4::Mat4(float n00, float n01, float n02, float n03,
	float n10, float n11, float n12, float n13,
	float n20, float n21, float n22, float n23,
	float n30, float n31, float n32, float n33)
{
	n[0][0] = n00; n[0][1] = n10; n[0][2] = n20; n[0][3] = n30;
	n[1][0] = n01; n[1][1] = n11; n[1][2] = n21; n[1][3] = n31;
	n[2][0] = n02; n[2][1] = n12; n[2][2] = n22; n[2][3] = n32;
	n[3][0] = n03; n[3][1] = n13; n[3][2] = n23; n[3][3] = n33;
}

Mat4::Mat4(const Vec4& a, const Vec4& b, const Vec4& c, const Vec4& d)
{
	n[0][0] = a.x; n[0][1] = a.y; n[0][2] = a.z; n[0][3] = a.w;
	n[1][0] = b.x; n[1][1] = b.y; n[1][2] = b.z; n[1][3] = b.w;
	n[2][0] = c.x; n[2][1] = c.y; n[2][2] = c.z; n[2][3] = c.w;
	n[3][0] = d.x; n[3][1] = d.y; n[3][2] = d.z; n[3][3] = d.w;
}

float& Mat4::operator ()(int i, int j)
{
	return (n[j][i]);
}

const float& Mat4::operator ()(int i, int j) const
{
	return (n[j][i]);
}

Vec4& Mat4::operator [](int j)
{
	return (*reinterpret_cast<Vec4 *>(n[j]));
}

const Vec4& Mat4::operator [](int j) const
{
	return (*reinterpret_cast<const Vec4 *>(n[j]));
}

Mat4 operator *(const Mat4& a, const Mat4& b)
{
	return (Mat4(
		a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0) + a(0, 3) * b(3, 0),
		a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1) + a(0, 3) * b(3, 1),
		a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2) + a(0, 3) * b(3, 2),
		a(0, 0) * b(0, 3) + a(0, 1) * b(1, 3) + a(0, 2) * b(2, 3) + a(0, 3) * b(3, 3),
		a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0) + a(1, 3) * b(3, 0),
		a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1) + a(1, 3) * b(3, 1),
		a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2) + a(1, 3) * b(3, 2),
		a(1, 0) * b(0, 3) + a(1, 1) * b(1, 3) + a(1, 2) * b(2, 3) + a(1, 3) * b(3, 3),
		a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0) + a(2, 3) * b(3, 0),
		a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1) + a(2, 3) * b(3, 1),
		a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2) + a(2, 3) * b(3, 2),
		a(2, 0) * b(0, 3) + a(2, 1) * b(1, 3) + a(2, 2) * b(2, 3) + a(2, 3) * b(3, 3),
		a(3, 0) * b(0, 0) + a(3, 1) * b(1, 0) + a(3, 2) * b(2, 0) + a(3, 3) * b(3, 0),
		a(3, 0) * b(0, 1) + a(3, 1) * b(1, 1) + a(3, 2) * b(2, 1) + a(3, 3) * b(3, 1),
		a(3, 0) * b(0, 2) + a(3, 1) * b(1, 2) + a(3, 2) * b(2, 2) + a(3, 3) * b(3, 2),
		a(3, 0) * b(0, 3) + a(3, 1) * b(1, 3) + a(3, 2) * b(2, 3) + a(3, 3) * b(3, 3)));
}

Vec4 operator *(const Mat4& m, const Vec4& v)
{
	return (Vec4(
		m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z + m(0, 3) * v.w,
		m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z + m(1, 3) * v.w,
		m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z + m(2, 3) * v.w,
		m(3, 0) * v.x + m(3, 1) * v.y + m(3, 2) * v.z + m(3, 3) * v.w));
}

Mat4 inverse(const Mat4& m)
{
	const Vec3& a = reinterpret_cast<const Vec3&>(m[0]);
	const Vec3& b = reinterpret_cast<const Vec3&>(m[1]);
	const Vec3& c = reinterpret_cast<const Vec3&>(m[2]);
	const Vec3& d = reinterpret_cast<const Vec3&>(m[3]);

	const float& x = m(3, 0);
	const float& y = m(3, 1);
	const float& z = m(3, 2);
	const float& w = m(3, 3);

	Vec3 s = cross(a, b);
	Vec3 t = cross(c, d);
	Vec3 u = a * y - b * x;
	Vec3 v = c * w - d * z;

	float inverse_determinant = 1.0f / (dot(s, v) + dot(t, u));
	s *= inverse_determinant;
	t *= inverse_determinant;
	u *= inverse_determinant;
	v *= inverse_determinant;

	Vec3 r0 = cross(b, v) + t * y;
	Vec3 r1 = cross(v, a) - t * x;
	Vec3 r2 = cross(d, u) + s * w;
	Vec3 r3 = cross(u, c) - s * z;

	return Mat4(
		r0.x, r0.y, r0.z, -dot(b, t),
		r1.x, r1.y, r1.z,  dot(a, t),
		r2.x, r2.y, r2.z, -dot(d, s),
		r3.x, r3.y, r3.z, dot(c, s));
}

Mat4 translate(const Mat4& m, const Vec3& v)
{
	const Vec4& a = m[0];
	const Vec4& b = m[1];
	const Vec4& c = m[2];
	Vec4 d = m[3];

	d.x = v.x;
	d.y = v.y;
	d.z = v.z;

	return Mat4(a, b, c, d);
}

} // namespace Math

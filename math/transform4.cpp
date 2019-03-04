#include "transform4.h"

namespace Math
{

Transform4::Transform4(
	float n00, float n01, float n02, float n03,
	float n10, float n11, float n12, float n13,
	float n20, float n21, float n22, float n23)
{
	n[0][0] = n00; n[0][1] = n10; n[0][2] = n20; n[0][3] = 0.0f;
	n[1][0] = n01; n[1][1] = n11; n[1][2] = n21; n[1][3] = 0.0f;
	n[2][0] = n02; n[2][1] = n12; n[2][2] = n22; n[2][3] = 0.0f;
	n[3][0] = n03; n[3][1] = n13; n[3][2] = n23; n[3][3] = 1.0f;
}

Transform4::Transform4(const Vec3& a, const Vec3& b, const Vec3& c, const Point3& p)
{
	n[0][0] = a.x; n[0][1] = a.y; n[0][2] = a.z; n[0][3] = 0.0f;
	n[1][0] = b.x; n[1][1] = b.y; n[1][2] = b.z; n[1][3] = 0.0f;
	n[2][0] = c.x; n[2][1] = c.y; n[2][2] = c.z; n[2][3] = 0.0f;
	n[3][0] = p.x; n[3][1] = p.y; n[3][2] = p.z; n[3][3] = 1.0f;
}

Vec3& Transform4::operator [](int j)
{
	return *reinterpret_cast<Vec3 *>(n[j]);
}

const Vec3& Transform4::operator [](int j) const
{
	return *reinterpret_cast<const Vec3 *>(n[j]);
}

const Point3& Transform4::get_translation() const
{
	return *reinterpret_cast<const Point3 *>(n[3]);
}

void Transform4::set_translation(const Point3& p)
{
	n[3][0] = p.x;
	n[3][1] = p.y;
	n[3][2] = p.z;
}

Transform4 inverse(const Transform4& h)
{
	const Vec3& a = h[0];
	const Vec3& b = h[1];
	const Vec3& c = h[2];
	const Vec3& d = h[3];

	Vec3 s = cross(a, b);
	Vec3 t = cross(c, d);

	float inverse_determinant = 1.0f / dot(s, c);
	
	s *= inverse_determinant;
	t *= inverse_determinant;
	Vec3 v = c * inverse_determinant;

	Vec3 r0 = cross(b, v);
	Vec3 r1 = cross(v, a);

	return Transform4(
		r0.x, r0.y, r0.z, -dot(b, t),
		r1.x, r1.y, r1.z,  dot(a, t),
		 s.x,  s.y,  s.z, -dot(d, s));
}

Transform4 operator *(const Transform4& a, const Transform4& b)
{
	return Transform4(
		a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0),
		a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1),
		a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2),
		a(0, 0) * b(0, 3) + a(0, 1) * b(1, 3) + a(0, 2) * b(2, 3) + a(0, 3),
		a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0),
		a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1),
		a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2),
		a(1, 0) * b(0, 3) + a(1, 1) * b(1, 3) + a(1, 2) * b(2, 3) + a(1, 3),
		a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0),
		a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1),
		a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2),
		a(2, 0) * b(0, 3) + a(2, 1) * b(1, 3) + a(2, 2) * b(2, 3) + a(2, 3));
}

Vec3 operator *(const Transform4& h, Vec3& v)
{
	return Vec3(
		h(0, 0) * v.x + h(0, 1) * v.y + h(0, 2) * v.z,
		h(1, 0) * v.x + h(1, 1) * v.y + h(1, 2) * v.z,
		h(2, 0) * v.x + h(2, 1) * v.y + h(2, 2) * v.z);
}

Point3 operator *(const Transform4& h, Point3& p)
{
	return Point3(
		h(0, 0) * p.x + h(0, 1) * p.y + h(0, 2) * p.z + h(0, 3),
		h(1, 0) * p.x + h(1, 1) * p.y + h(1, 2) * p.z + h(1, 3),
		h(2, 0) * p.x + h(2, 1) * p.y + h(2, 2) * p.z + h(2, 3));
}

} // namespace Math

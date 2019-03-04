#pragma once
#include "mat4.h"
#include "vec3.h"
#include "point3.h"

namespace Math
{

struct Transform4 : Mat4
{
	Transform4() = default;

	Transform4(
		float n00, float n01, float n02, float n03,
		float n10, float n11, float n12, float n13,
		float n20, float n21, float n22, float n23);

	Transform4(const Vec3& a, const Vec3& b, const Vec3& c, const Point3& p);

	Vec3& operator [](int j);
	const Vec3& operator [](int j) const;
	const Point3& get_translation() const;
	void set_translation(const Point3& p);
};

Transform4 inverse(const Transform4& h);
Transform4 operator *(const Transform4& a, const Transform4& b);
Vec3 operator *(const Transform4& h, Vec3& v);
Point3 operator *(const Transform4& h, Point3& p);

} // namespace Math

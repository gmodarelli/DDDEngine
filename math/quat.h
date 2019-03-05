#pragma once
#include "vec3.h"
#include "mat3.h"

namespace Math
{

struct Quat
{
	float x, y, z, w;

	Quat() = default;

	Quat(float a, float b, float c, float d);
	Quat(const Vec3& v, float s);

	const Vec3& get_vector_part() const;
	Mat3 get_rotation_matrix();
	void set_rotation_matrix(const Mat3& m);
};

Quat operator *(const Quat& q1, const Quat& q2);
Vec3 Transform(const Vec3& v, const Quat& q);

} // namespace Math

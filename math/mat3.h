#pragma once

#include "vec3.h"

namespace Math
{

struct Mat3
{
	// NOTE: We arrange entries in memory in a column-major order. 
	float n[3][3];

	Mat3() = default;

	Mat3(float n00, float n01, float n02,
		float n10, float n11, float n12,
		float n20, float n21, float n22);

	Mat3(const Vec3& a, const Vec3& b, const Vec3& c);

	// NOTE: Since we arrange entries in memory in a column-major order, 
	// we also provide an operator to access the matrix in a row-major order.
	float& operator ()(int i, int j);
	const float& operator ()(int i, int j) const;
	Vec3& operator [](int j);
	const Vec3& operator [](int j) const;
};

Mat3 operator *(const Mat3& a, const Mat3& b);
Vec3 operator *(const Mat3& m, const Vec3& v);
float determinant(const Mat3& m);
Mat3 inverse(const Mat3& m);

Mat3 make_rotation_x(float t);
Mat3 make_rotation_y(float t);
Mat3 make_rotation_z(float t);
Mat3 make_rotation(float t, const Vec3& a);
Mat3 make_reflection(const Vec3& a);
Mat3 make_involution(const Vec3& a);

} // namespace Math

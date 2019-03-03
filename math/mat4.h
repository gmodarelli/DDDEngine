#pragma once

#include "vec4.h"

namespace Math
{

struct Mat4
{
	// NOTE: We arrange entries in memory in a column-major order. 
	float n[4][4];

	Mat4() = default;

	Mat4(float n00, float n01, float n02, float n03,
		float n10, float n11, float n12, float n13,
		float n20, float n21, float n22, float n23,
		float n30, float n31, float n32, float n33);

	Mat4(const Vec4& a, const Vec4& b, const Vec4& c, const Vec4& d);
	Vec4& operator [](int j);
	const Vec4& operator [](int j) const;

	// NOTE: Since we arrange entries in memory in a column-major order, 
	// we also provide an operator to access the matrix in a row-major order.
	float& operator ()(int i, int j);
	const float& operator ()(int i, int j) const;
};

Mat4 operator *(const Mat4& a, const Mat4& b);
Vec4 operator *(const Mat4& m, const Vec4& v);
Mat4 inverse(const Mat4& m);

} // namespace Math

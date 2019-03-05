#include "math.h"

namespace Math
{

float radians(float degrees)
{
	return degrees * 0.01745329251994329576923690768489f;
}

float degrees(float radians)
{
	return radians * 57.295779513082320876798154814105f;
}

} // namespace Math

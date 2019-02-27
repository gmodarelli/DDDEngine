#version 450

layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outFragColor;

layout (push_constant) uniform Material
{
	layout(offset = 64) vec4 baseColorFormat;
	float metallicFactor;
	float roughnessFactor;
} material;

void main()
{
	outFragColor = material.baseColorFormat;
}

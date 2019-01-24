#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPosition;
} ubo;

layout (push_constant) uniform Material
{
	vec4 baseColorFormat;
	float metallicFactor;
	float roughnessFactor;
} material;

void main()
{
	outFragColor = material.baseColorFormat;
}

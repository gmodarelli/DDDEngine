#version 450

layout (set = 0, binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPosition;
} ubo;

layout (set = 1, binding = 0) uniform UBONode
{
	mat4 matrix;
} node;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
// layout (location = 2) in vec3 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
	vec4 localPosition = ubo.model * node.matrix * vec4(inPos, 1.0f);
	vec3 worldPosition = localPosition.xyz / localPosition.w;

	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(worldPosition.xyz, 1.0f);

	outColor = vec4(normalize(inNormal), 1);
}
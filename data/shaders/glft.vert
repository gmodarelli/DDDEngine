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

layout (location = 0) out vec3 outWorldPosition;
layout (location = 1) out vec3 outNormal;

void main()
{
	vec4 localPosition = ubo.model * node.matrix * vec4(inPos, 1.0f);
	outWorldPosition = localPosition.xyz / localPosition.w;
	outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix))) * inNormal);

	gl_Position = ubo.projection * ubo.view * vec4(outWorldPosition.xyz, 1.0f);
}
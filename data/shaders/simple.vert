#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inUV;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPosition;
} ubo;

layout (set = 0, binding = 1) uniform UBONode
{
	mat4 matrix;
} node;

layout(location = 0) out vec4 outColor;

void main()
{	
	
	mat4 modelView = ubo.view * ubo.model;
	gl_Position = ubo.projection * modelView * vec4(inPos.xyz, 1);
	outColor = vec4(inNormal, 1);
}
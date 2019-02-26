#version 450

layout (set = 0, binding = 0) uniform ViewUniformBufferObject
{
	mat4 view;
	mat4 projection;
} ubo;

layout (set = 1, binding = 0) uniform NodeUniformBufferObject
{
	mat4 model;
} node;

layout (push_constant) uniform Model
{
	mat4 model;
} entity;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 fragColor;

void main()
{
	gl_Position = ubo.projection * ubo.view * entity.model * node.model * vec4(inPosition, 1.0f);
	fragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

#version 450

layout (binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec4 fragColor;

void main()
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 0.0f, 1.0f);
	fragColor = vec4(inColor, 1.0f);
}

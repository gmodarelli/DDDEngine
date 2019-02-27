#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 frag_color;

void main()
{
	gl_Position = ubo.projection * ubo.view * vec4(in_position, 1.0f);
	frag_color = in_color;
}

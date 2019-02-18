#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 view;
	mat4 projection;
} ubo;

layout (push_constant) uniform Model
{
	mat4 model;
} model;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 fragColor;

void main()
{
	gl_Position = ubo.projection * ubo.view * model.model * vec4(inPosition, 1.0f);
	fragColor = vec4(inColor, 1.0f);
}

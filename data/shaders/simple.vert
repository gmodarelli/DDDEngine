#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 3) in vec3 instancePosition;
layout (location = 4) in vec3 instanceScale;

layout (location = 0) out vec4 fragColor;

void main()
{
    vec4 pos = vec4((inPosition.xyz * instanceScale) + instancePosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * ubo.model * pos;
	fragColor = vec4(inColor, 1.0f);
}

#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 3) in vec3 instancePosition;
layout (location = 4) in vec3 instanceScale;
layout (location = 5) in vec4 instanceRotation;

layout (location = 0) out vec4 fragColor;

void main()
{
	vec3 b = instanceRotation.xyz;
	float b2 = b.x * b.x + b.y * b.y + b.z * b.z;
	vec3 rotatedPosition = (inPosition * (instanceRotation.w * instanceRotation.w - b2) + b * (dot(inPosition, b) * 2.0f) + cross(b, inPosition) * (instanceRotation.w * 2.0f));
    vec4 pos = vec4((rotatedPosition.xyz * instanceScale) + instancePosition, 1.0f);
	gl_Position = ubo.projection * ubo.view * pos;
	fragColor = vec4(inColor, 1.0f);
}

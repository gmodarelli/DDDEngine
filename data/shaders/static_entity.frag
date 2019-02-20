#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 0) out vec4 outFragColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
	// outFragColor = texture(texSampler, inTexCoord); // inColor;
	outFragColor = inColor;
}
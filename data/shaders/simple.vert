#version 420

layout(std140, set = 0, binding = 0) uniform buffer
{
	mat4 inProjectionMatrix;
	mat4 inViewMatrix;
	mat4 inModelMatrix;
};

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	mat4 mvp = inModelMatrix * inViewMatrix * inProjectionMatrix;
	gl_Position = vec4(inPos.xyz, 1) * mvp;
	outColor = inColor;
}
#version 420

layout(std140, set = 0, binding = 0) uniform buffer
{
	mat4 inProjectionMatrix;
	mat4 inViewMatrix;
	mat4 inModelMatrix;
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main()
{
	mat4 mvp = inModelMatrix * inViewMatrix * inProjectionMatrix;
	gl_Position = vec4(inPos.xyz, 1) * mvp;
	
	// Hard-coded lamber diffuse light
	vec3 lightDirection = vec3(0, 5, 2);
	vec3 lightColor = vec3(0.8, 0.8, 0.8);
	vec3 surfaceColor = vec3(1, 1, 1);
	float lightAttenuation = 0.25f;
	
	float NdotL = max(0.0, dot(inNormal, lightDirection));
	vec3 LambertDiffuse = NdotL * surfaceColor;
	vec3 finalColor = LambertDiffuse * lightAttenuation * lightColor;
	outColor = vec4(finalColor.rgb, 1);
}
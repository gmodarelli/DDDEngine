#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightPos;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inUV;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main()
{	
	
	mat4 modelView = ubo.view * ubo.model;
	gl_Position = ubo.projection * modelView * vec4(inPos.xyz, 1);
	outColor = vec4(inNormal, 1);
	
	/*
	// Hard-coded lamber diffuse light
	vec3 lightColor = vec3(0.8, 0.8, 0.8);
	float lightAttenuation = 0.25f;
	
	float NdotL = max(0.0, dot(inNormal, ubo.lightPos.xyz));
	vec3 lambertDiffuse = NdotL * inColor;
	vec3 finalColor = lambertDiffuse * lightAttenuation * lightColor;
	outColor = vec4(finalColor.rgb, 1);
	*/
}
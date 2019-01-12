#version 450

layout(binding = 0) uniform UniformBufferObject
{
	mat4 projection;
	mat4 view;
	mat4 model;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

/*
struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;
	float tu, tv;
};

layout(binding = 1) readonly buffer Vertices
{
	Vertex vertices[];
};
*/
layout(location = 0) out vec4 outColor;


void main()
{	
	/*
	Vertex v = vertices[gl_VertexIndex];
	
	vec3 position = vec3(v.vx, v.vy, v.vz);
	vec3 normal = vec3(v.nx, v.ny, v.nz);
	vec2 uv = vec2(v.tu, v.tv);
	*/
	
	mat4 mvp = ubo.model * ubo.view * ubo.projection;	
	gl_Position = vec4(position.xyz, 1) * mvp;
	// Hard-coded lamber diffuse light
	vec3 lightDirection = vec3(0, 5, 2);
	vec3 lightColor = vec3(0.8, 0.8, 0.8);
	vec3 surfaceColor = vec3(1, 1, 1);
	float lightAttenuation = 0.25f;
	
	float NdotL = max(0.0, dot(normal, lightDirection));
	vec3 lambertDiffuse = NdotL * surfaceColor;
	vec3 finalColor = lambertDiffuse * lightAttenuation * lightColor;
	outColor = vec4(finalColor.rgb, 1);
}
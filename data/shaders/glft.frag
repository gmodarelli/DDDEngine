#version 450
#define PI 3.1415926535897932384626433832795

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inWorldNormal;

layout(location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPosition;
} ubo;

layout (set = 0, binding = 1) uniform UBOParams
{
	vec3 lightPosition;
	vec4 lightColor;
} uboParams;

layout (push_constant) uniform Material
{
	vec4 baseColorFormat;
	float metallicFactor;
	float roughnessFactor;
} material;

// Normal Distribution Function
float DistributionGGX(vec3 N, vec3 H, float a);
// Geometry Functions
float GeometrySchlickGGX(float NdotV, float k);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k);
// Fresnel Function
vec3 FresnelSchlick(float cosTheta, vec3 F0);

void main()
{
	// TODO: Compute the albedo form the albedo map once we have one
	vec3 albedo = material.baseColorFormat.rgb;

    vec3 N = normalize(inWorldNormal);
	vec3 V = normalize(ubo.cameraPosition - inWorldPos);

	// Precomputing F0
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, material.baseColorFormat.rgb, material.metallicFactor);

	// Reflectance equation
	// calculate direct-light radiance
	vec3 L = normalize(uboParams.lightPosition - inWorldPos);
	vec3 H = normalize(V + L);
	float distance = length(uboParams.lightPosition - inWorldPos);
	float attenuation = 1.0f / (distance * distance);
	vec3 radiance = uboParams.lightColor.rgb * attenuation;

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, material.roughnessFactor);
	float G = GeometrySmith(N, V, L, material.roughnessFactor);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	kD *= 1.0f - material.metallicFactor;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
	vec3 specular = numerator / max(denominator, 0.001f);

	float NdotL = max(dot(N, L), 0.0f);
	// TODO: When adding multiple light we need to accumulate (sum) all light radiances into Lo
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

	// TODO: When we have ambient maps change this to `color = ambient + Lo;`
	vec3 color = Lo; 
	// TODO: Write why we're manipulating the color this way
	color = color / (color + vec3(1.0f));
	color = pow(color, vec3(1.0f / 2.0f));
	outFragColor = vec4(color, 1.0f);
}

// Normal Distribution Function
//
// Trowbridge-Reitz GGX
// TODO: Add an explanation for what this function does
// Params:
// N: normal
// H: halfway vector
// a: surface roughness
//
// source: https://learnopengl.com/PBR/Theory
float DistributionGGX(vec3 N, vec3 H, float a)
{
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return nom / denom;
}

// Geometry Functions
//
// Schlick GGX
// TODO: Add an explanation for what this function does
// Params:
// NdotV: The dot product beween the normal and the view direction
// k: A remapping of a (surface roughness) for direct lighting or IBL lighting
//
// source: https://learnopengl.com/PBR/Theory
float GeometrySchlickGGX(float NdotV, float k)
{
	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}

// Smith's Method
// TODO: Add an explanation for what this function does
// Params:
// N: surface normal
// V: view direction
// L: light direction
// k: A remapping of a (surface roughness) for direct lighting or IBL lighting
//
// source: https://learnopengl.com/PBR/Theory
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx1 = GeometrySchlickGGX(NdotV, k);
	float ggx2 = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

// Fresnel Function
// TODO: Add an explanation for what this function does
// Params:
// cosTheta: the dot product between the surface's normal and the view direction
// F0: the base reflectivity of the surface. F0 can be precomputed for both 
//	   dielectircs and conductors and we can use the same Fresnel approximation
//	   for both type of surfaces.
//
// source: https://learnopengl.com/PBR/Theory
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f + F0) * pow(1.0f - cosTheta, 5.0f);
}

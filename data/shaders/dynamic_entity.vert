#version 450

layout (set = 0, binding = 0) uniform ViewUniformBufferObject
{
	mat4 view;
	mat4 projection;
	vec3 camera_position;
} ubo;

layout (set = 1, binding = 0) uniform NodeUniformBufferObject
{
	mat4 model;
} node;

layout (push_constant) uniform Model
{
	mat4 model;
} entity;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coord_0;

layout (location = 0) out vec3 out_world_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_world_normal;

void main()
{
	vec4 local_position = entity.model * node.model * vec4(in_position, 1.0f);

	out_world_position = local_position.xyz / local_position.w;
	out_normal = in_normal;
	// Transform the normal from object space to world space
	// TODO: Maybe add why we're inverting, transposing and normalizing
	out_world_normal = normalize(transpose(inverse(mat3(entity.model * node.model))) * in_normal);

	gl_Position = ubo.projection * ubo.view * local_position;
}

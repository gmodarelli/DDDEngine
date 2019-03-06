#pragma once
#include "../renderer/types.h"
#include "../renderer/camera.h"
#include "../resources/resources.h"

namespace Game
{

struct State
{
	// We are using a Data Oriented Design to store all the
	// game state informations.
	//
	// The size of the following arrays will be determined
	// when a level is loaded and it will not grow during
	// the lifetime of the level (I guess...)
	//
	// Entities are all dynamic entities in the level.
	// Their transforms will be written to in the simulation
	// loop. The renderer loop will read the transforms
	// and upload them to the GPU every frame.
	// For now we don't make any distinction between
	// entities and their rendering frequencies will be the same.
	uint32_t entity_count = 0;
	Renderer::Entity* entities = nullptr;
	// Static entities are entits whose transforms will
	// never changed, so we can optimize both the simulation
	// loop and the render loop
	uint32_t static_entity_count = 0;
	Renderer::Entity* static_entities = nullptr;
	// A Transform contains the position, scale and
	// rotation of an entity.
	uint32_t transform_count = 0;
	Renderer::Transform* transforms = nullptr;
	uint32_t static_transform_count = 0;
	Renderer::Transform* static_transforms = nullptr;

	const Resources::AssetsInfo* assets_info;

	// Player
	uint32_t player_entity_id = 0;
	glm::mat4 player_matrices[3] = { glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f) };
	glm::mat4 player_orientation = glm::mat4(1.0f);
	glm::vec3 player_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 player_target_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	float player_speed = 0.6f;
	float player_rotation_speed = 4.0f;

	// Current camera
	Renderer::Camera* current_camera;
	// Debug Camera
	Renderer::Camera* fly_camera;
	// Game Camera
	Renderer::Camera* look_at_camera;

	// Editor
	bool show_grid = false;
};

}

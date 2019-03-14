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
	// TODO: The entities table contains both static and dynamic entities.
	//		 Move the static entities to the top of the table and the dynamic
	//		 right after.
	uint32_t entity_count = 0;
	Renderer::Entity* entities = nullptr;
	// A Transform contains the position, scale and
	// rotation of an entity.
	uint32_t transform_count = 0;
	Renderer::Transform* transforms = nullptr;

	const Resources::AssetsInfo* assets_info;

	bool paused = true;

	// Player
	// NOTE: This must be equal to 0.6f / 25 where
	//		 0.6f is the size of a quad in the game grid, and
	//		 25 is the numbers of tick per seconds (ie the simulation
	//		 is run 25 times per second.
	// NOTE: 25 ticks to move by 1 square
	// float player_speed = 0.024f;
	// NOTE: 20 ticks to move by 1 square
	// float player_speed = 0.03f;
	// NOTE: 15 ticks to move by 1 square
	float player_speed = 0.04f;
	// NOTE: 10 ticks to move by 1 square
	// float player_speed = 0.06f;

	uint32_t apple_id = 0;
	uint32_t player_head_id = 0;
	bool queued_growing = false;
	bool growing = false;

	// NOTE: We store the positions where
	struct PlayerMove
	{
		glm::vec3 position;
		glm::vec3 direction;
		glm::mat4 orientation;
	};

	struct BodyPart
	{
		uint32_t target_move_index = 0;
		glm::vec3 position;
		glm::vec3 direction;
		glm::mat4 orientation;
	};

	static const uint32_t max_moves = 256;
	PlayerMove player_moves[max_moves];
	uint32_t player_move_count = 0;
	BodyPart body_parts[max_moves];
	uint32_t player_body_part_count = 0;

	glm::vec3 player_head_target_position;
	glm::vec3 player_head_target_direction;

	glm::mat4 player_matrices[max_moves] = {};

	// Current camera
	Renderer::Camera* current_camera;
	// Debug Camera
	Renderer::Camera* fly_camera;
	// Game Camera
	Renderer::Camera* look_at_camera;

	// Editor
	bool show_grid = true;
};

}

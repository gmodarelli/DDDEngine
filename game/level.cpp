#include "../game/level.h"
#include <stdio.h>

namespace Game
{

void Level::load_level(Game::State* game_state, const char* _path)
{
	// TODO: Replace these allocation with Memory::Stack.allocate.
	// It should be safe to use a Memory::Stack allocator here
	// cause the life time of these objects is the duration
	// of a single level.
	// The sizes of the table is set to an arbitrary 256 for now, but it
	// will be determined by the level data in the future.
	game_state->entities = new Renderer::Entity[256];
	game_state->transforms = new Renderer::Transform[256];

	float grid_size = 0.6f;
	uint32_t entity_index = 0;
	uint32_t transform_offset = 0;
	uint32_t player_length = 2;

	// NOTE: Initial player move.
	State::PlayerMove player_move = {};
	player_move.position = glm::vec3(0.0f, 0.0f, 0.0f);
	player_move.direction = glm::vec3(0.0f, 0.0f, 1.0f);
	player_move.orientation = glm::mat4(1.0f);
	game_state->player_moves[game_state->player_move_count++] = player_move;

	// Loading Entities
	//
	// Player Head
	assert(entity_index == 0);
	assert(entity_index == transform_offset);

	game_state->entities[entity_index] = {};
	game_state->entities[entity_index].model_id = 0; // Player Head
	memcpy(game_state->entities[entity_index].name, "Snake Head", 11);
	entity_index++;

	// Player Head transform
	State::BodyPart head = {};
	head.target_move_index = 0;
	head.position = player_move.position;
	head.direction = player_move.direction;
	head.orientation = player_move.orientation;
	game_state->body_parts[game_state->player_body_part_count++] = head;

	game_state->player_head_target_direction = player_move.direction;
	game_state->player_head_target_position = player_move.position + glm::vec3(grid_size) * player_move.direction;

	game_state->player_matrices[transform_offset] = glm::translate(glm::mat4(1.0f), player_move.position);
	game_state->transforms[transform_offset++] = { player_move.position, glm::vec3(1.0f) };
	assert(entity_index == transform_offset);

	// Player Tail
	game_state->entities[entity_index] = {};
	game_state->entities[entity_index].model_id = 2; // Player Tail
	memcpy(game_state->entities[entity_index].name, "Snake Tail", 11);
	entity_index++;

	// Player Tail transform
	glm::vec3 player_tail_position = glm::vec3(0.0f, 0.0f, -grid_size * (1 + player_length));
	State::BodyPart body_part = {};
	body_part.target_move_index = 0;
	body_part.position = player_tail_position;
	body_part.direction = player_move.direction;
	body_part.orientation = player_move.orientation;
	game_state->body_parts[game_state->player_body_part_count++] = body_part;

	game_state->player_matrices[transform_offset] = glm::translate(glm::mat4(1.0f), player_tail_position);
	game_state->transforms[transform_offset++] = { player_tail_position, glm::vec3(1.0f) };
	assert(entity_index == transform_offset);

	for (uint32_t i = 0; i < player_length; ++i)
	{
		// Player Body Part
		game_state->entities[entity_index] = {};
		game_state->entities[entity_index].model_id = 1; // Player Body
		memcpy(game_state->entities[entity_index].name, "Snake Body", 11);
		entity_index++;

		// Player Body transform
		glm::vec3 player_body_position = glm::vec3(0.0f, 0.0f, -grid_size * (1 + i));
		State::BodyPart body_part = {};
		body_part.target_move_index = 0;
		body_part.position = player_body_position;
		body_part.direction = player_move.direction;
		body_part.orientation = player_move.orientation;
		game_state->body_parts[game_state->player_body_part_count++] = body_part;

		game_state->player_matrices[transform_offset] = glm::translate(glm::mat4(1.0f), player_body_position);
		game_state->transforms[transform_offset++] = { player_body_position, glm::vec3(1.0f) };
		assert(entity_index == transform_offset);
	}

	// Ground
	game_state->entities[entity_index] = {};
	game_state->entities[entity_index].model_id = 4; // Ground Mesh
	memcpy(game_state->entities[entity_index].name, "Ground", 7);
	entity_index++;

	game_state->transforms[transform_offset++] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(20.0f, 1.0f, 20.0f) };
	assert(entity_index == transform_offset);

	// Wall Cubes
	for (uint32_t i = 1; i < 5; ++i)
	{
		game_state->entities[entity_index] = {};
		game_state->entities[entity_index].model_id = 3; // Cube Mesh
		char name[64];
		sprintf(name, "Wall %d", i);
		memcpy(game_state->entities[entity_index].name, name, 64);
		entity_index++;

		game_state->transforms[transform_offset++] = { glm::vec3(i * 0.6f, 0.3f, 0.0f), glm::vec3(1.0f) };
		assert(entity_index == transform_offset);
	}

	game_state->entity_count = entity_index;
	game_state->transform_count = transform_offset;
}

} // namespace Game
#include "../game/level.h"

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

	uint32_t entity_index = 0;
	uint32_t transform_offset = 0;

	// Loading Entities
	//
	// Player
	assert(entity_index == 0);
	assert(entity_index == transform_offset);

	game_state->entities[entity_index] = {};
	game_state->entities[entity_index].model_id = 0; // Cube Mesh
	const char* player_name = "Player";
	memcpy(game_state->entities[entity_index].name, player_name, 7);
	entity_index++;

	// Player transform
	game_state->transforms[transform_offset++] = { glm::vec4(0.0f), glm::vec4(1.0f) };
	assert(entity_index == transform_offset);

	game_state->entity_count = entity_index;
	game_state->transform_count = transform_offset;
}

} // namespace Game
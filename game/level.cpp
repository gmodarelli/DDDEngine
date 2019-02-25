#include "../game/level.h"

namespace Game
{

void Level::load_level(Game::State* game_state, const char* _path)
{
	// TODO: Replace these allocation with Memory::Stack.allocate.
	// It should be safe to use a Memory::Stack allocator here
	// cause the life time of these objects is the duration
	// of a single level.
	// The sizes of the arrays is "random" for now, but it
	// will be determined by the level data in the future.
	game_state->entities = new Renderer::Entity[256];
	game_state->transforms = new Renderer::Transform[256];
	game_state->static_entities = new Renderer::Entity[256];
	game_state->static_transforms = new Renderer::Transform[256];

	game_state->meshes = new Renderer::Primitive[16];
	game_state->vertices = new Renderer::Vertex[512];
	game_state->indices = new Renderer::Index[1024];

	uint32_t mesh_offset = 0;
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t entity_index = 0;
	uint32_t transform_offset = 0;
	uint32_t static_entity_index = 0;
	uint32_t static_transform_offset = 0;

	// Loading meshes
	//
	// Cube mesh
	game_state->meshes[mesh_offset] = {};
	game_state->meshes[mesh_offset].vertex_offset = vertex_offset;
	game_state->meshes[mesh_offset].index_offset = index_offset;
	game_state->meshes[mesh_offset].index_count = 36; // A cube has 36 indices
	mesh_offset++;
	// Cube vertices
	game_state->vertices[vertex_offset++] = { { -0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {1.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { { 0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { {	0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.3f, 0.3f, 0.3f}, {0.0f, 1.0f} };
	game_state->vertices[vertex_offset++] = { {	-0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 1.0f} };
	game_state->vertices[vertex_offset++] = { {	-0.3f, -0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { {	 0.3f, -0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {0.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { { 0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {0.0f, 1.0f} };
	game_state->vertices[vertex_offset++] = { {	-0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 0.0f }, {0.4f, 0.4f, 0.4f}, {1.0f, 1.0f} };
	// Cube indices
	// Front
	game_state->indices[index_offset++] = 0; game_state->indices[index_offset++] = 1; game_state->indices[index_offset++] = 2;
	game_state->indices[index_offset++] = 2; game_state->indices[index_offset++] = 3; game_state->indices[index_offset++] = 0;
	// Right
	game_state->indices[index_offset++] = 1; game_state->indices[index_offset++] = 5; game_state->indices[index_offset++] = 6;
	game_state->indices[index_offset++] = 6; game_state->indices[index_offset++] = 2; game_state->indices[index_offset++] = 1;
	// Back
	game_state->indices[index_offset++] = 7; game_state->indices[index_offset++] = 6; game_state->indices[index_offset++] = 5;
	game_state->indices[index_offset++] = 5; game_state->indices[index_offset++] = 4; game_state->indices[index_offset++] = 7;
	// Left
	game_state->indices[index_offset++] = 4; game_state->indices[index_offset++] = 0; game_state->indices[index_offset++] = 3;
	game_state->indices[index_offset++] = 3; game_state->indices[index_offset++] = 7; game_state->indices[index_offset++] = 4;
	// Bottom
	game_state->indices[index_offset++] = 4; game_state->indices[index_offset++] = 5; game_state->indices[index_offset++] = 1;
	game_state->indices[index_offset++] = 1; game_state->indices[index_offset++] = 0; game_state->indices[index_offset++] = 4;
	// Top
	game_state->indices[index_offset++] = 3; game_state->indices[index_offset++] = 2; game_state->indices[index_offset++] = 6;
	game_state->indices[index_offset++] = 6; game_state->indices[index_offset++] = 7; game_state->indices[index_offset++] = 3;
	//
	// Plane mesh
	game_state->meshes[mesh_offset] = {};
	game_state->meshes[mesh_offset].vertex_offset = vertex_offset;
	game_state->meshes[mesh_offset].index_offset = index_offset;
	game_state->meshes[mesh_offset].index_count = 6; // A plane has 6 indices
	mesh_offset++;
	// Plane vertices
	game_state->vertices[vertex_offset++] = { { -0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {1.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { { 0.3f, -0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {0.0f, 0.0f} };
	game_state->vertices[vertex_offset++] = { {	0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {0.0f, 1.0f} };
	game_state->vertices[vertex_offset++] = { {	-0.3f, 0.3f, 0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.36f, 0.03f }, {1.0f, 1.0f} };
	// Plane indices
	game_state->indices[index_offset++] = 0; game_state->indices[index_offset++] = 1; game_state->indices[index_offset++] = 2;
	game_state->indices[index_offset++] = 2; game_state->indices[index_offset++] = 3; game_state->indices[index_offset++] = 0;

	// Loading Entities
	//
	// Player
	assert(entity_index == transform_offset);
	game_state->entities[entity_index] = {};
	game_state->entities[entity_index].primitive_id = 0; // Cube Mesh
	const char* player_name = "Player";
	memcpy(game_state->entities[entity_index].name, player_name, 7);
	entity_index++;
	// Player transform
	game_state->transforms[transform_offset++] = { glm::vec3(0.0f, 0.6f, 0.0f), glm::vec3(1.0f) };
	assert(entity_index == transform_offset);

	uint32_t board_width = 20;
	uint32_t board_height = 20;
	float box_width = 0.6f;

	//
	// Ground
	assert(static_entity_index == static_transform_offset);
	game_state->static_entities[static_entity_index] = {};
	game_state->static_entities[static_entity_index].primitive_id = 1; // Plane Mesh
	const char* ground_name = "Ground";
	memcpy(game_state->static_entities[static_entity_index].name, ground_name, 7);
	static_entity_index++;
	// Ground transform
	game_state->static_transforms[static_transform_offset++] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(board_width, 1.0f, board_height) , glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) };
	assert(static_entity_index == static_transform_offset);

	// Walls
	glm::vec3 scale(1.0f, 1.0f, 1.0f);
	for (uint32_t c = 0; c < board_height; ++c)
	{
		for (uint32_t r = 0; r < board_width; ++r)
		{
			if ((c == 0 || c == board_height - 1) || ((r == 0 || r == board_width - 1)))
			{
				assert(static_entity_index == static_transform_offset);
				game_state->static_entities[static_entity_index] = {};
				game_state->static_entities[static_entity_index].primitive_id = 0; // Cube Mesh
				const char* name = "Box";
				memcpy(game_state->static_entities[static_entity_index].name, name, 4);
				static_entity_index++;
				game_state->static_transforms[static_transform_offset++] = { glm::vec3(r * box_width, box_width, c * box_width), scale };
				assert(static_entity_index == static_transform_offset);
			}
		}
	}

	game_state->mesh_count = mesh_offset;
	game_state->vertex_count = vertex_offset;
	game_state->index_count = index_offset;
	game_state->entity_count = entity_index;
	game_state->transform_count = transform_offset;
	game_state->static_entity_count = static_entity_index;
	game_state->static_transform_count = static_transform_offset;
}

} // namespace Game
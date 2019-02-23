#pragma once
#include "../renderer/types.h"

namespace Game
{

struct State
{
	// Player info
	Renderer::Transform player_transform;
	glm::vec3 player_direction = glm::vec3(0.0f, 0.0f, 0.0f);
	float player_speed = 2.0f;
};

}

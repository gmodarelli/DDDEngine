#pragma once

#include "../game/state.h"
#include "../resources/resources.h"

namespace Game
{

struct Level
{
	static void load_level(Game::State* game_state, const char* _path);
};

} // namespace Game

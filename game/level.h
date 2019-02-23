#pragma once

#include "../game/state.h"

namespace Game
{

struct Level
{
	static void load_level(Game::State* game_state, const char* path);
};

} // namespace Game

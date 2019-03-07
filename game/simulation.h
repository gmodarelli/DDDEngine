#pragma once

#include "../application/platform.h"
#include "../game/state.h"

namespace Game
{

struct Simulation
{
	Simulation(Application::Platform* platform);
	void init();
	void cleanup();

	void update(Game::State* game_state);

	Application::Platform* platform;
};

}

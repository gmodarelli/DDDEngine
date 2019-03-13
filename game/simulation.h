#pragma once

#include "../application/platform.h"
#include "../renderer/renderer.h"
#include "../game/state.h"

namespace Game
{

struct Simulation
{
	Simulation(Application::Platform* platform, Renderer::Renderer* renderer);
	void init();
	void cleanup();

	void update(Game::State* game_state, uint32_t simulation_frame_index);

	Application::Platform* platform;
	Renderer::Renderer* renderer;
};

}

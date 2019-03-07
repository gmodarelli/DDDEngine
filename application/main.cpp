#include <stdio.h>
#include <chrono>
#include <cassert>

#include "../memory/stack.h"
#include "../application/platform.h"
#include "../renderer/renderer.h"
#include "../renderer/camera.h"
#include "../game/simulation.h"
#include "../game/level.h"
#include "../resources/loader.h"

int main()
{
	Application::Platform* platform = new Application::Platform();
	platform->init("73 Games", 1600, 1200);

	Renderer::Renderer* renderer = new Renderer::Renderer(platform);
	renderer->init();

	Game::Simulation* simulation = new Game::Simulation(platform);
	simulation->init();

	Game::State* game_state = new Game::State();
	game_state->fly_camera = new Renderer::Camera(Renderer::CameraType::Fly, glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	game_state->look_at_camera = new Renderer::Camera(Renderer::CameraType::LookAt, glm::vec3(0.0f, 10.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	game_state->current_camera = game_state->look_at_camera;

	// Level preparation
	Resources::AssetsInfo* assets_info = new Resources::AssetsInfo();
	uint32_t snake_head_id = Resources::Loader::load_model("../data/models/snake_head.glb", "snake_head", assets_info);
	uint32_t snake_body_id = Resources::Loader::load_model("../data/models/snake_body.glb", "snake_body", assets_info);
	uint32_t snake_tail_id = Resources::Loader::load_model("../data/models/snake_tail.glb", "snake_tail", assets_info);
	uint32_t wall_id = Resources::Loader::load_model("../data/models/wall.glb", "wall", assets_info);
	uint32_t ground_id = Resources::Loader::load_model("../data/models/ground.glb", "ground", assets_info);
	game_state->assets_info = assets_info;
 
	// Load a level data
	// TODO: Eventually from file
	Game::Level::load_level(game_state, "");
	// Upload vertices and indices data to the GPU
	renderer->upload_buffers(game_state);
	// Upload nodes UBOs data to the GPU
	renderer->upload_dynamic_uniform_buffers(game_state);

	// 
	const uint32_t ticks_per_second = 25;
	const uint32_t skip_ticks = 1000 / ticks_per_second;
	const uint32_t max_frame_skip = 5;

	double next_game_tick = glfwGetTime() * 1000.0f;
	uint32_t loops;
	float interpolation;

	while (platform->alive())
	{
		loops = 0;
		while (glfwGetTime() * 1000.0f > next_game_tick && loops < max_frame_skip)
		{
			simulation->update(game_state);

			next_game_tick += skip_ticks;
			loops++;
		}

		interpolation = float(glfwGetTime() * 1000.0f + skip_ticks - next_game_tick) / float(skip_ticks);
		renderer->render_frame(game_state, interpolation);
	}

	simulation->cleanup();
	renderer->cleanup();
	platform->cleanup();

	return 0;
}

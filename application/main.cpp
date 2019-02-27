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
	game_state->look_at_camera = new Renderer::Camera(Renderer::CameraType::LookAt, glm::vec3(0.0f, 20.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	game_state->current_camera = game_state->look_at_camera;

	// Level preparation
	Resources::AssetsInfo* assets_info = new Resources::AssetsInfo();
	uint32_t snake_head_id = Resources::Loader::load_model("../data/models/snake_head.glb", "snake_head", assets_info);
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

	char stats[256];
	auto start_time = std::chrono::high_resolution_clock::now();

	while (platform->alive())
	{
		auto current_time = std::chrono::high_resolution_clock::now();
		float delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		simulation->update(game_state, delta_time);
		renderer->render_frame(game_state, delta_time);

		sprintf(stats, "CPU: %.4f ms - GPU: %.4f ms", renderer->frame_cpu_avg, renderer->backend->device->frame_gpu_avg);
		platform->set_window_title(stats);

		start_time = current_time;
	}

	simulation->cleanup();
	renderer->cleanup();
	platform->cleanup();

	return 0;
}

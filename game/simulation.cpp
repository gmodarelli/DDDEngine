#include "simulation.h"

namespace Game
{

Simulation::Simulation(Application::Platform* platform) : platform(platform)
{
}

void Simulation::init()
{
}

void Simulation::update(Game::State* game_state, float delta_time)
{
	Application::InputState input_state = platform->get_input_state();

	if (input_state.key_up_pressed)
	{
		game_state->player_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	if (input_state.key_down_pressed)
	{
		game_state->player_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	if (input_state.key_left_pressed)
	{
		game_state->player_direction = glm::vec3(-1.0f, 0.0f, 0.0f);
	}

	if (input_state.key_right_pressed)
	{
		game_state->player_direction = glm::vec3(1.0f, 0.0f, 0.0f);
	}

	game_state->transforms[game_state->player_entity_id].position += game_state->player_direction * delta_time * game_state->player_speed;
}

}
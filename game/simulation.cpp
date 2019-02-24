#include "simulation.h"

namespace Game
{

Simulation::Simulation(Application::Platform* platform) : platform(platform)
{
}

void Simulation::init()
{
}

void Simulation::cleanup()
{
}

void Simulation::update(Game::State* game_state, float delta_time)
{
	Application::InputState input_state = platform->get_input_state();

	if (input_state.key_up)
	{
		game_state->player_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	if (input_state.key_down)
	{
		game_state->player_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	}

	if (input_state.key_left)
	{
		game_state->player_direction = glm::vec3(-1.0f, 0.0f, 0.0f);
	}

	if (input_state.key_right)
	{
		game_state->player_direction = glm::vec3(1.0f, 0.0f, 0.0f);
	}

	game_state->transforms[game_state->player_entity_id].position += game_state->player_direction * delta_time * game_state->player_speed;

	// Handle Debug Camera Movements
	if (input_state.key_w)
	{
		game_state->camera_position += game_state->camera_front * delta_time * game_state->camera_speed;
	}

	if (input_state.key_s)
	{
		game_state->camera_position -= game_state->camera_front * delta_time * game_state->camera_speed;
	}

	if (input_state.key_a)
	{
		game_state->camera_position -= glm::normalize(glm::cross(game_state->camera_front, game_state->camera_up)) * delta_time * game_state->camera_speed;
	}

	if (input_state.key_d)
	{
		game_state->camera_position += glm::normalize(glm::cross(game_state->camera_front, game_state->camera_up)) * delta_time * game_state->camera_speed;
	}

	// Handle Debug Camera Rotations
	static float mouse_last_x = 1600 / 2.0f;
	static float mouse_last_y = 1200 / 2.0f;
	static bool first_mouse = true;

	if (first_mouse)
	{
		mouse_last_x = input_state.cursor_position_x;
		mouse_last_y = input_state.cursor_position_y;
		first_mouse = false;
	}

	float x_offest = (input_state.cursor_position_x - mouse_last_x);
	float y_offest = (mouse_last_y - input_state.cursor_position_y);
	mouse_last_x = input_state.cursor_position_x;
	mouse_last_y = input_state.cursor_position_y;

	if (input_state.mouse_btn_left)
	{
		x_offest *= game_state->camera_mouse_sensitivity;
		y_offest *= game_state->camera_mouse_sensitivity;

		game_state->camera_yaw += x_offest;
		game_state->camera_pitch += y_offest;

		if (game_state->camera_pitch > 89.0f)
			game_state->camera_pitch = 89.0f;
		if (game_state->camera_pitch < -89.0f)
			game_state->camera_pitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(game_state->camera_yaw)) * cos(glm::radians(game_state->camera_pitch));
		front.y = sin(glm::radians(game_state->camera_pitch));
		front.z = sin(glm::radians(game_state->camera_yaw)) * cos(glm::radians(game_state->camera_pitch));
		game_state->camera_front = front;
	}

	game_state->camera_view = glm::lookAt(game_state->camera_position, game_state->camera_position + game_state->camera_front, game_state->camera_up);
}

}
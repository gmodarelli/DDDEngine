#include "simulation.h"
#include <stdio.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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

	if (game_state->player_rotating == false)
	{
		// Player Movement
		if (input_state.key_up)
		{
			game_state->player_target_direction = glm::vec3(0.0f, 0.0f, -1.0f);
		}

		if (input_state.key_down)
		{
			game_state->player_target_direction = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		if (input_state.key_left)
		{
			game_state->player_target_direction = glm::vec3(-1.0f, 0.0f, 0.0f);
		}

		if (input_state.key_right)
		{
			game_state->player_target_direction = glm::vec3(1.0f, 0.0f, 0.0f);
		}
	}

	if (game_state->player_direction != game_state->player_target_direction)
	{
		float dot = glm::dot(game_state->player_direction, game_state->player_target_direction);

		// NOTE: Cannot turn to face the opposite direction. The snake can only turn by 90
		// degrees left or right
		if (dot == -1.0f)
		{
			game_state->player_target_direction = game_state->player_direction;
		}
		else
		{
			glm::vec3 player_position = game_state->transforms[game_state->player_entity_id].position;
			game_state->player_orientation = glm::rotate(game_state->player_orientation, acosf(dot), glm::vec3(0.0f, 1.0f, 0.0f));
			game_state->player_direction = game_state->player_target_direction;
		}
	}

	game_state->transforms[game_state->player_entity_id].position += game_state->player_direction * delta_time * game_state->player_speed;
	game_state->transforms[game_state->player_entity_id + 1].position += game_state->player_direction * delta_time * game_state->player_speed;
	game_state->transforms[game_state->player_entity_id + 2].position += game_state->player_direction * delta_time * game_state->player_speed;

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, game_state->transforms[game_state->player_entity_id].position);
	model *= game_state->player_orientation;

	game_state->player_matrices[0] = model;


	// Swith cameras
	if (input_state.key_1)
	{
		game_state->current_camera = game_state->look_at_camera;
	}

	if (input_state.key_2)
	{
		game_state->current_camera = game_state->fly_camera;
	}

	// Toggle grid
	if (input_state.key_g)
	{
		game_state->show_grid = !game_state->show_grid;
	}

	// Handle Debug Camera Movements
	if (game_state->current_camera == game_state->fly_camera)
	{
		if (input_state.key_w)
		{
			game_state->current_camera->position += game_state->current_camera->front * delta_time * game_state->current_camera->speed;
		}

		if (input_state.key_s)
		{
			game_state->current_camera->position -= game_state->current_camera->front * delta_time * game_state->current_camera->speed;
		}

		if (input_state.key_a)
		{
			game_state->current_camera->position -= glm::normalize(glm::cross(game_state->current_camera->front, game_state->current_camera->up)) * delta_time * game_state->current_camera->speed;
		}

		if (input_state.key_d)
		{
			game_state->current_camera->position += glm::normalize(glm::cross(game_state->current_camera->front, game_state->current_camera->up)) * delta_time * game_state->current_camera->speed;
		}
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

	if (input_state.mouse_btn_right)
	{
		if (game_state->current_camera == game_state->fly_camera)
		{
			x_offest *= game_state->current_camera->sensitivity;
			y_offest *= game_state->current_camera->sensitivity;

			game_state->current_camera->yaw += x_offest;
			game_state->current_camera->pitch += y_offest;

			if (game_state->current_camera->pitch > 89.0f)
				game_state->current_camera->pitch = 89.0f;
			if (game_state->current_camera->pitch < -89.0f)
				game_state->current_camera->pitch = -89.0f;

			glm::vec3 front;
			front.x = cos(glm::radians(game_state->current_camera->yaw)) * cos(glm::radians(game_state->current_camera->yaw));
			front.y = sin(glm::radians(game_state->current_camera->pitch));
			front.z = sin(glm::radians(game_state->current_camera->yaw)) * cos(glm::radians(game_state->current_camera->pitch));
			game_state->current_camera->front = front;
		}
	}

	game_state->current_camera->update_view();
}

}
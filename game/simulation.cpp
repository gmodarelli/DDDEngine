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

void Simulation::update(Game::State* game_state, uint32_t simulation_frame_index)
{
	Application::InputState input_state = platform->get_input_state();

	// Player Movement

	State::BodyPart& head = game_state->body_parts[game_state->player_head_id];
	float head_target_distance = glm::distance(game_state->player_head_target_position, head.position);
	// if (head_target_distance <= 0.2f)
	if (head.direction == game_state->player_head_target_direction)
	{
		if (input_state.key_up)
		{
			game_state->player_head_target_direction = glm::vec3(0.0f, 0.0f, -1.0f);
		}

		if (input_state.key_down)
		{
			game_state->player_head_target_direction = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		if (input_state.key_left)
		{
			game_state->player_head_target_direction = glm::vec3(-1.0f, 0.0f, 0.0f);
		}

		if (input_state.key_right)
		{
			game_state->player_head_target_direction = glm::vec3(1.0f, 0.0f, 0.0f);
		}
	}

	// HEAD
	if (head_target_distance - game_state->player_speed >= 0.0001f)
	{
		head.position += head.direction * game_state->player_speed;
		game_state->transforms[game_state->player_head_id].position = head.position;
	}
	else
	{
		head.position = game_state->player_head_target_position;
		game_state->transforms[game_state->player_head_id].position = head.position;

		if (head.direction != game_state->player_head_target_direction)
		{
			float dot = glm::dot(head.direction, game_state->player_head_target_direction);

			// NOTE: Cannot turn to face the opposite direction. The snake can only turn by 90
			// degrees left or right
			if (dot == -1.0f)
			{
				game_state->player_head_target_direction = head.direction;
			}
			else
			{
				// NOTE: acosf returns a value between 0 and PI radians, so we don't have information
				// about the direction of the rotation. That's why we use atan2f instead.
				float tetha = atan2f(head.direction.z, game_state->player_head_target_direction.z) - atan2f(head.direction.x, game_state->player_head_target_direction.x);
				head.orientation = glm::rotate(head.orientation, tetha, glm::vec3(0.0f, 1.0f, 0.0f));
				head.direction = game_state->player_head_target_direction;
			}
		}

		State::PlayerMove player_move = {};
		player_move.position = head.position;
		player_move.direction = head.direction;
		player_move.orientation = head.orientation;

		game_state->player_moves[game_state->player_move_count++ % game_state->max_moves] = player_move;

		game_state->player_head_target_position = head.position + glm::vec3(0.6f) * head.direction;
	}

	for (uint32_t i = game_state->player_tail_id; i < game_state->player_body_part_count; ++i)
	{
		State::BodyPart& body_part = game_state->body_parts[i];
		State::PlayerMove& target_move = game_state->player_moves[body_part.target_move_index % game_state->max_moves];
		float target_distance = glm::distance(target_move.position, body_part.position);

		if (target_distance - game_state->player_speed >= 0.0001f)
		{
			body_part.position += body_part.direction * game_state->player_speed;
		}
		else
		{
			body_part.position = target_move.position;
			body_part.direction = target_move.direction;
			body_part.orientation = target_move.orientation;
			body_part.target_move_index++;
		}

		game_state->transforms[i + 1].position = body_part.position;
	}

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
			game_state->current_camera->position += game_state->current_camera->front * game_state->current_camera->speed;
		}

		if (input_state.key_s)
		{
			game_state->current_camera->position -= game_state->current_camera->front * game_state->current_camera->speed;
		}

		if (input_state.key_a)
		{
			game_state->current_camera->position -= glm::normalize(glm::cross(game_state->current_camera->front, game_state->current_camera->up)) * game_state->current_camera->speed;
		}

		if (input_state.key_d)
		{
			game_state->current_camera->position += glm::normalize(glm::cross(game_state->current_camera->front, game_state->current_camera->up)) * game_state->current_camera->speed;
		}
	}

	// Handle Debug Camera Rotations
	// TODO: Take the width and height from the window
	static float mouse_last_x = 1600 / 2.0f;
	static float mouse_last_y = 1200 / 2.0f;
	static bool first_mouse = true;

	if (first_mouse)
	{
		mouse_last_x = (float)input_state.cursor_position_x;
		mouse_last_y = (float)input_state.cursor_position_y;
		first_mouse = false;
	}

	float x_offest = ((float)input_state.cursor_position_x - mouse_last_x);
	float y_offest = (mouse_last_y - (float)input_state.cursor_position_y);
	mouse_last_x = (float)input_state.cursor_position_x;
	mouse_last_y = (float)input_state.cursor_position_y;

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
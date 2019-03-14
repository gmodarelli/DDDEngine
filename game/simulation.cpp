#include "simulation.h"
#include <stdio.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Game
{

Simulation::Simulation(Application::Platform* platform, Renderer::Renderer* renderer) : platform(platform), renderer(renderer)
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

	if (!game_state->paused)
	{
		// Player Movement

		State::BodyPart& head = game_state->body_parts[0];
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
		float distance = glm::distance(game_state->player_head_target_position, head.position);
		if (distance - game_state->player_speed >= 0.0001f)
		{
			head.position += head.direction * game_state->player_speed;
		}
		else
		{
			head.position = game_state->player_head_target_position;

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


			// Check for collisions
			// printf("\nChecking for collisions");
			for (uint32_t i = 1; i < game_state->player_body_part_count; ++i)
			{
				State::BodyPart& body_part = game_state->body_parts[i];
				State::PlayerMove& target_move = game_state->player_moves[body_part.target_move_index];
				float distance = glm::distance(game_state->player_head_target_position, target_move.position);
				if (distance < 0.001f)
				{
					// printf("\n%d: %.4f", i, distance);
					printf("\nCollided with body part at index %d", i);
				}
			}

			if (game_state->growing)
			{
				game_state->growing = false;
			}

			if (game_state->queued_growing)
			{
				game_state->queued_growing = false;
				game_state->growing = true;

				// Player Body Part
				game_state->entities[game_state->entity_count] = {};
				game_state->entities[game_state->entity_count].model_id = 1; // Player Body
				memcpy(game_state->entities[game_state->entity_count].name, "Snake Body", 11);
				game_state->entity_count++;

				// Player Body transform
				// TODO: Replace the hardcoded 0.6f
				glm::vec3 player_body_position = head.position - (glm::vec3(0.6f) * head.direction);
				State::BodyPart body_part = {};
				body_part.target_move_index = (game_state->player_move_count - 1) % game_state->max_moves;
				body_part.position = player_body_position;
				body_part.direction = head.direction;
				body_part.orientation = head.orientation;
				game_state->body_parts[game_state->player_body_part_count] = body_part;

				game_state->player_matrices[game_state->player_body_part_count++] = glm::translate(glm::mat4(1.0f), player_body_position);
				// assert(game_state->entity_count == ++transform_offset);

				renderer->upload_dynamic_uniform_buffers(game_state, game_state->entity_count - 1, game_state->entity_count);
			}
		}

		// Check for collisions with apple
		if (!game_state->queued_growing && !game_state->growing)
		{
			Renderer::Transform& apple_transform = game_state->transforms[game_state->apple_id];
			float apple_distance = glm::distance(head.position, apple_transform.position);
			if (apple_distance <= game_state->player_speed)
			{
				game_state->queued_growing = true;
				// TODO: Move to a random, free, square on the grid
				apple_transform.position += 1.2f * glm::vec3(1.0f, 0.0f, 1.0f);
			}
		}

		uint32_t player_body_offset = 1;
		if (game_state->growing)
		{
			player_body_offset = game_state->player_body_part_count - 1;
		}

		for (uint32_t i = player_body_offset; i < game_state->player_body_part_count; ++i)
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
		}

		if (input_state.key_space)
		{
			if (!game_state->queued_growing && !game_state->growing)
			{
				game_state->queued_growing = true;
			}
		}
	}

	// Toggle pause
	if (input_state.key_p)
	{
		game_state->paused = !game_state->paused;
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
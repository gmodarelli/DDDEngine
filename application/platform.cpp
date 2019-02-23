#include "platform.h"

#include <stdio.h>
#include <cassert>

void Platform::init(const char* title, uint32_t width, uint32_t height)
{
#ifdef SNAKE_USE_GLFW
	glfwInit();

	// GLFW was designed to work with OpenGL so we need to tell it not to
	// create an OpenGL context, otherwise we won't be able to create a 
	// VkSwapchainKHR later.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	const char* error;
	glfwGetError(&error);
	if (error)
	{
		printf("[WSI]: %s\n", error);
		assert(!"Failed to initialize GLFW");
	}

	window_parameters.title = new char;
	sprintf(window_parameters.title, "%s", title);
	window_parameters.window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (window_parameters.window == NULL)
	{
		glfwGetError(&error);
		if (error)
		{
			printf("[WSI]: %s\n", error);
			assert(false);
		}
	}
	glfwSetWindowUserPointer(window_parameters.window, this);

	glfwSetKeyCallback(window_parameters.window, key_callback);
	glfwSetFramebufferSizeCallback(window_parameters.window, framebuffer_resize_callback);
#endif
}


bool Platform::alive()
{
#ifdef SNAKE_USE_GLFW
	glfwPollEvents();
	return !glfwWindowShouldClose(window_parameters.window);
#endif
}

void Platform::cleanup()
{
	glfwDestroyWindow(window_parameters.window);
}

void Platform::set_window_title(const char* title)
{
#ifdef SNAKE_USE_GLFW
	char buffer[256];
	sprintf(buffer, "%s: %s", window_parameters.title, title);
	glfwSetWindowTitle(window_parameters.window, buffer);
#endif
}

InputState Platform::get_input_state() const
{
	return input_state;
}

#ifdef SNAKE_USE_GLFW
static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	printf("\n Resize callback fired\n");
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Platform* platform = (Platform*)glfwGetWindowUserPointer(window);

	if ((key == GLFW_KEY_W || key == GLFW_KEY_UP))
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_up_pressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_up_pressed = false;
		}
	}

	if ((key == GLFW_KEY_S || key == GLFW_KEY_DOWN))
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_down_pressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_down_pressed = false;
		}
	}

	if ((key == GLFW_KEY_A || key == GLFW_KEY_LEFT))
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_left_pressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_left_pressed = false;
		}
	}

	if ((key == GLFW_KEY_D || key == GLFW_KEY_RIGHT))
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_right_pressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_right_pressed = false;
		}
	}
}
#endif

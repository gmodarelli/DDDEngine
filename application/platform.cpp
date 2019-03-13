#include "platform.h"

#include <stdio.h>
#include <cassert>

namespace Application
{

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

	glfwSetInputMode(window_parameters.window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetInputMode(window_parameters.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glfwSetKeyCallback(window_parameters.window, key_callback);
	glfwSetCursorEnterCallback(window_parameters.window, cursor_enter_callback);
	glfwSetCursorPosCallback(window_parameters.window, cursor_position_callback);
	glfwSetScrollCallback(window_parameters.window, scroll_callback);
	glfwSetMouseButtonCallback(window_parameters.window, mouse_button_callback);
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

void* Platform::allocate(size_t size)
{
#ifdef _WIN32
	void* base_address = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(base_address);
	return base_address;
#endif
}

void Platform::free(void* base_address, size_t size)
{
#ifdef _WIN32
	VirtualFree(base_address, size, MEM_RELEASE);
#endif
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

static void cursor_enter_callback(GLFWwindow* window, int entered)
{
	Platform* platform = (Platform*)glfwGetWindowUserPointer(window);

	if (entered)
	{
		platform->input_state.cursor_entered = true;
	}
	else
	{
		platform->input_state.cursor_entered = false;
	}
}

static void cursor_position_callback(GLFWwindow* window, double x_position, double y_position)
{
	Platform* platform = (Platform*)glfwGetWindowUserPointer(window);

	platform->input_state.cursor_position_x = x_position;
	platform->input_state.cursor_position_y = y_position;
}

static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
	printf("\nscroll(%.2f, %.2f)", x_offset, y_offset);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

	Platform* platform = (Platform*)glfwGetWindowUserPointer(window);

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.mouse_btn_left = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.mouse_btn_left = false;
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.mouse_btn_right = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.mouse_btn_right = false;
		}
	}

	if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.mouse_btn_middle = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.mouse_btn_middle = false;
		}
	}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Platform* platform = (Platform*)glfwGetWindowUserPointer(window);

	if (key == GLFW_KEY_UP)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_up = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_up = false;
		}
	}

	if (key == GLFW_KEY_DOWN)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_down = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_down = false;
		}
	}

	if (key == GLFW_KEY_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_left = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_left = false;
		}
	}

	if (key == GLFW_KEY_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_right = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_right = false;
		}
	}

	if (key == GLFW_KEY_W)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_w = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_w = false;
		}
	}

	if (key == GLFW_KEY_S)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_s = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_s = false;
		}
	}

	if (key == GLFW_KEY_A)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_a = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_a = false;
		}
	}

	if (key == GLFW_KEY_D)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_d = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_d = false;
		}
	}

	if (key == GLFW_KEY_G)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_g = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_g = false;
		}
	}

	if (key == GLFW_KEY_SPACE)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_space = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_space = false;
		}
	}

	if (key == GLFW_KEY_1)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_1 = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_1 = false;
		}
	}

	if (key == GLFW_KEY_2)
	{
		if (action == GLFW_PRESS)
		{
			platform->input_state.key_2 = true;
		}
		else if (action == GLFW_RELEASE)
		{
			platform->input_state.key_2 = false;
		}
	}
}
#endif

} // namespace Application

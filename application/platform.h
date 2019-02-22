#pragma once

#ifdef SNAKE_USE_GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

#ifdef SNAKE_USE_GLFW
static void framebuffer_resize_callback(GLFWwindow* window, int window_width, int window_height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
#endif

struct Platform
{
	struct WindowParameters
	{
		char* title;
#ifdef SNAKE_USE_GLFW
		GLFWwindow* window;
#endif
	};

	void init(const char* title, uint32_t width, uint32_t height, void* user_pointer);
	void cleanup();
	bool alive();
	void set_window_title(const char* title);

	WindowParameters window_parameters;

};

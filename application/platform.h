#pragma once

#ifdef SNAKE_USE_GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace Application
{

struct InputState
{
	// Mouse Input
	bool mouse_btn_left = false;
	bool mouse_btn_right = false;
	bool mouse_btn_middle = false;

	// Cursor Input
	bool cursor_entered = false;
	double cursor_position_x = 0.0;
	double cursor_position_y = 0.0;

	// Keyboard Input
	bool key_up = false;
	bool key_down = false;
	bool key_left = false;
	bool key_right = false;
	bool key_w = false;
	bool key_s = false;
	bool key_a = false;
	bool key_d = false;
	bool key_g = false;
	bool key_space = false;

	bool key_1 = false;
	bool key_2 = false;
};
	
struct Platform
{
	struct WindowParameters
	{
		char* title;
#ifdef SNAKE_USE_GLFW
		GLFWwindow* window;
#endif
	};

	void init(const char* title, uint32_t width, uint32_t height);
	void cleanup();
	bool alive();
	void set_window_title(const char* title);

	void* allocate(size_t size);
	void free(void* base_address, size_t size);

	InputState get_input_state() const;

	WindowParameters window_parameters;
	InputState input_state;
};

#ifdef SNAKE_USE_GLFW
static void framebuffer_resize_callback(GLFWwindow* window, int window_width, int window_height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void cursor_enter_callback(GLFWwindow* window, int entered);
static void cursor_position_callback(GLFWwindow* window, double x_position, double y_position);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void scroll_callback(GLFWwindow* window, double x_offset, double y_offset);
#endif

} // namespace Application
#pragma once

#ifdef _WIN32
#include <Windows.h>
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

struct WindowParameters
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
	HINSTANCE Hinstance;
	HWND HWnd;
#endif

	uint32_t width;
	uint32_t height;
};

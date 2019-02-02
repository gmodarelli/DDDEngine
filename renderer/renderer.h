#pragma once

#include "../vulkan/wsi.h"

namespace Renderer
{

struct Renderer
{
	Renderer(Vulkan::WSI* wsi);

	void init();
	void cleanup();

	void render();

private:
	// WSI
	Vulkan::WSI* wsi;

}; // struct Renderer

} // namespace Renderer

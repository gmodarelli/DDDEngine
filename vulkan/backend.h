#pragma once

#include "wsi.h"
#include "device.h"

namespace Vulkan
{

struct Backend
{
	Vulkan::WSI* wsi;
	Vulkan::Device* device;
};

}

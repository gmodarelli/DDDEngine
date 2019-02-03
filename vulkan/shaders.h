#pragma once

#include "context.h"

namespace Vulkan
{

struct Shader
{
	static VkPipelineShaderStageCreateInfo load_shader(VkDevice device, VkPhysicalDevice gpu, const char* shaderPath, VkShaderStageFlagBits stage, const char* entrypoint = "main");
};

} // namespace Vulkan

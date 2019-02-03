#include "shaders.h"

#include <stdio.h>
#include <cassert>

namespace Vulkan
{

VkPipelineShaderStageCreateInfo Shader::load_shader(VkDevice device, VkPhysicalDevice gpu, const char* shaderPath, VkShaderStageFlagBits stage, const char* entrypoint)
{
	FILE* fileHandle = NULL;
	fileHandle = fopen(shaderPath, "rb");
	assert(fileHandle != NULL && "Could not load the shader file");

	// Compute the file size
	fseek(fileHandle, 0, SEEK_END);
	size_t size = ftell(fileHandle);
	rewind(fileHandle);

	// Make sure size is a multiple of 4
	size_t aligned_size = (size + 3) & -4;

	// Allocate a buffer to hold the file content and read the file
	char* buffer = new char[aligned_size];
	uint32_t bytes_read = fread(buffer, sizeof(buffer[0]), aligned_size, fileHandle);
	assert(bytes_read == size);

	VkShaderModuleCreateInfo shader_module_create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	shader_module_create_info.codeSize = aligned_size;
	shader_module_create_info.pCode = (uint32_t*)buffer;

	VkShaderModule shader_module;
	VkResult result = vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module);
	assert(result == VK_SUCCESS);

	VkPipelineShaderStageCreateInfo stage_create_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stage_create_info.stage = stage;
	stage_create_info.module = shader_module;
	stage_create_info.pName = entrypoint;

	return stage_create_info;
}

} // namespace Vulkan

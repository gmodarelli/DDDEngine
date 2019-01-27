#pragma once

#include <volk.h>
#include <stdio.h>

#include "utils.h"

namespace Vulkan
{
	void readCode(const char* path, char* outCode, uint32_t* outCodeSize)
	{
		FILE* fileHandle = NULL;
		fileHandle = fopen(path, "rb");
		printf("Trying to open: %s\n", path);
		GM_ASSERT(fileHandle != NULL && "Could not find the file");
		*outCodeSize = static_cast<uint32_t>(fread(outCode, 1, 10000, fileHandle));
		fclose(fileHandle);
		fileHandle = NULL;
	}

	void setupShader(VkDevice device, VkPhysicalDevice physicalDevice, const char* vertShaderPath, VkShaderModule* outVertShaderModule, const char* fragShaderPath, VkShaderModule* outFragShaderModule)
	{
		// Simple Vertex and Fragment Shaders

		uint32_t codeSize = 0;
		// FACEPALM: Assume your file is less than 10000 bytes
		char* code = new char[10000];
		readCode(vertShaderPath, code, &codeSize);

		VkShaderModuleCreateInfo vertexShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		vertexShaderCreateInfo.codeSize = codeSize;
		vertexShaderCreateInfo.pCode = (uint32_t*)code;

		GM_CHECK(vkCreateShaderModule(device, &vertexShaderCreateInfo, nullptr, outVertShaderModule), "");

		readCode(fragShaderPath, code, &codeSize);

		VkShaderModuleCreateInfo fragmentShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		fragmentShaderCreateInfo.codeSize = codeSize;
		fragmentShaderCreateInfo.pCode = (uint32_t*)code;

		GM_CHECK(vkCreateShaderModule(device, &fragmentShaderCreateInfo, nullptr, outFragShaderModule), "");

		delete[] code;
		code = NULL;
	}
}

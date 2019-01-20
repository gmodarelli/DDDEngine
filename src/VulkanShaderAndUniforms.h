#ifndef VULKAN_SHADER_AND_UNIFORMS_H_
#define VULKAN_SHADER_AND_UNIFORMS_H_

void ReadCode(const char* path, char* outCode, uint32_t* outCodeSize)
{
	FILE* fileHandle = NULL;
	fileHandle = fopen(path, "rb");
	printf("Trying to open: %s\n", path);
	R_ASSERT(fileHandle != NULL && "Could not find the file");
	*outCodeSize = static_cast<uint32_t>(fread(outCode, 1, 10000, fileHandle));
	fclose(fileHandle);
	fileHandle = NULL;
}

void SetupShader(VkDevice device, VkPhysicalDevice physicalDevice, const char* vertShaderPath, VkShaderModule* outVertShaderModule, const char* fragShaderPath, VkShaderModule* outFragShaderModule)
{
	// Simple Vertex and Fragment Shaders

	uint32_t codeSize = 0;
	// FACEPALM: Assume your file is less than 10000 bytes
	char* code = new char[10000];
	// ReadCode("../data/shaders/vert.spv", code, &codeSize);
	ReadCode(vertShaderPath, code, &codeSize);

	VkShaderModuleCreateInfo vertexShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertexShaderCreateInfo.codeSize = codeSize;
	vertexShaderCreateInfo.pCode = (uint32_t*)code;

	VK_CHECK(vkCreateShaderModule(device, &vertexShaderCreateInfo, nullptr, outVertShaderModule));

	// ReadCode("../data/shaders/frag.spv", code, &codeSize);
	ReadCode(fragShaderPath, code, &codeSize);

	VkShaderModuleCreateInfo fragmentShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragmentShaderCreateInfo.codeSize = codeSize;
	fragmentShaderCreateInfo.pCode = (uint32_t*)code;

	VK_CHECK(vkCreateShaderModule(device, &fragmentShaderCreateInfo, nullptr, outFragShaderModule));

	delete[] code;
	code = NULL;

}

void DestroyShaderModule(VkDevice device, VkShaderModule* shaderModule)
{
	vkDestroyShaderModule(device, *shaderModule, nullptr);
	*shaderModule = VK_NULL_HANDLE;
}

#endif // VULKAN_SHADER_AND_UNIFORMS_H_ 

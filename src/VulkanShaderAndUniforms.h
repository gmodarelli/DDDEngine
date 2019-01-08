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

void SetupShaderandUniforms(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkShaderModule* outVertShaderModule, VkShaderModule* outFragShaderModule, Buffer *outBuffer)
{
	// Simple Vertex and Fragment Shaders

	uint32_t codeSize = 0;
	// FACEPALM: Assume your file is less than 10000 bytes
	char* code = new char[10000];
	ReadCode("../data/shaders/vert.spv", code, &codeSize);

	VkShaderModuleCreateInfo vertexShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vertexShaderCreateInfo.codeSize = codeSize;
	vertexShaderCreateInfo.pCode = (uint32_t*)code;

	VK_CHECK(vkCreateShaderModule(device, &vertexShaderCreateInfo, nullptr, outVertShaderModule));

	ReadCode("../data/shaders/frag.spv", code, &codeSize);

	VkShaderModuleCreateInfo fragmentShaderCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	fragmentShaderCreateInfo.codeSize = codeSize;
	fragmentShaderCreateInfo.pCode = (uint32_t*)code;

	VK_CHECK(vkCreateShaderModule(device, &fragmentShaderCreateInfo, nullptr, outFragShaderModule));

	delete[] code;
	code = NULL;

	// TODO: Extract this code into a camera struct
	// Create a uniform buffer for passing constant data to the shader
	const float PI = 3.14159265359f;
	const float TO_RAD = PI / 180.0f;

	// Perspecive Projection parameters
	float fov = 45.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
	float aspectRatio = width / (float)height;
	float t = 1.0f / tanf(fov * TO_RAD * 0.5f);
	float nf = nearZ - farZ;

	// Simple Model/View/Projection matrices
	static float lhProjectionMatrix[16] =
	{
		t / aspectRatio, 0, 0, 0,
		0, -1 * t, 0, 0,
		0, 0, (-nearZ - farZ) / nf, (2 * nearZ * farZ) / nf,
		0, 0, 1, 0
	};

	static float lhViewMatrix[16] =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// TODO: This matrix needs to be attached to a model
	// Position of the object
	float positionX = 0;
	float positionY = 0;
	float positionZ = 5;

	static float lhModelMatrix[16] =
	{
		1, 0, 0, positionX,
		0, 1, 0, positionY,
		0, 0, 1, positionZ,
		0, 0, 0, 1
	};

	VkDeviceSize bufferSize = sizeof(float) * 16 * 3;
	// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU
	// NOTE: This is the optimal path for mobile GPUs, where the memory unified.
	CreateBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outBuffer);

	// Set buffer content
	VK_CHECK(vkMapMemory(device, outBuffer->DeviceMemory, 0, VK_WHOLE_SIZE, 0, &outBuffer->data));

	memcpy(outBuffer->data, &lhProjectionMatrix[0], sizeof(lhProjectionMatrix));
	memcpy(((float*)outBuffer->data + 16), &lhViewMatrix[0], sizeof(lhViewMatrix));
	memcpy(((float*)outBuffer->data + 32), &lhModelMatrix[0], sizeof(lhModelMatrix));
}

void DestroyShaderModule(VkDevice device, VkShaderModule* shaderModule)
{
	vkDestroyShaderModule(device, *shaderModule, nullptr);
	*shaderModule = VK_NULL_HANDLE;
}

#endif // VULKAN_SHADER_AND_UNIFORMS_H_ 

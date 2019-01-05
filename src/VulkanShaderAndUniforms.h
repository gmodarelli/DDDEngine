#ifndef VULKAN_SHADER_AND_UNIFORMS_H_
#define VULKAN_SHADER_AND_UNIFORMS_H_

void ReadCode(const char* path, char* outCode, uint32_t* outCodeSize)
{
	FILE* fileHandle = NULL;
	fileHandle = fopen(path, "rb");
	printf("Trying to open: %s\n", path);
	assert(fileHandle != NULL && "Could not find the file");
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
		0, t, 0, 0,
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

	// Position of the object
	float positionX = 0;
	float positionY = 2;
	float positionZ = 10;

	static float lhModelMatrix[16] =
	{
		1, 0, 0, positionX,
		0, 1, 0, positionY,
		0, 0, 1, positionZ,
		0, 0, 0, 1
	};

	// Create uniform buffers
	VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = sizeof(float) * 16 * 3; // 3 4x4 matrices
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &outBuffer->Buffer));

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, outBuffer->Buffer, &memoryRequirements);

	VkMemoryAllocateInfo bufferAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	bufferAllocateInfo.allocationSize = memoryRequirements.size;

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
	{
		VkMemoryType memoryType = memoryProperties.memoryTypes[i];
		if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		{
			bufferAllocateInfo.memoryTypeIndex = i;
			break;
		}
	}

	VK_CHECK(vkAllocateMemory(device, &bufferAllocateInfo, nullptr, &outBuffer->DeviceMemory));
	VK_CHECK(vkBindBufferMemory(device, outBuffer->Buffer, outBuffer->DeviceMemory, 0));

	// Set buffer content
	void* mapped = NULL;
	VK_CHECK(vkMapMemory(device, outBuffer->DeviceMemory, 0, VK_WHOLE_SIZE, 0, &mapped));

	memcpy(mapped, &lhProjectionMatrix[0], sizeof(lhProjectionMatrix));
	memcpy(((float*)mapped + 16), &lhViewMatrix[0], sizeof(lhViewMatrix));
	memcpy(((float*)mapped + 32), &lhModelMatrix[0], sizeof(lhModelMatrix));

	vkUnmapMemory(device, outBuffer->DeviceMemory);
}

void DestroyShaderModule(VkDevice device, VkShaderModule* shaderModule)
{
	vkDestroyShaderModule(device, *shaderModule, nullptr);
	*shaderModule = VK_NULL_HANDLE;
}

#endif // VULKAN_SHADER_AND_UNIFORMS_H_ 

#ifndef VULKAN_VERTEX_BUFFER_H_
#define VULKAN_VERTEX_BUFFER_H_

void SetupVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t* outTriangleNumber, Buffer* outVertexInputBuffer)
{
	// A cube
	static const float verticesForCube[] =
	{
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, +1.0f, +1.0f,
		-1.0f, -1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, +1.0f, -1.0f,
		+1.0f, -1.0f, -1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, -1.0f,
		-1.0f, +1.0f, -1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, -1.0f,
		-1.0f, +1.0f, +1.0f,
		+1.0f, +1.0f, +1.0f,
		-1.0f, +1.0f, +1.0f,
		+1.0f, -1.0f, +1.0f,
	};

	const int vertexCount = sizeof(verticesForCube) / (sizeof(float) * 3);
	*outTriangleNumber = vertexCount / 3;

	// Create the vertex buffer
	VkBufferCreateInfo vbi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vbi.size = sizeof(Vertex) * vertexCount;
	vbi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vbi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vbi.queueFamilyIndexCount = 0;
	vbi.pQueueFamilyIndices = nullptr;

	VK_CHECK(vkCreateBuffer(device, &vbi, nullptr, &outVertexInputBuffer->buffer));

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, outVertexInputBuffer->buffer, &memoryRequirements);

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

	VK_CHECK(vkAllocateMemory(device, &bufferAllocateInfo, nullptr, &outVertexInputBuffer->memory));

	// Set buffer content
	void* mapped = NULL;
	VK_CHECK(vkMapMemory(device, outVertexInputBuffer->memory, 0, VK_WHOLE_SIZE, 0, &mapped));

	Vertex* vertices = (Vertex*)mapped;
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		vertices[i].x = verticesForCube[i * 3 + 0];
		vertices[i].y = verticesForCube[i * 3 + 1];
		vertices[i].z = verticesForCube[i * 3 + 2];
		vertices[i].w = 1;

		vertices[i].r = vertices[i].x;
		vertices[i].g = (float(i % 10) * 0.1f);
		vertices[i].b = vertices[i].z;
		vertices[i].a = 1;
	}

	vkUnmapMemory(device, outVertexInputBuffer->memory);
	VK_CHECK(vkBindBufferMemory(device, outVertexInputBuffer->buffer, outVertexInputBuffer->memory, 0));
}

void DestroyVertexBuffer(VkDevice device, Buffer* buffer)
{
	vkFreeMemory(device, buffer->memory, nullptr);
	vkDestroyBuffer(device, buffer->buffer, nullptr);

	buffer->memory = nullptr;
	buffer->buffer = VK_NULL_HANDLE;
}

#endif // VULKAN_VERTEX_BUFFER_H_ 

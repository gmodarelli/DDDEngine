#ifndef VULKAN_VERTEX_BUFFER_H_
#define VULKAN_VERTEX_BUFFER_H_

void CreateBuffersForMesh(VkDevice device, VkPhysicalDevice physicalDevice, Mesh mesh, Buffer* outVertexBuffer, Buffer* outIndexBuffer)
{
	VkDeviceSize vertexBufferSize = sizeof(Vertex) * mesh.Vertices.size();
	// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU
	CreateBuffer(device, physicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, outVertexBuffer);

	void* vertexData;
	vkMapMemory(device, outVertexBuffer->DeviceMemory, 0, vertexBufferSize, 0, &vertexData);
	memcpy(vertexData, mesh.Vertices.data(), (size_t)vertexBufferSize);
	vkUnmapMemory(device, outVertexBuffer->DeviceMemory);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * mesh.Indices.size();
	// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU
	CreateBuffer(device, physicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, outIndexBuffer);

	void* indexData;
	vkMapMemory(device, outIndexBuffer->DeviceMemory, 0, indexBufferSize, 0, &indexData);
	memcpy(indexData, mesh.Indices.data(), (size_t)indexBufferSize);
	vkUnmapMemory(device, outIndexBuffer->DeviceMemory);
}

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

	VK_CHECK(vkCreateBuffer(device, &vbi, nullptr, &outVertexInputBuffer->Buffer));

	// Allocate memory for the buffer
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, outVertexInputBuffer->Buffer, &memoryRequirements);

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

	VK_CHECK(vkAllocateMemory(device, &bufferAllocateInfo, nullptr, &outVertexInputBuffer->DeviceMemory));

	// Set buffer content
	void* mapped = NULL;
	VK_CHECK(vkMapMemory(device, outVertexInputBuffer->DeviceMemory, 0, VK_WHOLE_SIZE, 0, &mapped));

	Vertex* vertices = (Vertex*)mapped;
	for (uint32_t i = 0; i < vertexCount; ++i)
	{
		vertices[i].vx = verticesForCube[i * 3 + 0];
		vertices[i].vy = verticesForCube[i * 3 + 1];
		vertices[i].vz = verticesForCube[i * 3 + 2];
	}

	vkUnmapMemory(device, outVertexInputBuffer->DeviceMemory);
	VK_CHECK(vkBindBufferMemory(device, outVertexInputBuffer->Buffer, outVertexInputBuffer->DeviceMemory, 0));
}

#endif // VULKAN_VERTEX_BUFFER_H_ 

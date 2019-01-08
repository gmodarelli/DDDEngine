#ifndef VULKAN_VERTEX_BUFFER_H_
#define VULKAN_VERTEX_BUFFER_H_

void CreateBuffersForMesh(VkDevice device, VkPhysicalDevice physicalDevice, Mesh mesh, Buffer* outVertexBuffer, Buffer* outIndexBuffer)
{
	VkDeviceSize vertexBufferSize = sizeof(Vertex) * mesh.Vertices.size();
	// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU
	// NOTE: This is the optimal path for mobile GPUs, where the memory unified.
	CreateBuffer(device, physicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outVertexBuffer);

	vkMapMemory(device, outVertexBuffer->DeviceMemory, 0, vertexBufferSize, 0, &outVertexBuffer->data);
	assert(outVertexBuffer->size >= mesh.Vertices.size() * sizeof(Vertex));
	memcpy(outVertexBuffer->data, mesh.Vertices.data(), (size_t)vertexBufferSize);

	VkDeviceSize indexBufferSize = sizeof(uint32_t) * mesh.Indices.size();
	// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU.
	// NOTE: This is the optimal path for mobile GPUs, where the memory unified.
	CreateBuffer(device, physicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outIndexBuffer);

	vkMapMemory(device, outIndexBuffer->DeviceMemory, 0, indexBufferSize, 0, &outIndexBuffer->data);
	assert(outIndexBuffer->size >= mesh.Indices.size() * sizeof(uint32_t));
	memcpy(outIndexBuffer->data, mesh.Indices.data(), (size_t)indexBufferSize);
}

#endif // VULKAN_VERTEX_BUFFER_H_ 

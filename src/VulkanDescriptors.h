#ifndef VULKAN_DESCRIPTORS_H_
#define VULKAN_DESCRIPTORS_H_

void SetupDescriptors(VkDevice device, VkBuffer uniformBuffer, uint32_t descriptorSetCount, Descriptor *outDescriptor)
{
	R_ASSERT(descriptorSetCount >= 1 && "You must allocate at least on DescriptorSet");

	VkDescriptorSetLayoutBinding bindings[1];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setLayoutCreateInfo.bindingCount = 1;
	setLayoutCreateInfo.pBindings = bindings;

	VK_CHECK(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &outDescriptor->DescriptorSetLayout));

	VkDescriptorPoolSize uniformBufferPoolSize[1];
	uniformBufferPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolCreateInfo.maxSets = 1;
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = uniformBufferPoolSize;

	VK_CHECK(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &outDescriptor->DescriptorPool));

	outDescriptor->DescriptorSetCount = descriptorSetCount;
	outDescriptor->DescriptorSets.resize(descriptorSetCount);
	VkDescriptorSetAllocateInfo dsai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	dsai.descriptorPool = outDescriptor->DescriptorPool;
	dsai.descriptorSetCount = descriptorSetCount;
	dsai.pSetLayouts = &outDescriptor->DescriptorSetLayout;

	VK_CHECK(vkAllocateDescriptorSets(device, &dsai, outDescriptor->DescriptorSets.data()));

	// When the sets are allocated all their values are undefined and all descriptors are uninitialized.
	// You must init all statically used bindings
	VkDescriptorBufferInfo dbi = {};
	dbi.buffer = uniformBuffer;
	dbi.offset = 0;
	dbi.range = VK_WHOLE_SIZE;

	std::vector<VkWriteDescriptorSet> writeDescriptors;
	for (uint32_t i = 0; i < descriptorSetCount; ++i)
	{
		VkWriteDescriptorSet wd = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wd.descriptorCount = 1;
		wd.dstSet = outDescriptor->DescriptorSets[i];
		wd.dstBinding = 0;
		wd.dstArrayElement = 0;
		wd.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		wd.pImageInfo = nullptr;
		wd.pBufferInfo = &dbi;
		wd.pTexelBufferView = nullptr;

		writeDescriptors.push_back(wd);
	}

	vkUpdateDescriptorSets(device, descriptorSetCount, writeDescriptors.data(), 0, nullptr);
}

void DestroyDescriptor(VkDevice device, Descriptor* descriptor)
{
	// NOTE: You can only free descriptor sets if the pool was created with the
	// VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
	// TODO: Figure out if we need that bit set and what it does
	// vkFreeDescriptorSets(device, descriptor->pool, descriptor->setCount, descriptor->sets.data());
	vkDestroyDescriptorPool(device, descriptor->DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptor->DescriptorSetLayout, nullptr);
	descriptor->DescriptorPool = VK_NULL_HANDLE;
	descriptor->DescriptorSetLayout = VK_NULL_HANDLE;
	descriptor->DescriptorSetCount = 0;
	descriptor->DescriptorSets.clear();
}

#endif // VULKAN_DESCRIPTORS_H_ 

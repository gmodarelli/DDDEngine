#pragma once
 
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "tiny_gltf.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <chrono>

namespace gm
{
	struct Primitive
	{
		uint32_t firstIndex;
		uint32_t indexCount;

		struct Dimensions
		{
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		void setDimensions(glm::vec3 min, glm::vec3 max)
		{
			dimensions.min = min;
			dimensions.max = max;
			dimensions.size = max - min;
			dimensions.center = (min + max) * 0.5f;
			dimensions.radius = glm::distance(min, max) * 0.5f;
		}

		// TODO: Add material
		Primitive(uint32_t firstIndex, uint32_t indexCount) : firstIndex(firstIndex), indexCount(indexCount) {};
	};

	struct Mesh
	{
		gm::VulkanDevice* device;

		std::vector<Primitive*> primitives;

		struct UniformBuffer
		{
			VkBuffer buffer;
			VkDeviceMemory memory;
			VkDescriptorBufferInfo descriptor;
			VkDescriptorSet descriptorSet;
			void* mapped;
		} uniformBuffer;

		struct UniformBlock
		{
			glm::mat4 matrix;
		} uniformBlock;

		Mesh(gm::VulkanDevice* device, glm::mat4 matrix)
		{
			this->device = device;
			this->uniformBlock.matrix = matrix;

			GM_CHECK(gm::createBuffer(
				device,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(uniformBlock),
				&uniformBuffer.buffer,
				&uniformBuffer.memory,
				&uniformBlock), "Failed to create uniform buffer");

			GM_CHECK(vkMapMemory(device->Device, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped), "Failed to map memory to uniform buffer");
			uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
		};

		~Mesh()
		{
			vkDestroyBuffer(device->Device, uniformBuffer.buffer, nullptr);
			vkFreeMemory(device->Device, uniformBuffer.memory, nullptr);

			for (Primitive* p : primitives)
				delete p;
		}
	};

	struct Node
	{
		Node* parent;
		uint32_t index;

		std::vector<Node*> children;
		glm::mat4 matrix;
		std::string name;

		Mesh* mesh;
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{};

		~Node()
		{
			if (mesh)
				delete mesh;

			for (auto& child : children)
				delete child;
		}

		glm::mat4 localMatrix()
		{
			return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
		}

		glm::mat4 getMatrix()
		{
			glm::mat4 m = localMatrix();
			gm::Node* p = parent;
			while (p)
			{
				m = p->localMatrix() * m;
				p = p->parent;
			}

			return m;
		}
	};

	struct Model
	{
		gm::VulkanDevice* device;

		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 uv;
		};

		struct Vertices
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory;
		} vertices;

		struct Indices
		{
			int count;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory;
		} indices;

		std::vector<Node*> nodes;
		std::vector<Node*> linearNodes;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		void destroy()
		{
			if (vertices.buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(device->Device, vertices.buffer, nullptr);
				vkFreeMemory(device->Device, vertices.memory, nullptr);
			}

			if (indices.buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(device->Device, indices.buffer, nullptr);
				vkFreeMemory(device->Device, indices.memory, nullptr);
			}

			for (auto node : nodes)
			{
				delete node;
			}
		}

		void drawNode(Node* node, VkCommandBuffer commandBuffer)
		{
			if (node->mesh)
			{
				for (Primitive* primitive : node->mesh->primitives)
				{
					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				}
				for (auto& child : node->children)
				{
					drawNode(child, commandBuffer);
				}
			}
		}

		void draw(VkCommandBuffer commandBuffer)
		{
			const VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			for (auto& node : nodes)
			{
				drawNode(node, commandBuffer);
			}
		}

		void loadNode(gm::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<gm::Model::Vertex>& vertexBuffer, float globalScale)
		{
			gm::Node* newNode = new gm::Node{};
			newNode->index = nodeIndex;
			newNode->parent = parent;
			newNode->name = node.name;
			newNode->matrix = glm::mat4(1.0f);

			// Generate local node matrix
			glm::vec3 translation = glm::vec3(0.0f);
			if (node.translation.size() == 3)
			{
				translation = glm::make_vec3(node.translation.data());
				newNode->translation = translation;
			}

			glm::mat4 rotation = glm::mat4(1.0f);
			if (node.rotation.size() == 4)
			{
				glm::quat q = glm::make_quat(node.rotation.data());
				newNode->rotation = glm::mat4(q);
			}

			glm::vec3 scale = glm::vec3(1.0f);
			if (node.scale.size() == 3)
			{
				scale = glm::make_vec3(node.scale.data());
				newNode->scale = scale;
			}

			if (node.matrix.size() == 16)
			{
				newNode->matrix = glm::make_mat4x4(node.matrix.data());
			}

			// Node with children
			if (node.children.size() > 0)
			{
				for (size_t i = 0; i < node.children.size(); ++i)
				{
					loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalScale);
				}
			}

			// Node with mesh data
			if (node.mesh > -1)
			{
				const tinygltf::Mesh mesh = model.meshes[node.mesh];
				gm::Mesh *newMesh = new gm::Mesh(device, newNode->matrix);
				for (size_t j = 0; j < mesh.primitives.size(); ++j)
				{
					const tinygltf::Primitive& primitive = mesh.primitives[j];
					if (primitive.indices < 0) continue;

					uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
					uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
					uint32_t indexCount = 0;
					glm::vec3 posMin{};
					glm::vec3 posMax{};

					// Vertices
					{
						const float* bufferPos = nullptr;
						const float* bufferNormals = nullptr;
						const float* bufferTexCoords = nullptr;

						// Position attribute is required
						GM_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end());

						const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
						bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
						posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
						posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

						if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
						{
							const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
							const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
							bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
						}

						if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
						{
							const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
							const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
							bufferTexCoords = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}

						for (size_t v = 0; v < posAccessor.count; ++v)
						{
							gm::Model::Vertex vert{};
							vert.position = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
							vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
							vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);

							vertexBuffer.push_back(vert);
						}
					}

					// Indices
					{
						const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
						const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

						indexCount = static_cast<uint32_t>(accessor.count);
						const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

						switch (accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
							const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; ++index)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; ++index)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; ++index)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						default:
							GM_ASSERT(!"Index component type not supported!");
							return;
						}
					}

					// TODO: Add material
					gm::Primitive* newPrimitive = new gm::Primitive(indexStart, indexCount);
					newPrimitive->setDimensions(posMin, posMax);
					newMesh->primitives.push_back(newPrimitive);
				}
				newNode->mesh = newMesh;
			}

			if (parent)
			{
				parent->children.push_back(newNode);
			}
			else
			{
				nodes.push_back(newNode);
			}
			linearNodes.push_back(newNode);
		}

		void loadFromFile(const char* path, gm::VulkanDevice* device, float scale = 1.0f)
		{
			tinygltf::Model gltfModel;
			tinygltf::TinyGLTF gltfContext;
			std::string error;
			std::string warning;

			this->device = device;

			auto fileLoadStart = std::chrono::high_resolution_clock::now();

			bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, path);

			auto fileLoadEnd = std::chrono::high_resolution_clock::now();
			auto fileLoad = (fileLoadEnd - fileLoadStart).count() * 1e-6;
			printf("glTF model loaded in %.2f ms\n", fileLoad);

			std::vector<uint32_t> indexBuffer;
			std::vector<gm::Model::Vertex> vertexBuffer;

			auto nodeLoadStart = std::chrono::high_resolution_clock::now();

			if (fileLoaded)
			{
				// TODO: scene handling with no default scene
				const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
				for (size_t i = 0; i < scene.nodes.size(); ++i)
				{
					const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
					loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
				}
			}
			else
			{
				char* message = new char[256];
				sprintf(message, "Could not load file: %s\n", error.c_str());
				printf(message);
				GM_ASSERT(false);
			}

			auto nodeLoadEnd = std::chrono::high_resolution_clock::now();
			auto nodeLoad = (nodeLoadEnd - nodeLoadStart).count() * 1e-6;
			printf("glTF nodes loaded in %.2f ms\n", nodeLoad);

			size_t vertexBufferSize = vertexBuffer.size() * sizeof(Model::Vertex);
			size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
			indices.count = static_cast<uint32_t>(indexBuffer.size());

			GM_ASSERT((vertexBufferSize > 0) && (indexBufferSize > 0));

			struct StagingBuffer
			{
				VkBuffer buffer = VK_NULL_HANDLE;
				VkDeviceMemory memory = { 0 };
			} vertexStaging, indexStaging;

			// Create staging buffers
			// Vertex data
			GM_CHECK(gm::createBuffer(this->device,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vertexBufferSize,
				&vertexStaging.buffer,
				&vertexStaging.memory,
				vertexBuffer.data()), "Failed to upload vertices to staging buffer");

			// Index data
			GM_CHECK(gm::createBuffer(this->device,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStaging.buffer,
				&indexStaging.memory,
				indexBuffer.data()), "Failed to upload indices to staging buffer");

			// Create device local buffers
			// Vertex buffer
			GM_CHECK(gm::createBuffer(this->device,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBufferSize,
				&vertices.buffer,
				&vertices.memory), "Failed to create vertex buffer");

			// Index buffer
			GM_CHECK(gm::createBuffer(this->device,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				&indices.buffer,
				&indices.memory), "Failed to create index buffer");

			// Copy the staging buffers
			VkCommandBuffer copyCmd = device->createTransferCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkQueue queue = device->getQueue(device->TransferFamilyIndex);
			
			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

			device->flushCommandBuffer(copyCmd, queue, true, true);

			vkDestroyBuffer(device->Device, vertexStaging.buffer, nullptr);
			vkFreeMemory(device->Device, vertexStaging.memory, nullptr);
			vkDestroyBuffer(device->Device, indexStaging.buffer, nullptr);
			vkFreeMemory(device->Device, indexStaging.memory, nullptr);

			getSceneDimensions();
		}

		void getSceneDimensions()
		{
			dimensions.min = glm::vec3(FLT_MAX);
			dimensions.max = glm::vec3(-FLT_MAX);
			for (auto node : nodes)
			{
				getNodeDimensions(node, dimensions.min, dimensions.max);
			}

			dimensions.size = dimensions.max - dimensions.min;
			dimensions.center = (dimensions.min + dimensions.max) * 0.5f;
			dimensions.radius = glm::distance(dimensions.min, dimensions.max) * 0.5f;
		}

		void getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max)
		{
			if (node->mesh)
			{
				for (Primitive* primitive : node->mesh->primitives)
				{
					glm::vec4 locMin = glm::vec4(primitive->dimensions.min, 1.0f) * node->getMatrix();
					glm::vec4 locMax = glm::vec4(primitive->dimensions.max, 1.0f) * node->getMatrix();
					if (locMin.x < min.x) { min.x = locMin.x; }
					if (locMin.y < min.y) { min.y = locMin.y; }
					if (locMin.z < min.z) { min.z = locMin.z; }
					if (locMax.x < max.x) { max.x = locMax.x; }
					if (locMax.y < max.y) { max.y = locMax.y; }
					if (locMax.z < max.z) { max.z = locMax.z; }
				}
			}
			for (auto child : node->children)
			{
				getNodeDimensions(child, min, max);
			}
		}
	};
}

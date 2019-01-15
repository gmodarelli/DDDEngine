#pragma once

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include "meshoptimizer.h"
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

namespace vkr
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 UV;
		glm::vec3 Color;
	};

	struct ScenePart
	{
		uint32_t IndexBase;
		uint32_t IndexCount;
	};

	struct Scene
	{
		const aiScene* aScene;
		std::vector<vkr::ScenePart> meshes;
		std::vector<vkr::Vertex> vertices;
		std::vector<uint32_t> indices;
		vkr::Camera camera;

		void load(const char* path, const vkr::VulkanDevice& vulkanDevice, Buffer* outVertexBuffer, Buffer* outIndexBuffer)
		{
			Assimp::Importer Importer;
			int importFlags = aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_GenNormals;
			aScene = Importer.ReadFile(path, importFlags);

			if (aScene)
			{
				// TODO: Load materials

				loadMeshes();

				VkDeviceSize vertexBufferSize = sizeof(vkr::Vertex) * vertices.size();
				// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU
				// NOTE: This is the optimal path for mobile GPUs, where the memory unified.
				CreateBuffer(vulkanDevice.Device, vulkanDevice.PhysicalDevice, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outVertexBuffer);

				vkMapMemory(vulkanDevice.Device, outVertexBuffer->DeviceMemory, 0, vertexBufferSize, 0, &outVertexBuffer->data);
				memcpy(outVertexBuffer->data, vertices.data(), (size_t)vertexBufferSize);

				VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
				// TODO: We could also create a staging buffer and use the transfer queue to upload to the GPU.
				// NOTE: This is the optimal path for mobile GPUs, where the memory unified.
				CreateBuffer(vulkanDevice.Device, vulkanDevice.PhysicalDevice, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, outIndexBuffer);

				vkMapMemory(vulkanDevice.Device, outIndexBuffer->DeviceMemory, 0, indexBufferSize, 0, &outIndexBuffer->data);
				memcpy(outIndexBuffer->data, indices.data(), (size_t)indexBufferSize);
			}
			else
			{
				printf("Error parsing '%s': '%s'\n", path, Importer.GetErrorString());
			}
		}

	private:
		void loadMeshes()
		{
			meshes.resize(aScene->mNumMeshes);
#if _DEBUG
			printf("Found %d meshes\n", aScene->mNumMeshes);
#endif

			for (uint32_t i = 0; i < meshes.size(); ++i)
			{
				aiMesh* aMesh = aScene->mMeshes[i];

				std::vector<vkr::Vertex> meshVertices;
#if _DEBUG
				printf("Mesh %s, Faces: %d\n", aMesh->mName.C_Str(), aMesh->mNumFaces);
#endif
				// Vertices
				bool hasUV = aMesh->HasTextureCoords(0);
				bool hasColor = aMesh->HasVertexColors(0);
				bool hasNormals = aMesh->HasNormals();

				for (uint32_t v = 0; v < aMesh->mNumVertices; ++v)
				{
					vkr::Vertex vertex;
					vertex.Position = glm::make_vec3(&aMesh->mVertices[v].x);
					vertex.Position.y = -vertex.Position.y; // Vulkan y-axis points down

					// TODO: There could be multiple textures
					vertex.UV = hasUV ? glm::vec3(glm::make_vec2(&aMesh->mTextureCoords[0][v].x), 1) : glm::vec3(0.0f);

					vertex.Normal = hasNormals ? glm::make_vec3(&aMesh->mNormals[v].x) : glm::vec3(0.0f);
					vertex.Normal.y = -vertex.Normal.y; // Vulkan y-axis points down
					vertex.Normal = glm::normalize(vertex.Normal);

					// TODO: There could be multiple colors
					vertex.Color = hasColor ? glm::make_vec3(&aMesh->mColors[0][v].r) : glm::vec3(1.0f);

					meshVertices.push_back(vertex);
				}


				// Unoptimized, duplicated vertices
				if (1)
				{
					// Note: Index Base represents the offset to the first index in the vertex buffer
					meshes[i].IndexCount = aMesh->mNumFaces * 3;
					meshes[i].IndexBase = static_cast<uint32_t>(vertices.size());

					// Indices
					for (uint32_t f = 0; f < aMesh->mNumFaces; ++f)
					{
						for (uint32_t j = 0; j < 3; ++j)
						{
							indices.push_back(aMesh->mFaces[f].mIndices[j]);
						}
					}

					vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
				}
				else
				{
					// TODO: This mesh optimization doesn't seem to work with multiple meshes.
					// I must be doing something stupid when putting them together into the index/vertex buffers
					// cause when optimizing them one by one (that is without going from 0 to size - 1) the
					// meshes look alright.

					// Note: Index Base represents the offset to the first index in the vertex buffer
					meshes[i].IndexCount = aMesh->mNumFaces * 3;
					meshes[i].IndexBase = static_cast<uint32_t>(vertices.size());

					uint32_t indexCount = aMesh->mNumFaces * 3;

					// Calculate unique vertices count
					std::vector<uint32_t> remap(indexCount);
					size_t vertexCount = meshopt_generateVertexRemap(remap.data(), 0, indexCount, meshVertices.data(), indexCount, sizeof(vkr::Vertex));

					// Reindexing
					std::vector<uint32_t> meshIndices(indexCount);
					meshopt_remapIndexBuffer(meshIndices.data(), 0, indexCount, remap.data());

					// Unique vertices
					std::vector<vkr::Vertex> uniqVertices(vertexCount);
					meshopt_remapVertexBuffer(uniqVertices.data(), meshVertices.data(), indexCount, sizeof(vkr::Vertex), remap.data());

#if _DEBUG
					// validate
					assert(indexCount % 3 == 0);
					const unsigned int* indicesTest = &meshIndices[0];
					for (size_t i = 0; i < indexCount; ++i)
						assert(indicesTest[i] < vertexCount);
#endif

					vertices.insert(vertices.end(), uniqVertices.begin(), uniqVertices.end());
					indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());
				}
			}

#if _DEBUG
			printf("Meshes imported\n");
#endif
		}
	};
}

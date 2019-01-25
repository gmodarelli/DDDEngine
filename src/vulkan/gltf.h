#pragma once
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <cassert>
#include <string>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "volk.h"
#include "utils.h"

namespace gm
{
	typedef rapidjson::GenericObject<false, rapidjson::Value::ValueType> JsonObject;
	typedef rapidjson::GenericArray<false, rapidjson::Value::ValueType> JsonArray;
	typedef rapidjson::GenericObject<true, rapidjson::Value::ValueType> ConstJsonObject;
	typedef rapidjson::GenericArray<true, rapidjson::Value::ValueType> ConstJsonArray;

	struct Node
	{
		int32_t parentId = -1;
		int32_t meshId = -1;
		glm::mat4 matrix = glm::identity<glm::mat4>();
	};

	struct Mesh
	{
		const char* name;
	};

	// TODO: Extend this once we can read more info
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
	};

	struct Primitive
	{
		uint32_t meshId;
		int32_t materialId;
		uint32_t firstIndex;
		uint32_t indexCount;
	};

	struct Material
	{
		std::string name = "";
		std::string alphaMode = "OPAQUE";

		struct PBRMetallicRoughness
		{
			glm::vec4 baseColorFactor = glm::vec4(1.0f);
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
		} pbrMetallicRoughness;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	};

	struct UniformBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo descriptor = {};
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		void* mapped = {0};
	};

	struct Model
	{
		std::vector<Node> nodes;
		std::vector<Mesh> meshes;
		std::vector<UniformBuffer> uniformBuffers;
		std::vector<Primitive> primitives;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		// These are the offset into the two "big" vertex and index buffers that
		// all the models share on the GPU. That is we don't have one vertex buffer
		// and one index buffer per model
		uint32_t indexOffset;
		uint32_t vertexOffet;

		std::vector<Material> opaqueMaterials;
		std::vector<Material> maskMaterials;
		std::vector<Material> transparentMaterials;
	};

	static rapidjson::Document parseFile(std::string path);
	static bool isValidFormat(rapidjson::Document& document);
	static void loadNode(uint32_t nodeIndex, int32_t parentIndex, const JsonArray& nodes, const JsonArray& meshes, Model& model, gm::VulkanDevice* device);
	static void readBuffer(std::string path, unsigned char* buffer, uint32_t bufferSize);

	Model loadModelFromFile(std::string path, gm::VulkanDevice* device)
	{
		Model model = {};

		auto jsonParseStart = std::chrono::high_resolution_clock::now();
		rapidjson::Document document = parseFile(path);
		auto jsonParseEnd = std::chrono::high_resolution_clock::now();
		assert(isValidFormat(document));

		auto jsonParseDuration = (jsonParseEnd - jsonParseStart).count() * 1e-6;
		printf("[glTF] JSON parsed in %.2f ms\n", jsonParseDuration);

		// Preparation work

		// Parse buffers
		std::vector<unsigned char*> buffers;
		{
			JsonArray buffers_ = document["buffers"].GetArray();
			std::string basePath = path.substr(0, path.find_last_of("/\\") + 1);
			for (rapidjson::SizeType i = 0; i < buffers_.Size(); ++i)
			{
				JsonObject buffer = buffers_[i].GetObjectW();
				uint32_t bufferSize = buffer["byteLength"].GetInt();
				std::string bufferName = buffer["uri"].GetString();

				unsigned char* data = new unsigned char[bufferSize];
				readBuffer(basePath + bufferName, data, bufferSize);
				buffers.push_back(data);
			}
		}

		// Parse buffer views
		struct BufferView
		{
			uint32_t bufferId;
			uint32_t byteOffset;
			uint32_t byteLength;
			uint32_t byteStride;
			uint32_t target;
		};

		std::vector<BufferView> bufferViews;

		{
			JsonArray bufferViews_ = document["bufferViews"].GetArray();
			for (rapidjson::SizeType i = 0; i < bufferViews_.Size(); ++i)
			{
				JsonObject bufferView_ = bufferViews_[i].GetObjectW();
				BufferView bufferView;

				// Required fields
				bufferView.bufferId = bufferView_["buffer"].GetInt();
				bufferView.byteLength = bufferView_["byteLength"].GetInt();

				// Optional fields
				bufferView.byteOffset = bufferView_.HasMember("byteOffset") ? bufferView_["byteOffset"].GetInt() : 0;
				bufferView.byteStride = bufferView_.HasMember("byteStride") ? bufferView_["byteStride"].GetInt() : 0;
				bufferView.target = bufferView_.HasMember("target") ? bufferView_["target"].GetInt() : 0;

				bufferViews.push_back(bufferView);
			}
		}

		// Parse accessors
		struct Accessor
		{
			uint32_t bufferViewId;
			uint32_t byteOffset;
			uint32_t componentType;
			uint32_t count;
			std::string type;
			bool normalized;
			std::vector<double> min;
			std::vector<double> max;
		};

		std::vector<Accessor> accessors;

		{
			JsonArray accessors_ = document["accessors"].GetArray();
			for (rapidjson::SizeType i = 0; i < accessors_.Size(); ++i)
			{
				JsonObject accessor_ = accessors_[i].GetObjectW();
				Accessor accessor;

				// Require fields
				accessor.componentType = accessor_["componentType"].GetInt();
				accessor.count = accessor_["count"].GetInt();
				accessor.type = accessor_["type"].GetString();

				// Optional fields
				accessor.bufferViewId = accessor_.HasMember("bufferView") ? accessor_["bufferView"].GetInt() : 0;
				accessor.byteOffset = accessor_.HasMember("byteOffset") ? accessor_["byteOffset"].GetInt() : 0;
				accessor.normalized = accessor_.HasMember("normalized") ? accessor_["normalized"].GetBool() : false;

				// Min Max
				if (accessor_.HasMember("min"))
				{
					JsonArray min_ = accessor_["min"].GetArray();
					for (rapidjson::SizeType j = 0; j < min_.Size(); ++j)
					{
						accessor.min.push_back(min_[j].GetDouble());
					}
				}

				if (accessor_.HasMember("max"))
				{
					JsonArray max_ = accessor_["max"].GetArray();
					for (rapidjson::SizeType j = 0; j < max_.Size(); ++j)
					{
						accessor.max.push_back(max_[j].GetDouble());
					}
				}

				accessors.push_back(accessor);
			}
		}

		// Parse materials
		JsonArray materials = document["materials"].GetArray();
		{
			for (rapidjson::SizeType i = 0; i < materials.Size(); ++i)
			{
				JsonObject material_ = materials[i].GetObjectW();
				Material material;
				material.name = material_.HasMember("name") ? material_["name"].GetString() : "material_" + i;
				material.alphaMode = material_.HasMember("alphaMode") ? material_["alphaMode"].GetString() : "OPAQUE";

				// NOTE: We only support pbrMetallicRoughness for now
				// TODO: Remove this assert and add support for more workflows
				assert(material_.HasMember("pbrMetallicRoughness"));
				JsonObject pbrMetallicRoughness = material_["pbrMetallicRoughness"].GetObjectW();

				material.pbrMetallicRoughness = {};
				material.pbrMetallicRoughness.baseColorFactor = glm::vec4(1.0f);
				if (pbrMetallicRoughness.HasMember("baseColorFactor"))
				{
					JsonArray baseColorFactor = pbrMetallicRoughness["baseColorFactor"].GetArray();
					material.pbrMetallicRoughness.baseColorFactor.r = baseColorFactor[0].GetFloat();
					material.pbrMetallicRoughness.baseColorFactor.g = baseColorFactor[1].GetFloat();
					material.pbrMetallicRoughness.baseColorFactor.b = baseColorFactor[2].GetFloat();
					material.pbrMetallicRoughness.baseColorFactor.a = baseColorFactor[3].GetFloat();
				}

				material.pbrMetallicRoughness.metallicFactor = 0.0f;
				if (pbrMetallicRoughness.HasMember("metallicFactor"))
				{
					material.pbrMetallicRoughness.metallicFactor = pbrMetallicRoughness["metallicFactor"].GetFloat();
				}

				material.pbrMetallicRoughness.roughnessFactor = 0.0f;
				if (pbrMetallicRoughness.HasMember("roughnessFactor"))
				{
					material.pbrMetallicRoughness.roughnessFactor = pbrMetallicRoughness["roughnessFactor"].GetFloat();
				}

				if (material.alphaMode == "OPAQUE")
				{
					model.opaqueMaterials.push_back(material);
				}
				else if (material.alphaMode == "MASK")
				{
					model.maskMaterials.push_back(material);
				}
				else if (material.alphaMode == "BLEND")
				{
					model.transparentMaterials.push_back(material);
				}
			}
		}

		// NOTE: For now we're assuming there's only one scene, at index 0
		auto modelParseStart = std::chrono::high_resolution_clock::now();
		rapidjson::Value& value = document["scenes"];
		JsonArray scenes = value.GetArray();
		JsonObject scene = scenes[0].GetObjectW();
		value = scene["nodes"];

		JsonArray rootNodes = value.GetArray();
		JsonArray nodes = document["nodes"].GetArray();
		JsonArray meshes = document["meshes"].GetArray();

		model.nodes.resize((size_t)nodes.Size());
		model.meshes.resize((size_t)meshes.Size());
		model.uniformBuffers.resize((size_t)meshes.Size());

		// Parse meshes
		{
			for (rapidjson::SizeType meshId = 0; meshId < meshes.Size(); ++meshId)
			{
				JsonObject mesh_ = meshes[meshId].GetObjectW();
				Mesh mesh = {};
				mesh.name = mesh_["name"].GetString();

				// Parse mesh primitives
				JsonArray primitives = mesh_["primitives"].GetArray();
				for (rapidjson::SizeType j = 0; j < primitives.Size(); ++j)
				{
					JsonObject primitive_ = primitives[j].GetObjectW();
					Primitive primitive;
					primitive.meshId = meshId;
					primitive.materialId = primitive_.HasMember("material") ? primitive_["material"].GetInt() : -1;
					primitive.firstIndex = static_cast<uint32_t>(model.indices.size());

					// NOTE: We need this for the indices
					uint32_t vertexStart = static_cast<uint32_t>(model.vertices.size());

					JsonObject attributes = primitive_["attributes"].GetObjectW();

					// 
					// Vertex Data
					//
					{
						const float* positionBuffer = nullptr;
						const float* normalBuffer = nullptr;

						// Position should always be there
						assert(attributes.HasMember("POSITION") && "POSITION attribute not found");
						uint32_t positionAccessorId = attributes["POSITION"].GetInt();
						Accessor& positionAccessor = accessors[positionAccessorId];
						BufferView& positionBufferView = bufferViews[positionAccessor.bufferViewId];

						// TODO: We're assuming entries are float, but we should check the accessor's componentType so we can
						// do a reinterpet_cast based on that info
						positionBuffer = reinterpret_cast<const float *>(&(buffers[positionBufferView.bufferId][positionAccessor.byteOffset + positionBufferView.byteOffset]));

						if (attributes.HasMember("NORMAL"))
						{
							uint32_t normalAccessorId = attributes["NORMAL"].GetInt();
							Accessor& normalAccessor = accessors[normalAccessorId];
							BufferView& normalBufferView = bufferViews[normalAccessor.bufferViewId];

							// TODO: We're assuming entries are float, but we should check the accessor's componentType so we can
							// do a reinterpet_cast based on that info
							// assert(normalAccessor.componentType == GM_GLTF_COMPONENT_TYPE_FLOAT);
							normalBuffer = reinterpret_cast<const float *>(&(buffers[normalBufferView.bufferId][normalAccessor.byteOffset + normalBufferView.byteOffset]));
						}

						// TODO: Add UVs

						for (uint32_t v = 0; v < positionAccessor.count; ++v)
						{
							// TODO: We're assuming we're dealing with 3D vectors here, but we should check the accessor's type
							Vertex vertex;
							vertex.position = glm::make_vec3(&positionBuffer[v * 3]);
							vertex.normal = normalBuffer ? glm::make_vec3(&normalBuffer[v * 3]) : glm::vec3(0.0f);
							vertex.normal = glm::normalize(vertex.normal);

							model.vertices.push_back(vertex);
						}
					}

					//
					// Index Data
					//
					{
						if (primitive_.HasMember("indices"))
						{
							uint32_t indexAccessorId = primitive_["indices"].GetInt();
							Accessor& indexAccessor = accessors[indexAccessorId];
							BufferView& indexBufferView = bufferViews[indexAccessor.bufferViewId];

							// TODO: We're assuming entries are shorts, but we should check the accessor's componentType so we can
							// do a reinterpet_cast based on that info
							const uint16_t* indexBuffer = reinterpret_cast<const uint16_t *>(&(buffers[indexBufferView.bufferId][indexAccessor.byteOffset + indexBufferView.byteOffset]));

							for (uint32_t i = 0; i < indexAccessor.count; ++i)
							{
								model.indices.push_back(indexBuffer[i] + vertexStart);
							}

							primitive.indexCount = indexAccessor.count;
						}
					}

					model.primitives.push_back(primitive);
				}

				model.meshes[meshId] = mesh;
			}
		}

		for (rapidjson::SizeType i = 0; i < rootNodes.Size(); ++i)
		{
			uint32_t nodeIndex = (uint32_t)rootNodes[i].GetInt();
			loadNode(nodeIndex, -1, nodes, meshes, model, device);
		}

		auto modelParseEnd = std::chrono::high_resolution_clock::now();
		auto modelParseDuration = (modelParseEnd - modelParseStart).count() * 1e-6;
		printf("[glTF] Model parsed in %.2f ms\n", modelParseDuration);

		return model;
	}

	void destroyModel(gm::Model model, gm::VulkanDevice* device)
	{
		for (size_t i = 0; i < model.uniformBuffers.size(); ++i)
		{
			if (model.uniformBuffers[i].buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(device->Device, model.uniformBuffers[i].buffer, nullptr);
				model.uniformBuffers[i].buffer = VK_NULL_HANDLE;
			}

			if (model.uniformBuffers[i].memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(device->Device, model.uniformBuffers[i].memory, nullptr);
				model.uniformBuffers[i].memory = VK_NULL_HANDLE;
			}
		}
	}

	static void readBuffer(std::string path, unsigned char* buffer, uint32_t bufferSize)
	{
		// Open the input file
		FILE* fileHandle = NULL;
		fileHandle = fopen(path.c_str(), "rb");
		assert(fileHandle != NULL && "Could not open the file");

		uint32_t bytesRead = fread(buffer, sizeof(unsigned char), bufferSize, fileHandle);
		fclose(fileHandle);

		assert(bytesRead == bufferSize);
	}

	static rapidjson::Document parseFile(std::string path)
	{
		// Open the input file
		FILE* fileHandle = NULL;
		fileHandle = fopen(path.c_str(), "rb");
		assert(fileHandle != NULL && "Could not open the file");
		// Compute the file size
		fseek(fileHandle, 0, SEEK_END);
		size_t size = ftell(fileHandle);
		rewind(fileHandle);

		// Allocate a buffer to hold the file content and read the file
		char* buffer = new char[size + 1];
		uint32_t bytesRead = fread(buffer, sizeof(buffer[0]), size, fileHandle);
		assert(bytesRead == size);
		// NOTE: Make sure the buffer is null terminated
		buffer[size - 1] = '\0';

		// Parse the JSON content
		rapidjson::Document document;
		document.Parse<rapidjson::kParseStopWhenDoneFlag>(buffer);
		fclose(fileHandle);

		if (document.HasParseError())
		{
			fprintf(stderr, "\nError(offset %u): %s\n",
				(uint32_t)document.GetErrorOffset(),
				rapidjson::GetParseError_En(document.GetParseError()));

			assert(!"Error parsing the glFT document\n");
		}

		return document;
	}

	static bool isValidFormat(rapidjson::Document& document)
	{
		rapidjson::Value& asset = document["asset"];
		if (asset.IsObject())
		{
			auto value = asset.GetObjectW();
			if (value.HasMember("version") && strstr("2.0", value["version"].GetString()))
				return true;
		}

		return false;
	}

	static void loadNode(uint32_t nodeIndex, int32_t parentIndex, const JsonArray& nodes, const JsonArray& meshes, Model& model, gm::VulkanDevice* device)
	{
		JsonObject node_ = nodes[nodeIndex].GetObjectW();
		Node node;
		node.parentId = parentIndex;

		if (node_.HasMember("matrix"))
		{
			JsonArray matrix = node_["matrix"].GetArray();
			float raw[16];
			for (uint32_t m = 0; m < 16; ++m)
			{
				raw[m] = matrix[m].GetFloat();
			}

			node.matrix = glm::make_mat4x4(raw);
		}
		else
		{
			node.matrix = glm::mat4(1.0f);
		}

		if (node_.HasMember("mesh"))
		{
			node.meshId = node_["mesh"].GetInt();

			UniformBuffer uniformBuffer;

			// Create 
			GM_CHECK(gm::createBuffer(
				device,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(glm::mat4),
				&uniformBuffer.buffer,
				&uniformBuffer.memory,
				&node.matrix), "Failed to create uniform buffer");

			GM_CHECK(vkMapMemory(device->Device, uniformBuffer.memory, 0, sizeof(glm::mat4), 0, &uniformBuffer.mapped), "Failed to map memory to uniform buffer");

			uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(glm::mat4) };

			// BUG: we are potentially overriding this buffer
			model.uniformBuffers[node.meshId] = uniformBuffer;
		}

		// TODO: Add rotation, translation and scale

		// If it is a parent node
		if (node_.HasMember("children"))
		{
			auto children = node_["children"].GetArray();
			for (rapidjson::SizeType j = 0; j < children.Size(); ++j)
				loadNode(children[j].GetInt(), nodeIndex, nodes, meshes, model, device);
		}

		model.nodes[nodeIndex] = node;

	}
}

#pragma once
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <cassert>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gm
{
	typedef rapidjson::GenericObject<false, rapidjson::Value::ValueType> JsonObject;
	typedef rapidjson::GenericArray<false, rapidjson::Value::ValueType> JsonArray;
	typedef rapidjson::GenericObject<true, rapidjson::Value::ValueType> ConstJsonObject;
	typedef rapidjson::GenericArray<true, rapidjson::Value::ValueType> ConstJsonArray;

	struct gNode
	{
		int32_t parentId = -1;
		int32_t meshId = -1;
		glm::mat4 matrix = glm::identity<glm::mat4>();
	};

	struct gMesh
	{
		const char* name;
	};

	// TODO: Extend this once we can read more info
	struct gVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
	};

	struct gPrimitive
	{
		uint32_t meshId;
		uint32_t firstIndex;
		uint32_t indexCount;
	};

	struct gModel
	{
		std::vector<gNode> nodes;
		std::vector<gMesh> meshes;
		std::vector<gPrimitive> primitives;

		std::vector<gVertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;
	};

	static rapidjson::Document parseFile(std::string path);
	static bool isValidFormat(rapidjson::Document& document);
	static void loadNode(uint32_t nodeIndex, int32_t parentIndex, const JsonArray& nodes, const JsonArray& meshes, gModel& model);
	static void readBuffer(std::string path, unsigned char* buffer, uint32_t bufferSize);

	void loadModelFromFile(std::string path)
	{
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

		// NOTE: For now we're assuming there's only one scene, at index 0
		gModel model = {};
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

		// Parse meshes
		{
			for (rapidjson::SizeType meshId = 0; meshId < meshes.Size(); ++meshId)
			{
				JsonObject mesh_ = meshes[meshId].GetObjectW();
				gMesh mesh = {};
				mesh.name = mesh_["name"].GetString();

				// Parse mesh primitives
				JsonArray primitives = mesh_["primitives"].GetArray();
				for (rapidjson::SizeType j = 0; j < primitives.Size(); ++j)
				{
					gPrimitive primitive;
					primitive.meshId = meshId;
					primitive.firstIndex = static_cast<uint32_t>(model.indexBuffer.size());

					// NOTE: We need this for the indices
					uint32_t vertexStart = static_cast<uint32_t>(model.vertexBuffer.size());

					JsonObject primitive_ = primitives[j].GetObjectW();
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
							normalBuffer = reinterpret_cast<const float *>(&(buffers[normalBufferView.bufferId][normalAccessor.byteOffset + normalBufferView.byteOffset]));
						}

						// TODO: Add UVs

						for (uint32_t v = 0; v < positionAccessor.count; ++v)
						{
							// TODO: We're assuming we're dealing with 3D vectors here, but we should check the accessor's type
							gVertex vertex;
							vertex.position = glm::make_vec3(&positionBuffer[v * 3]);
							vertex.normal = normalBuffer ? glm::make_vec3(&normalBuffer[v * 3]) : glm::vec3(0.0f);
							vertex.normal = glm::normalize(vertex.normal);

							model.vertexBuffer.push_back(vertex);
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
								model.indexBuffer.push_back(indexBuffer[i] + vertexStart);
							}
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
			loadNode(nodeIndex, -1, nodes, meshes, model);
		}

		auto modelParseEnd = std::chrono::high_resolution_clock::now();
		auto modelParseDuration = (modelParseEnd - modelParseStart).count() * 1e-6;
		printf("[glTF] Model parsed in %.2f ms\n", modelParseDuration);
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

	static void loadNode(uint32_t nodeIndex, int32_t parentIndex, const JsonArray& nodes, const JsonArray& meshes, gModel& model)
	{
		JsonObject node_ = nodes[nodeIndex].GetObjectW();
		gNode node;
		node.parentId = parentIndex;

		if (node_.HasMember("mesh"))
		{
			node.meshId = node_["mesh"].GetInt();
		}

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

		// TODO: Add rotation, translation and scale

		// If it is a parent node
		if (node_.HasMember("children"))
		{
			auto children = node_["children"].GetArray();
			for (rapidjson::SizeType j = 0; j < children.Size(); ++j)
				loadNode(children[j].GetInt(), nodeIndex, nodes, meshes, model);
		}

		model.nodes[nodeIndex] = node;
	}
}

#include "loader.h"

#include <stdio.h>
#include <cassert>
#include <inttypes.h>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

namespace Resources
{

typedef rapidjson::GenericObject<false, rapidjson::Value::ValueType> JsonObject;
typedef rapidjson::GenericArray<false, rapidjson::Value::ValueType> JsonArray;
typedef rapidjson::GenericObject<true, rapidjson::Value::ValueType> ConstJsonObject;
typedef rapidjson::GenericArray<true, rapidjson::Value::ValueType> ConstJsonArray;

Model Loader::load_model(const char* path, AssetsInfo* assets_info)
{
	// Open input file
	FILE* file_handle = NULL;
	file_handle = fopen(path, "rb");
	assert(file_handle != NULL && "Failed to open asset file");

	size_t file_length = 0;

	// Read the GLB Header
	const size_t header_buffer_size = 3;
	uint32_t header_buffer[header_buffer_size];
	uint32_t objects_read = fread(header_buffer, sizeof(uint32_t), header_buffer_size, file_handle);
	assert(objects_read == header_buffer_size);
	assert(header_buffer[0] == GLTF_MAGIC && "The asset is not a GLFT file");
	assert(header_buffer[1] == GLTF_VERSION && "The asset is not of the right GLTF version");

	file_length = header_buffer[2];

	// Read the JSON Chunk Header
	size_t json_length = 0;
	const size_t chunk_header_size = 2;
	uint32_t chunk_header[chunk_header_size];
	objects_read = fread(chunk_header, sizeof(uint32_t), chunk_header_size, file_handle);
	assert(objects_read == chunk_header_size);
	assert(chunk_header[1] == GLTF_CHUNK_TYPE_JSON && "The chunk of data is not JSON");

	json_length = static_cast<size_t>(chunk_header[0]);

	// Read the JSON data
	char* json_string = new char[json_length + 1];
	uint32_t bytes_read = fread(json_string, sizeof(char), json_length, file_handle);
	assert(bytes_read == json_length);
	json_string[json_length] = '\0';

	// Parse the JSON data
	rapidjson::Document document;
	document.Parse<rapidjson::kParseStopWhenDoneFlag>(json_string);

	rapidjson::Value& document_asset = document["asset"];
	assert(document_asset.IsObject());
	auto value = document_asset.GetObject();
	assert(value.HasMember("version") && strstr("2.0", value["version"].GetString()));

	delete[] json_string;

	// Read all buffers
	size_t buffer_count = (size_t)document["buffers"].GetArray().Size();
	unsigned char** buffers = new unsigned char*[buffer_count];

	for (size_t i = 0; i < buffer_count; ++i)
	{
		// Read the next Chunk Header
		size_t chunk_length = 0;
		const size_t chunk_header_size = 2;
		uint32_t chunk_header[chunk_header_size];
		uint32_t objects_read = fread(chunk_header, sizeof(uint32_t), chunk_header_size, file_handle);
		assert(objects_read == chunk_header_size);
		assert(chunk_header[1] == GLTF_CHUNK_TYPE_BIN && "The chunk of data is not Binary");

		chunk_length = static_cast<size_t>(chunk_header[0]);

		buffers[i] = new unsigned char[chunk_length];
		uint32_t bytes_read = fread(buffers[i], sizeof(unsigned char), chunk_length, file_handle);
		assert(bytes_read == chunk_length);
	}

	fclose(file_handle);

	// Parse the content of the GLFT 2 document

	// Parse the Buffer Views
	struct BufferView
	{
		uint32_t buffer_id;
		uint32_t byte_offset;
		uint32_t byte_length;
		uint32_t byte_stride;
		uint32_t target;
	};

	JsonArray __buffer_views = document["bufferViews"].GetArray();
	size_t buffer_view_count = (size_t)__buffer_views.Size();
	BufferView* buffer_views = new BufferView[buffer_view_count];

	for (size_t i = 0; i < buffer_view_count; ++i)
	{
		JsonObject __buffer_view = __buffer_views[i].GetObject();
		buffer_views[i] = {};

		// Required fields
		buffer_views[i].buffer_id = __buffer_view["buffer"].GetInt();
		buffer_views[i].byte_length = __buffer_view["byteLength"].GetInt();

		// Optional fields
		buffer_views[i].byte_offset = __buffer_view.HasMember("byteOffset") ? __buffer_view["byteOffset"].GetInt() : 0;
		buffer_views[i].byte_stride = __buffer_view.HasMember("byteStride") ? __buffer_view["byteStride"].GetInt() : 0;
		buffer_views[i].target = __buffer_view.HasMember("target") ? __buffer_view["target"].GetInt() : 0;
	}

	// Parse the Accessors
	enum ComponentType
	{
		Byte = 5120,
		UnsignedByte = 5121,
		Short = 5122,
		UnsignedShort = 5123,
		UnsignedInt = 5125,
		Float = 5126
	};

	enum Type
	{
		Scalar,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4
	};

	struct Accessor
	{
		uint32_t buffer_view_id;
		uint32_t byte_offset;
		uint32_t count;
		ComponentType component_type;
		Type type;
		bool normalized;
		size_t min_length;
		size_t max_length;
		double min[4];
		double max[4];
	};

	JsonArray __accessors = document["accessors"].GetArray();
	size_t accessor_count = __accessors.Size();
	Accessor* accessors = new Accessor[accessor_count];

	for (size_t i = 0; i < accessor_count; ++i)
	{
		JsonObject __accessor = __accessors[i].GetObject();
		accessors[i] = {};

		// Required fields
		accessors[i].component_type = static_cast<ComponentType>(__accessor["componentType"].GetInt());
		accessors[i].count = __accessor["count"].GetInt();
		const char* string_type = __accessor["type"].GetString();
		if (strcmp("SCALAR", string_type) == 0)
		{
			accessors[i].type = Type::Scalar;
		}
		else if (strcmp("VEC2", string_type) == 0)
		{
			accessors[i].type = Type::Vec2;
		}
		else if (strcmp("VEC3", string_type) == 0)
		{
			accessors[i].type = Type::Vec3;
		}
		else
		{
			assert(!"Type not yet supported");
		}

		// Optional fields
		accessors[i].buffer_view_id = __accessor.HasMember("bufferView") ? __accessor["bufferView"].GetInt() : 0;
		accessors[i].byte_offset = __accessor.HasMember("byteOffset") ? __accessor["byteOffset"].GetInt() : 0;
		accessors[i].normalized = __accessor.HasMember("normalized") ? __accessor["normalized"].GetBool() : false;

		// Min Max
		if (__accessor.HasMember("min"))
		{
			JsonArray min = __accessor["min"].GetArray();
			assert((size_t)min.Size() <= 4);
			accessors[i].min_length = (size_t)min.Size();
			for (size_t j = 0; j < accessors[i].min_length; ++j)
			{
				accessors[i].min[j] = min[j].GetDouble();
			}
		}

		if (__accessor.HasMember("max"))
		{
			JsonArray max = __accessor["min"].GetArray();
			assert((size_t)max.Size() <= 4);
			accessors[i].max_length = (size_t)max.Size();
			for (size_t j = 0; j < accessors[i].max_length; ++j)
			{
				accessors[i].max[j] = max[j].GetDouble();
			}
		}
	}

	// Parse Materials

	// NOTE: The material index inside a glTF 2 JSON document is relative
	// to the document itself. Since we're loading all the materials
	// across different documents into one single Material* list, we
	// need a material_base_index to keep the relation between a primitive
	// and its associated material
	uint32_t material_offset = assets_info->material_offset;

	JsonArray __materials = document["materials"].GetArray();
	size_t material_size = (size_t)__materials.Size();
	for (size_t i = 0; i < material_size; ++i)
	{
		JsonObject __material = __materials[i].GetObject();
		Material material = {};
		assert(__material.HasMember("name"));
		uint32_t name_length = strlen(__material["name"].GetString());
		material.name_length = name_length;
		memcpy(material.name, __material["name"].GetString(), name_length);

		if (__material.HasMember("alphaMode"))
		{
			if (strcmp("OPAQUE", __material["alphaMode"].GetString()) == 0)
			{
				material.alpha_mode = MaterialAlphaMode::Opaque;
			}
			else if (strcmp("MASK", __material["alphaMode"].GetString()) == 0)
			{
				material.alpha_mode = MaterialAlphaMode::Mask;
			}
			else if (strcmp("BLEND", __material["alphaMode"].GetString()) == 0)
			{
				material.alpha_mode = MaterialAlphaMode::Blend;
			}
			else
			{
				assert(!"Invalid alphaMode");
			}
		}
		else
		{
			material.alpha_mode = MaterialAlphaMode::Opaque;
		}

		// TODO: Add support for other workflows
		assert(__material.HasMember("pbrMetallicRoughness"));
		JsonObject __pbr_metallic = __material["pbrMetallicRoughness"].GetObject();
		material.pbr_metallic_roughness = {};
		if (__pbr_metallic.HasMember("baseColorFactor"))
		{
			JsonArray __base_color_format = __pbr_metallic["baseColorFactor"].GetArray();
			material.pbr_metallic_roughness.base_color_factor.r = __base_color_format[0].GetFloat();
			material.pbr_metallic_roughness.base_color_factor.g = __base_color_format[1].GetFloat();
			material.pbr_metallic_roughness.base_color_factor.b = __base_color_format[2].GetFloat();
			material.pbr_metallic_roughness.base_color_factor.a = __base_color_format[3].GetFloat();
		}

		if (__pbr_metallic.HasMember("metallicFactor"))
		{
			material.pbr_metallic_roughness.metallic_factor = __pbr_metallic["metallicFactor"].GetFloat();
		}

		if (__pbr_metallic.HasMember("roughnessFactor"))
		{
			material.pbr_metallic_roughness.metallic_factor = __pbr_metallic["roughnessFactor"].GetFloat();
		}

		assets_info->materials[assets_info->material_offset] = material;
		assets_info->material_offset++;
	}

	// Parse the default scene
	JsonArray __scenes = document["scenes"].GetArray();
	assert(__scenes.Size() == 1);
	JsonObject __scene = __scenes[0].GetObject();

	// NOTE: Root nodes are the parent nodes of the scene
	JsonArray __root_nodes = __scene["nodes"].GetArray();
	// NOTE: Nodes are all the nodes present in the document
	JsonArray __nodes = document["nodes"].GetArray();
	assert(__root_nodes.Size() == __nodes.Size() && "It is time to add support for nodes hierarchy");
	JsonArray __meshes = document["meshes"].GetArray();

	Model model = {};
	memcpy(model.name, __scene["name"].GetString(), strlen(__scene["name"].GetString()));
	model.node_count = __nodes.Size();
	assert(model.node_count <= 8);

	// Parse the meshes
	uint32_t mesh_offset = assets_info->mesh_offset;

	for (rapidjson::SizeType mesh_id = 0; mesh_id < __meshes.Size(); ++mesh_id)
	{
		JsonObject __mesh = __meshes[mesh_id].GetObject();
		JsonArray __primitives = __mesh["primitives"].GetArray();
		assert(__primitives.Size() <= 8);

		Mesh mesh = {};
		mesh.primitive_count = __primitives.Size();
		memcpy(mesh.name, __mesh["name"].GetString(), strlen(__mesh["name"].GetString()));

		for (rapidjson::SizeType primitive_id = 0; primitive_id < __primitives.Size(); ++primitive_id)
		{
			JsonObject __primitive = __primitives[primitive_id].GetObject();
			Primitive primitive = {};
			primitive.material_id = material_offset + __primitive["material"].GetInt();
			primitive.index_offset = assets_info->index_offset;
			primitive.vertex_offset = assets_info->vertex_offset;

			JsonObject __attributes = __primitive["attributes"].GetObject();

			//
			// Vertex Data
			//
			const float* position_buffer = nullptr;
			const float* normal_buffer = nullptr;
			// TODO: Add UV buffer
			// const float* uv_0_buffer = nullptr;

			// Position should always be there
			assert(__attributes.HasMember("POSITION") && "POSITION attribute not found");
			uint32_t position_accessor_id = __attributes["POSITION"].GetInt();
			Accessor& position_accessor = accessors[position_accessor_id];
			assert((position_accessor.count + assets_info->vertex_offset) < Resources::vertex_buffer_size && "The vertices buffer is full");

			BufferView& position_buffer_view = buffer_views[position_accessor.buffer_view_id];

			assert(position_accessor.type == Type::Vec3);

			if (position_accessor.component_type == ComponentType::Float)
			{
				position_buffer = reinterpret_cast<const float *>(&(buffers[position_buffer_view.buffer_id][position_accessor.byte_offset + position_buffer_view.byte_offset]));
			}
			else
			{
				assert(!"Component Type not supported for attribute POSITION");
			}

			// Normal is not mandatory
			if (__attributes.HasMember("NORMAL"))
			{
				uint32_t normal_accessor_id = __attributes["NORMAL"].GetInt();
				Accessor& normal_accessor = accessors[normal_accessor_id];
				BufferView& normal_buffer_view = buffer_views[normal_accessor.buffer_view_id];

				assert(normal_accessor.type == Type::Vec3);

				if (normal_accessor.component_type == ComponentType::Float)
				{
					normal_buffer = reinterpret_cast<const float *>(&(buffers[normal_buffer_view.buffer_id][normal_accessor.byte_offset + normal_buffer_view.byte_offset]));
				}
				else
				{
					assert(!"Component Type not supported for attribute NORMAL");
				}
			}

			// TODO: Add UVs

			for (uint32_t v = 0; v < position_accessor.count; ++v)
			{
				Vertex vertex = {
					{ position_buffer[v * 3], position_buffer[v * 3 + 1], position_buffer[v * 3 + 2] },
					{ 0.0f, 0.0f, 0.0f },
					{ 0.0f, 0.0f }
				};

				if (normal_buffer)
				{
					vertex.normal = { normal_buffer[v * 3], normal_buffer[v * 3 + 1], normal_buffer[v * 3 + 2] };
				}

				assets_info->vertices[primitive.vertex_offset + v] = vertex;
			}

			assets_info->vertex_offset += position_accessor.count;

			//
			// Index Data
			//
			if (__primitive.HasMember("indices"))
			{
				uint32_t index_accessor_id = __primitive["indices"].GetInt();
				Accessor& index_accessor = accessors[index_accessor_id];
				assert((index_accessor.count + assets_info->index_offset) < Resources::index_buffer_size && "The indices buffer is full");
				BufferView& index_buffer_view = buffer_views[index_accessor.buffer_view_id];

				if (index_accessor.component_type == ComponentType::UnsignedShort)
				{
					const Index16* index_buffer = reinterpret_cast<const Index16 *>(&(buffers[index_buffer_view.buffer_id][index_accessor.byte_offset + index_buffer_view.byte_offset]));
					for (uint32_t i = 0; i < index_accessor.count; ++i)
					{
						assets_info->indices[primitive.index_offset + i] = index_buffer[i] + primitive.vertex_offset;
					}

					primitive.index_count = index_accessor.count;
					assets_info->index_offset += primitive.index_count;
				}
				else
				{
					assert(!"Component Type not supported for Indices");
				}
			}

			mesh.primitives[primitive_id] = primitive;
		}

		assets_info->meshes[mesh_offset + mesh_id] = mesh;
	}

	assets_info->mesh_offset += __meshes.Size();

	model.node_count = __root_nodes.Size();

	for (rapidjson::SizeType i = 0; i < __root_nodes.Size(); ++i)
	{
		uint32_t node_index = (uint32_t)__root_nodes[i].GetInt();
		JsonObject __node = __nodes[node_index].GetObject();
		assert(!__node.HasMember("matrix") && "Add matrix support");
		Node node = {};
		node.mesh_id = mesh_offset + __node["mesh"].GetInt();

		if (__node.HasMember("translation"))
		{
			JsonArray __translation = __node["translation"].GetArray();
			node.translation = {
				__translation[0].GetFloat(),
				__translation[1].GetFloat(),
				__translation[2].GetFloat()
			};
		}

		if (__node.HasMember("scale"))
		{
			JsonArray __scale = __node["scale"].GetArray();
			node.scale = {
				__scale[0].GetFloat(),
				__scale[1].GetFloat(),
				__scale[2].GetFloat()
			};
		}

		if (__node.HasMember("rotation"))
		{
			JsonArray __rotation = __node["rotation"].GetArray();
			node.rotation = {
				__rotation[0].GetFloat(),
				__rotation[1].GetFloat(),
				__rotation[2].GetFloat(),
				__rotation[3].GetFloat()
			};
		}

		model.nodes[i] = node;
	}

	return model;
}

}
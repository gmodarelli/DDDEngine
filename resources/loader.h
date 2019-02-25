#pragma once

#include "resources.h"

namespace Resources
{

static const uint32_t GLTF_MAGIC = 0x46546C67;
static const uint32_t GLTF_VERSION = 0x00000002;
static const uint32_t GLTF_CHUNK_TYPE_JSON = 0x4E4F534A;
static const uint32_t GLTF_CHUNK_TYPE_BIN = 0x004E4942;

struct Loader
{
	static Model load_model(const char* path, AssetsInfo* assets_info);
};

}

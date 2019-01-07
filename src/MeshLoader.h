#ifndef MESH_LOADER_H_
#define MESH_LOADER_H_

#include "VulkanTypes.h"
#include <assert.h>
#include "objparser.h"
#include "meshoptimizer.h"

bool LoadMesh(Mesh& mesh, const char* path)
{
	ObjFile file;
	if (!objParseFile(file, path))
		return false;

	size_t indexCount = file.f_size / 3;
	std::vector<Vertex> vertices(indexCount);

	for (size_t i = 0; i < indexCount; ++i)
	{
		Vertex& v = vertices[i];

		int vi = file.f[i * 3 + 0];
		int vti = file.f[i * 3 + 1];
		int vni = file.f[i * 3 + 2];

		v.vx = file.v[vi * 3 + 0];
		v.vy = file.v[vi * 3 + 1];
		v.vz = file.v[vi * 3 + 2];

		v.nx = vni < 0 ? 0.0f : file.vn[vni * 3 + 0];
		v.ny = vni < 0 ? 0.0f : file.vn[vni * 3 + 1];
		v.nz = vni < 0 ? 1.0f : file.vn[vni * 3 + 2];

		v.tu = vti < 0 ? 0.0f : file.vt[vti * 3 + 0];
		v.tv = vti < 0 ? 0.0f : file.vt[vti * 3 + 1];
	}

	if (1)
	{
		// Unoptimized, duplicated vertices

		mesh.Vertices = vertices;
		mesh.Indices.resize(indexCount);

		for (size_t i = 0; i < indexCount; ++i)
		{
			mesh.Indices[i] = i;
		}
	}
	else
	{
		// Unique vertices

		std::vector<uint32_t> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.data(), 0, indexCount, vertices.data(), indexCount, sizeof(Vertex));

		mesh.Vertices.resize(vertexCount);
		mesh.Indices.resize(indexCount);

		meshopt_remapVertexBuffer(mesh.Vertices.data(), vertices.data(), vertexCount, sizeof(Vertex), remap.data());
		meshopt_remapIndexBuffer(mesh.Indices.data(), 0, indexCount, remap.data());

		// TODO: optimize the mesh for more efficient GPU rendering
	}

	return true;
}

#endif // MESH_LOADER_H_
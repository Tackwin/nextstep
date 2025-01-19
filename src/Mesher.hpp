#pragma once

#include "Common.hpp"
#include "Memory.hpp"
#include "AP242.hpp"

struct Mesh {
	struct Vertex {
		float x;
		float y;
		float z;
	};
	struct Vertex_Metadata {
		u32 face_idx = 0xFFFF'FFFF;
		u32 edge_idx = 0xFFFF'FFFF;
	};

	DynArray<Vertex>          vertices;
	DynArray<Vertex_Metadata> vertices_metadata;

	void clear() {
		vertices.clear();
		vertices_metadata.clear();
	}
};

extern void mesh_from_ap242(
	const parse_express_from_memory_result& parsed,
	const A242& a242,
	Mesh& mesh
);

extern void write_obj(const Mesh& mesh, DynArray<u8>& out);

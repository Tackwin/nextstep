#include "Mesher.hpp"
#include "String.hpp"
#include "Math.hpp"
#include "math.h"

void discretize(const A242& a242, const A242::Edge_Curve& edge_curve, Mesh& mesh) {
	const A242::Curve& curve = *edge_curve.edge_geometry;
	const A242::Vertex_Point& start_vertex = *edge_curve.edge_start;
	const A242::Vertex_Point& end_vertex = *edge_curve.edge_end;
	bool is_cartesian = true;
	is_cartesian &= start_vertex.vertex_geometry->id == A242::Cartesian_Point::hash;
	is_cartesian &= start_vertex.vertex_geometry->id == A242::Cartesian_Point::hash;

	Vector3f start(
		((const A242::Cartesian_Point*)start_vertex.vertex_geometry)->x,
		((const A242::Cartesian_Point*)start_vertex.vertex_geometry)->y,
		((const A242::Cartesian_Point*)start_vertex.vertex_geometry)->z
	);
	Vector3f end(
		((const A242::Cartesian_Point*)end_vertex.vertex_geometry)->x,
		((const A242::Cartesian_Point*)end_vertex.vertex_geometry)->y,
		((const A242::Cartesian_Point*)end_vertex.vertex_geometry)->z
	);
	if (!is_cartesian) {
		// >TODO_MESH
		return;
	}
	
	if (curve.id == A242::Circle::hash) {
		const A242::Circle& circle = (const A242::Circle&)curve;
		const A242::Axis2_Placement_3d& placement = *circle.position;
		Vector3f center(placement.location->x, placement.location->y, placement.location->z);
		Vector3f p1(1, 0, 0);
		Vector3f p2(0, 1, 0);
		Vector3f p3(0, 0, 1);

		Vector3f axis(0, 0, 1);
		Vector3f ref(1, 0, 0);

		if (placement.axis)
			axis = Vector3f(placement.axis->x, placement.axis->y, placement.axis->z);
		if (placement.ref_direction)
			ref = Vector3f(
				placement.ref_direction->x, placement.ref_direction->y, placement.ref_direction->z
			);

		p1 = project_point_into_plane(p1, axis);
		p2 = axis <cross> p1;
		p3 = p1 <cross> p2;

		Vector3f center_to_start = center - start;
		Vector3f center_to_end = center - end;

		float angle_start = center_to_start <angle_between_unit> p1;
		float angle_end = center_to_end <angle_between_unit> p1;

		if (angle_start >= angle_end) {
			angle_end += 2 * PI;
		}

		constexpr size_t N = 32;
		for (size_t i = 0; i < N; i += 1) {
			float angle = angle_start + (angle_end - angle_start) * i / N;
			Vector3f point = center + cos(angle) * p1 + sin(angle) * p2;
			mesh.vertices.push({ point.x, point.y, point.z });
		}
	} else if (curve.id == A242::Line::hash) {
		const A242::Line& line = (const A242::Line&)curve;
		Vector3f o(line.point->x, line.point->y, line.point->z);
		Vector3f d(
			line.line_direction->orientation->x,
			line.line_direction->orientation->y,
			line.line_direction->orientation->z
		);

		float t = (start - o) <dot> d;
		float u = (end - o) <dot> d;

		constexpr size_t N = 32;
		for (size_t i = 0; i < N; i += 1) {
			float t_ = t + (u - t) * i / N;
			Vector3f point = o + t_ * d;
			mesh.vertices.push({ point.x, point.y, point.z });
		}
	} else {
		print("Unknown curve type ");
		print(curve.name);
		print("\n");
	}

}

void mesh_from_ap242(
	const parse_express_from_memory_result& parsed,
	const A242& a242,
	Mesh& mesh
) {

	for (size_t i = 0; i < a242.instance_name_to_items.size; i += 1) {
		if (!a242.instance_name_to_items[i])
			continue;

		if (auto ptr = lookup<A242::Edge_Curve>(parsed, a242, i); ptr) {
			discretize(a242, *ptr, mesh);
			// mesh.vertices.push(vertex);
		}
	}
}

void write_obj(const Mesh& mesh, DynArray<u8>& out) {
	Write_String str;
	u8 buf[64];
	str.data = buf;
	str.size = sizeof(buf);
	
	append(out, "o mesh\n");
	for (size_t i = 0; i < mesh.vertices.size; i += 1) {
		const Mesh::Vertex& vertex = mesh.vertices[i];
		append(out, "v ");
		str.size = 64;
		to_string((i64)(vertex.x * 10'000), str);
		append(out, { str.data, str.size });
		append(out, " ");
		str.size = 64;
		to_string((i64)(vertex.y * 10'000), str);
		append(out, { str.data, str.size });
		append(out, " ");
		str.size = 64;
		to_string((i64)(vertex.z * 10'000), str);
		append(out, { str.data, str.size });
		append(out, "\n");
	}
}

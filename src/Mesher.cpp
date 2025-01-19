#include "Mesher.hpp"
#include "String.hpp"
#include "Math.hpp"
#include "math.h"

void discretize(const A242& a242, const A242::Edge_Curve& edge_curve, Mesh& mesh) {
	const A242::Curve& curve = *edge_curve.edge_geometry;
	const A242::Vertex_Point& start = *edge_curve.edge_start;
	const A242::Vertex_Point& end = *edge_curve.edge_end;
	
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
		p3 = axis <cross> p2;

		bool is_cartesian = true;
		is_cartesian &= start.vertex_geometry->id == A242::Cartesian_Point::hash;
		is_cartesian &= end.vertex_geometry->id == A242::Cartesian_Point::hash;
		if (is_cartesian) {
			Vector3f s(
				((const A242::Cartesian_Point*)start.vertex_geometry)->x,
				((const A242::Cartesian_Point*)start.vertex_geometry)->y,
				((const A242::Cartesian_Point*)start.vertex_geometry)->z
			);
			Vector3f e(
				((const A242::Cartesian_Point*)end.vertex_geometry)->x,
				((const A242::Cartesian_Point*)end.vertex_geometry)->y,
				((const A242::Cartesian_Point*)end.vertex_geometry)->z
			);

			Vector3f center_to_start = center - s;
			Vector3f center_to_end = center - e;

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
		}

		// const A242::Axis2_Placement_3d& placement = *circle.position;
		// const A242::Cartesian_Point& center = *placement.location;
		// const A242::Direction& normal = *placement.axis;
		// const A242::Direction& tangent = *placement.ref_direction;
		// const A242::Direction& binormal = cross(normal, tangent);
		// const float radius = circle.radius;
		
		// const size_t n = 32;
		// const float step = 2 * PI / n;
		// for (size_t i = 0; i < n; i += 1) {
		// 	const float angle = i * step;
		// 	const float x = center.x + radius * (cos(angle) * tangent.x + sin(angle) * binormal.x);
		// 	const float y = center.y + radius * (cos(angle) * tangent.y + sin(angle) * binormal.y);
		// 	const float z = center.z + radius * (cos(angle) * tangent.z + sin(angle) * binormal.z);
		// 	mesh.vertices.push({ x, y, z });
		// }
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

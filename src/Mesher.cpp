#include "Mesher.hpp"
#include "String.hpp"
#include "Math.hpp"
#include "math.h"

Vector3f evaluate_bspline(
	const View<Vector3f>& control_points,
	const View<f32>& knots,
	u32 degree,
	float t
) {
	auto basis = [&] (auto& basis, u32 i, u32 p, f32 u) -> f32 {
		if (p == 0) {
			if (knots[i] <= u && u < knots[i + 1]) {
				return 1.0f;
			} else {
				return 0.0f;
			}
		}

		f32 left = basis(basis, i, p - 1, u);
		f32 right = basis(basis, i + 1, p - 1, u);
		f32 left_coeff = 0.0f;
		f32 right_coeff = 0.0f;
		if (knots[i + p] != knots[i]) {
			left_coeff = (u - knots[i]) / (knots[i + p] - knots[i]);
		}
		if (knots[i + p + 1] != knots[i + 1]) {
			right_coeff = (knots[i + p + 1] - u) / (knots[i + p + 1] - knots[i + 1]);
		}
		return left_coeff * left + right_coeff * right;
	};

	Vector3f point = { 0, 0, 0 };
	for (size_t i = 0; i < control_points.size; i += 1) {
		f32 b = basis(basis, i, degree, t);
		point += b * control_points[i];
	}
	return point;
}
void discretize(const A242& a242, const A242::Advanced_Face& advanced_face, Mesh& mesh) {
	A242::Surface* surface = (A242::Surface*)advanced_face.face_geometry;
	if (!surface)
		return;

	if (surface->id == A242::Plane::hash) {
		A242::Plane& plane = (A242::Plane&)*surface;
		Vector3f center;
		Vector3f normal;
		center.x = plane.position->location->x;
		center.y = plane.position->location->y;
		center.z = plane.position->location->z;
		normal.x = plane.position->axis->x;
		normal.y = plane.position->axis->y;
		normal.z = plane.position->axis->z;

		print("Plane\n");
	} else {
		print("Unsupported surface type\n");
	}
}

void discretize(const A242& a242, const A242::Edge_Curve& edge_curve, Mesh& mesh) {
	const A242::Curve& curve = *edge_curve.edge_geometry;
	const A242::Vertex_Point& start_vertex = *edge_curve.edge_start;
	const A242::Vertex_Point& end_vertex = *edge_curve.edge_end;
	bool is_cartesian = true;
	is_cartesian &= start_vertex.vertex_geometry->id == A242::Cartesian_Point::hash;
	is_cartesian &= end_vertex.vertex_geometry->id == A242::Cartesian_Point::hash;

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
		print("Non cartesian\n");
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

		if (!edge_curve.same_sense) {
			float temp = angle_start;
			angle_start = angle_end;
			angle_end = temp;
		}

		if (angle_start >= angle_end) {
			angle_end += 2 * PI;
		}

		constexpr size_t N = 32;
		for (size_t i = 0; i < N; i += 1) {
			float angle = angle_start + (angle_end - angle_start) * i / (N - 1);
			float c = cos(angle);
			float s = sin(angle);
			Vector3f point = center + c * p1 + s * p2;
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
			float t_ = t + (u - t) * i / (N - 1);
			Vector3f point = o + t_ * d;
			mesh.vertices.push({ point.x, point.y, point.z });
		}
	} else if (curve.id == A242::B_Spline_Curve_With_Knots::hash) {
		auto bspline = ((const A242::B_Spline_Curve_With_Knots&)curve);
		View<A242::Cartesian_Point*> control_points = bspline.control_points_list;

		u32 k = control_points.size - 1;
		u32 d = bspline.degree;
		
		f32* mults_knots = talloc<f32>(k + d + 2);
		Vector3f* control_points_out = talloc<Vector3f>(control_points.size);
		for (size_t i = 0; i < control_points.size; i += 1) {
			control_points_out[i] = Vector3f(
				control_points[i]->x, control_points[i]->y, control_points[i]->z
			);
		}
		for (size_t i = 0, cursor = 0; i < bspline.knot_multiplicities.size; i += 1) {
			for (size_t j = 0; j < bspline.knot_multiplicities[i]; j += 1, cursor += 1) {
				mults_knots[cursor] = bspline.knots[i];
			}
		}

		for (size_t i = 0; i < 32; i += 1) {
			float t = mults_knots[0] + (mults_knots[k + d + 1] - mults_knots[0]) * i / 31;
			Vector3f point = evaluate_bspline(
				{ control_points_out, control_points.size },
				{ mults_knots, k + d + 2 },
				bspline.degree,
				t
			);
			mesh.vertices.push({ point.x, point.y, point.z });
		}
	} else {
		print("Unknown curve type ");
		print(fromcstr<Read_String>(curve.type_name));
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

		// if (auto ptr = lookup<A242::Edge_Curve>(parsed, a242, i); ptr) {
		// 	discretize(a242, *ptr, mesh);
		// }
		if (auto ptr = lookup<A242::Advanced_Face>(parsed, a242, i); ptr) {
			discretize(a242, *ptr, mesh);
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
		to_string(vertex.x, str);
		append(out, { str.data, str.size });
		append(out, " ");
		str.size = 64;
		to_string(vertex.y, str);
		append(out, { str.data, str.size });
		append(out, " ");
		str.size = 64;
		to_string(vertex.z, str);
		append(out, { str.data, str.size });
		append(out, "\n");
	}
}

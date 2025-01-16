#include "AP242.hpp"
#include "Platform.hpp"

template<typename T>
T* get(
	const parse_express_from_memory_result& out,
	A242& a242,
	size_t id,
	T* (compile_f)(const parse_express_from_memory_result&, A242&, Read_String, size_t)
) {
	if (id >= a242.instance_name_to_items.size)
		return nullptr;
	
	u32** items = &a242.instance_name_to_items[id];
	if (!*items) {
		size_t node = a242.instance_name_to_node[id];
		if (node == 0)
			return nullptr;

		const Node& entity_instance = out.nodes[node];
		if (entity_instance.kind != Node::Kind::ENTITY_INSTANCE)
			return nullptr;

		if (entity_instance.entity_instance.simple_record) {
			const Node& record = out.nodes[*entity_instance.entity_instance.simple_record];
			const Token& token = out.tokens[record.simple_record.keyword_token];
			*items = (u32*)compile_f(out, a242, token.text, record.simple_record.parameters);
		}
	}

	return (T*)*items;
}
#define get_call(x, out, a242, id) get<A242::x>(out, a242, id, compile_##x)

template<typename T>
bool is_type(const parse_express_from_memory_result& out, A242& a242, size_t instance_name) {
	if (instance_name >= a242.instance_name_to_node.size)
		return false;

	size_t node = a242.instance_name_to_node[instance_name];
	if (node == 0)
		return false;

	const Node& entity_instance = out.nodes[node];
	if (entity_instance.kind != Node::Kind::ENTITY_INSTANCE)
		return false;

	if (entity_instance.entity_instance.simple_record) {
		const Node& record = out.nodes[*entity_instance.entity_instance.simple_record];
		const Token& token = out.tokens[record.simple_record.keyword_token];
		u32 hash = (u32)case_insenstive_hash(token.text);
		return hash == T::hash;
	} else {
		// Idk what to do here
	}
	return false;
}

#define compile_signature(x) A242::x* compile_##x(\
	const parse_express_from_memory_result& out, A242& a242, Read_String type, size_t parameters\
)


compile_signature(Cartesian_Point) {
	if (type != "CARTESIAN_POINT") {
		// cylindrical_point
		// polar_point
		// spherical_point

		return nullptr;
	}

	A242::Cartesian_Point point;

	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 2)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	if (name.kind == Node::Kind::STRING) {
		point.name = name.string;
	}

	const Node& number_list = out.nodes[param_list.list[1]];
	if (number_list.kind != Node::Kind::LIST)
		return nullptr;

	for (size_t j = 0; j < number_list.list.size; j += 1) {
		const Node& param = out.nodes[number_list.list[j]];
		if (param.kind != Node::Kind::NUMBER)
			continue;

		double n = param.number;

		if (j == 0)
			point.x = (float)param.number;
		if (j == 1)
			point.y = (float)param.number;
		if (j == 2)
			point.z = (float)param.number;
	}

	A242::Cartesian_Point* ptr = a242.arena.take<A242::Cartesian_Point>(xstd::move(point));
	a242.cartesian_points.push(ptr);
	return ptr;
}
compile_signature(Point) {
	if (type != "POINT") {
		if (auto ptr = compile_Cartesian_Point(out, a242, type, parameters); ptr)
			return (A242::Point*)ptr;
		return nullptr;
	}

	A242::Point point;

	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 1)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	if (name.kind == Node::Kind::STRING) {
		point.name = name.string;
	}

	A242::Point* ptr = a242.arena.take<A242::Point>(xstd::move(point));
	a242.points.push(ptr);
	return ptr;
}
compile_signature(Direction) {
	A242::Direction direction;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 2)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	if (name.kind == Node::Kind::STRING) {
		direction.name = name.string;
	}

	const Node& number_list = out.nodes[param_list.list[1]];
	if (number_list.kind != Node::Kind::LIST)
		return nullptr;
	
	for (size_t j = 0; j < number_list.list.size; j += 1) {
		const Node& param = out.nodes[number_list.list[j]];
		if (param.kind != Node::Kind::NUMBER)
			continue;
		
		if (j == 0)
			direction.x = (float)param.number;
		if (j == 1)
			direction.y = (float)param.number;
		if (j == 2)
			direction.z = (float)param.number;
	}

	A242::Direction* ptr = a242.arena.take<A242::Direction>(xstd::move(direction));
	a242.directions.push(ptr);
	return ptr;
}
compile_signature(Vertex_Point) {
	A242::Vertex_Point vertex_point;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 2)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	if (name.kind == Node::Kind::STRING) {
		vertex_point.name = name.string;
	}

	const Node& point_node = out.nodes[param_list.list[1]];
	vertex_point.vertex_geometry = get_call(Point, out, a242, point_node.integer);
	if (!vertex_point.vertex_geometry)
		return nullptr;

	A242::Vertex_Point* ptr = a242.arena.take<A242::Vertex_Point>(xstd::move(vertex_point));
	a242.vertex_points.push(ptr);
	return ptr;
}

compile_signature(Axis2_Placement_3d) {
	A242::Axis2_Placement_3d placement;

	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size < 2)
		return nullptr;
	
	{
		const Node& name = out.nodes[param_list.list[0]];
		if (name.kind == Node::Kind::STRING) {
			placement.name = name.string;
		} else {
			return nullptr;
		}
	}

	{
		const Node& location_node = out.nodes[param_list.list[1]];
		if (location_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		placement.location = get_call(Cartesian_Point, out, a242, location_node.integer);
		if (!placement.location)
			return nullptr;
	}

	if (param_list.list.size >= 3) {
		const Node& axis_node = out.nodes[param_list.list[2]];
		if (axis_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		placement.axis = get_call(Direction, out, a242, axis_node.integer);
	} else {
		// Idk what to do here
	}

	if (param_list.list.size >= 4) {
		const Node& ref_direction_node = out.nodes[param_list.list[3]];
		if (ref_direction_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		placement.ref_direction = get_call(Direction, out, a242, ref_direction_node.integer);
	} else {
		// Idk what to do here
	}

	A242::Axis2_Placement_3d* ptr = a242.arena.take<A242::Axis2_Placement_3d>(
		xstd::move(placement)
	);
	a242.axis2_placement_3ds.push(ptr);
	return ptr;
}
compile_signature(Circle) {
	A242::Circle circle;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[param_list.list[0]];
	circle.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& placement_node = out.nodes[param_list.list[1]];
	circle.position = get_call(Axis2_Placement_3d, out, a242, placement_node.integer);
	if (!circle.position)
		return nullptr;
	
	const Node& radius_node = out.nodes[param_list.list[2]];
	circle.radius = (float)radius_node.number;
	if (radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	A242::Circle* ptr = a242.arena.take<A242::Circle>(xstd::move(circle));
	a242.circles.push(ptr);
	return ptr;
}

compile_signature(Conic) {
	if (type != "CONIC") {
		if (auto ptr = compile_Circle(out, a242, type, parameters); ptr)
			return (A242::Conic*)ptr;
		// ellipse,
		// hyperbola,
		// parabola

		return nullptr;
	}

	print("A conic by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Vector) {
	if (type != "VECTOR") {
		return nullptr;
	}

	A242::Vector vector;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 3)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	vector.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& orientation_node = out.nodes[param_list.list[1]];
	vector.orientation = get_call(Direction, out, a242, orientation_node.integer);
	if (!vector.orientation)
		return nullptr;

	const Node& magnitude_node = out.nodes[param_list.list[2]];
	vector.magnitude = (float)magnitude_node.number;
	if (magnitude_node.kind != Node::Kind::NUMBER)
		return nullptr;
	
	A242::Vector* ptr = a242.arena.take<A242::Vector>(xstd::move(vector));
	a242.vectors.push(ptr);
	return ptr;
}

compile_signature(Line);

compile_signature(Curve) {
	if (type != "CURVE") {
		if (auto ptr = compile_Line(out, a242, type, parameters); ptr)
			return (A242::Curve*)ptr;
		if (auto ptr = compile_Conic(out, a242, type, parameters); ptr)
			return (A242::Curve*)ptr;
		// conic,
		// clothoid,
		// circular_involute,
		// pcurve,
		// surface_curve,
		// offset_curve_2d,
		// offset_curve_3d,
		// curve_replica
		return nullptr;
	}
	
	print("A curve by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Oriented_Edge);
compile_signature(Edge_Curve);

compile_signature(Edge) {
	if (type != "EDGE") {
		if (auto ptr = compile_Oriented_Edge(out, a242, type, parameters); ptr)
			return (A242::Edge*)ptr;
		if (auto ptr = compile_Edge_Curve(out, a242, type, parameters); ptr)
			return (A242::Edge*)ptr;
		// subedge
	}
	return nullptr;
}

compile_signature(Oriented_Edge) {
	if (type != "ORIENTED_EDGE") {
		return nullptr;
	}
	A242::Oriented_Edge oriented_edge;

	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 5)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	oriented_edge.name = name.string;

	const Node& edge_start = out.nodes[param_list.list[1]];
	// omitted

	const Node& edge_end = out.nodes[param_list.list[2]];
	// omitted

	const Node& edge_geometry = out.nodes[param_list.list[3]];
	oriented_edge.edge_element = get_call(Edge, out, a242, edge_geometry.integer);
	if (!oriented_edge.edge_element)
		return nullptr;

	const Node& orientation = out.nodes[param_list.list[4]];
	oriented_edge.orientation = orientation.enumeration == "T";
	if (orientation.kind != Node::Kind::ENUMERATION)
		return nullptr;

	A242::Oriented_Edge* ptr = a242.arena.take<A242::Oriented_Edge>(xstd::move(oriented_edge));
	a242.oriented_edges.push(ptr);
	return ptr;
}

compile_signature(Edge_Loop);
compile_signature(Loop) {
	if (type != "LOOP") {
		if (auto ptr = compile_Edge_Loop(out, a242, type, parameters); ptr)
			return (A242::Loop*)ptr;
		// vertex_loop,
		// poly_loop,

		return nullptr;
	}

	return nullptr;
}

compile_signature(Face_Bound) {
	if (type != "FACE_BOUND") {
		return nullptr;
	}

	A242::Face_Bound face_bound;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[param_list.list[0]];
	face_bound.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& bound_node = out.nodes[param_list.list[1]];
	face_bound.bound = get_call(Loop, out, a242, bound_node.integer);
	if (!face_bound.bound)
		return nullptr;

	const Node& orientation_node = out.nodes[param_list.list[2]];
	face_bound.orientation = orientation_node.enumeration == "T";
	if (orientation_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Face_Bound* ptr = a242.arena.take<A242::Face_Bound>(xstd::move(face_bound));
	a242.face_bounds.push(ptr);
	return ptr;
}

compile_signature(Line) {
	if (type != "LINE") {
		return nullptr;
	}

	A242::Line line;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 3)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	line.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& point_node = out.nodes[param_list.list[1]];
	line.point = get_call(Cartesian_Point, out, a242, point_node.integer);
	if (!line.point)
		return nullptr;

	const Node& direction_node = out.nodes[param_list.list[2]];
	line.line_direction = get_call(Vector, out, a242, direction_node.integer);
	if (!line.line_direction)
		return nullptr;

	A242::Line* ptr = a242.arena.take<A242::Line>(xstd::move(line));
	a242.lines.push(ptr);
	return ptr;
}

compile_signature(Edge_Loop) {
	if (type != "EDGE_LOOP") {
		return nullptr;
	}

	A242::Edge_Loop edge_loop;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 2)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	edge_loop.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& edge_list = out.nodes[param_list.list[1]];
	if (edge_list.kind != Node::Kind::LIST)
		return nullptr;

	size_t n = edge_list.list.size;
	edge_loop.edge_list.data = a242.arena.alloc<A242::Oriented_Edge*>(n);
	edge_loop.edge_list.size = n;

	for (size_t i = 0; i < n; i += 1) {
		const Node& edge_node = out.nodes[edge_list.list[i]];
		edge_loop.edge_list.data[i] = get_call(Oriented_Edge, out, a242, edge_node.integer);
		if (!edge_loop.edge_list.data[i])
			return nullptr;
	}

	A242::Edge_Loop* ptr = a242.arena.take<A242::Edge_Loop>(xstd::move(edge_loop));
	a242.edge_loops.push(ptr);
	return ptr;
}

compile_signature(Edge_Curve) {
	// Not a supertype
	// if (type != "EDGE_CURVE") {
	// 	return nullptr;
	// }

	A242::Edge_Curve edge_curve;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 5)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	edge_curve.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& edge_start_node = out.nodes[param_list.list[1]];
	edge_curve.edge_start = get_call(Vertex_Point, out, a242, edge_start_node.integer);
	if (!edge_curve.edge_start)
		return nullptr;
	
	const Node& edge_end_node = out.nodes[param_list.list[2]];
	edge_curve.edge_end = get_call(Vertex_Point, out, a242, edge_end_node.integer);
	if (!edge_curve.edge_end)
		return nullptr;

	const Node& edge_geometry_node = out.nodes[param_list.list[3]];
	edge_curve.edge_geometry = get_call(Curve, out, a242, edge_geometry_node.integer);
	if (!edge_curve.edge_geometry)
		return nullptr;

	const Node& same_sense_node = out.nodes[param_list.list[4]];
	edge_curve.same_sense = same_sense_node.enumeration == "T";
	if (same_sense_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Edge_Curve* ptr = a242.arena.take<A242::Edge_Curve>(xstd::move(edge_curve));
	a242.edge_curves.push(ptr);
	return ptr;
}

compile_signature(Cylindrical_Surface) {
	A242::Cylindrical_Surface surface;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 3)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	surface.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& placement_node = out.nodes[param_list.list[1]];
	surface.position = get_call(Axis2_Placement_3d, out, a242, placement_node.integer);
	if (!surface.position)
		return nullptr;

	const Node& radius_node = out.nodes[param_list.list[2]];
	surface.radius = (float)radius_node.number;
	if (radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	A242::Cylindrical_Surface* ptr =
		a242.arena.take<A242::Cylindrical_Surface>(xstd::move(surface));
	a242.cylindrical_surfaces.push(ptr);
	return ptr;
}

compile_signature(Plane) {
	A242::Plane plane;
	const Node& param_list = out.nodes[parameters];
	if (param_list.list.size != 2)
		return nullptr;

	const Node& name = out.nodes[param_list.list[0]];
	if (name.kind == Node::Kind::STRING) {
		plane.name = name.string;
	}

	const Node& location_node = out.nodes[param_list.list[1]];
	if (location_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
		return nullptr;

	plane.position = get_call(Axis2_Placement_3d, out, a242, location_node.integer);

	A242::Plane* ptr = a242.arena.take<A242::Plane>(xstd::move(plane));
	a242.planes.push(ptr);
	return ptr;
}
void compile_express_from_memory(const parse_express_from_memory_result& out, A242& a242) {
	auto_release_scratch();

	if (false) for (size_t i = 0; i < out.nodes.size; i += 1) {
		const Node& node = out.nodes[i];
		if (node.kind != Node::Kind::ENTITY_INSTANCE)
			continue;
			
		const Node& name = out.nodes[node.entity_instance.entity_instance_name];
		if (node.entity_instance.simple_record.has_value) {
			const Node& record = out.nodes[*node.entity_instance.simple_record];
			const Token& token = out.tokens[record.simple_record.keyword_token];

			print(name.integer);
			print(" ");
			print(token.text);
			print("\n");
		}
		if (node.entity_instance.subsuper_record.has_value) {
			const Node& subsuper = out.nodes[*node.entity_instance.subsuper_record];
			for (size_t j = 0; j < subsuper.list.size; j += 1) {
				const Node& record = out.nodes[subsuper.list[j]];
				const Token& token = out.tokens[record.simple_record.keyword_token];

				print("  ");
				print(name.integer);
				print(" ");
				print(token.text);
				print("\n");
			}
		}
	}

	size_t max_instance_name = 0;
	for (size_t i = 0; i < out.nodes.size; i += 1) {
		const Node& node = out.nodes[i];
		if (node.kind != Node::Kind::ENTITY_INSTANCE)
			continue;

		size_t id = out.nodes[node.entity_instance.entity_instance_name].integer;
		if (id > max_instance_name)
			max_instance_name = id;
	}

	a242.instance_name_to_items = Fixed_Array<u32*>(max_instance_name + 1);
	memset(a242.instance_name_to_items.data, 0, a242.instance_name_to_items.size * sizeof(u32*));

	a242.instance_name_to_node = Fixed_Array<size_t>(max_instance_name + 1);
	for (size_t i = 0; i < out.nodes.size; i += 1) {
		const Node& node = out.nodes[i];
		if (node.kind != Node::Kind::ENTITY_INSTANCE)
			continue;

		size_t id = out.nodes[node.entity_instance.entity_instance_name].integer;
		a242.instance_name_to_node[id] = i;
	}

	for (size_t i = 1; i < max_instance_name; i += 1) {
		if (is_type<A242::Plane>(out, a242, i)) {
			get_call(Plane, out, a242, i);
		}
		if (is_type<A242::Face_Bound>(out, a242, i)) {
			get_call(Face_Bound, out, a242, i);
		}
		if (is_type<A242::Cylindrical_Surface>(out, a242, i)) {
			get_call(Cylindrical_Surface, out, a242, i);
		}
	}

	for (size_t i = 0; i < a242.edge_loops.size; i += 1) {
		A242::Edge_Loop* edge_loop = a242.edge_loops[i];
		print("Edge Loop: ");
		print(edge_loop->name);

		print("(");
		for (size_t j = 0; j < edge_loop->edge_list.size; j += 1) {
			print(" ");
			A242::Oriented_Edge* oriented_edge = edge_loop->edge_list.data[j];
			print("Oriented Edge: ");
			print(oriented_edge->name);
			print(" ");
		}
		print(")\n");
	}

}

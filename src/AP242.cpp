#include "AP242.hpp"
#include "Platform.hpp"


void report_error(A242& res, Read_String msg) {
	// if (msg.size == 0)
	// 	print("Unknown error\n");
	res.diagnostic.push(msg);
}

struct compile_feedback_t {
	bool error_type = false;
};
template<typename T>
T* get(
	const parse_express_from_memory_result& out,
	A242& a242,
	size_t id,
	T* (compile_f)(
		const parse_express_from_memory_result&,
		A242&,
		Read_String,
		const View<const size_t>,
		compile_feedback_t*
	)
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
			const Node& parameter_node = out.nodes[record.simple_record.parameters];
			*items = (u32*)compile_f(out, a242, token.text, parameter_node.list.view(), nullptr);
		}
		if (entity_instance.entity_instance.subsuper_record) {
			const Node& record = out.nodes[*entity_instance.entity_instance.subsuper_record];
			*items = (u32*)compile_f(out, a242, {}, record.list.view(), nullptr);
		}
	}

	return (T*)*items;
}
#define get_call(x, out, a242, id) get<A242::x>(out, a242, id, compile_##x)

#define enum_to_str(assign, e, text, name) if (text == name) assign = e;

template<typename T>
bool is_type(const parse_express_from_memory_result& out, const A242& a242, size_t instance_name) {
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

template<size_t N>
View<const size_t> flatten_complex(
	const parse_express_from_memory_result& out,
	const char* types[N],
	const View<const size_t> entities
) {
	size_t n = 0;
	for (size_t i = 0; i < entities.size; i += 1) {
		size_t entity_node_index = entities[i];
		const Node& record = out.nodes[entity_node_index];
		if (record.kind != Node::Kind::SIMPLE_RECORD)
			continue;
		
		const Node& parameters = out.nodes[record.simple_record.parameters];
		n += parameters.list.size;
	}

	size_t* out_entities = talloc<size_t>(n);
	n = 0;
	for (size_t i = 0; i < N; i += 1) {
		const char* type = types[i];

		bool found = false;
		// Loop through all entities and find the one with the right type
		for (size_t j = 0; j < entities.size; j += 1) {
			size_t entity_node_index = entities[j];
			const Node& record = out.nodes[entity_node_index];
			if (record.kind != Node::Kind::SIMPLE_RECORD)
				continue;
			
			const Token& token = out.tokens[record.simple_record.keyword_token];
			if (token.text == type) {
				const Node& parameters = out.nodes[record.simple_record.parameters];
				for (size_t k = 0; k < parameters.list.size; k += 1) {
					out_entities[n++] = parameters.list[k];
				}
				found = true;
				break;
			}
		}

		if (!found) {
			return {};
		}
	}

	return { (const size_t*)out_entities, n };
}

xstd::optional<A242::Knot_Type> get_knot_type(
	const parse_express_from_memory_result& out, Read_String text
) {
	if (text == "UNIFORM_KNOTS")
		return A242::Knot_Type::Uniform_Knots;
	else if (text == "QUASI_UNIFORM_KNOTS")
		return A242::Knot_Type::Quasi_Uniform_Knots;
	else if (text == "PIECEWISE_BEZIER_KNOTS")
		return A242::Knot_Type::Piecewise_Bezier_Knots;
	else if (text == "UNSPECIFIED")
		return A242::Knot_Type::Unspecified;
	else
		return xstd::nullopt;
}

xstd::optional<float> get_number(const parse_express_from_memory_result& out, size_t id) {
	if (id >= out.nodes.size)
		return xstd::nullopt;
	const Node& node = out.nodes[id];
	if (node.kind != Node::Kind::NUMBER)
		return xstd::nullopt;
	return (float)node.number;
}

xstd::optional<bool> get_bool(const parse_express_from_memory_result& out, size_t id) {
	if (id >= out.nodes.size)
		return xstd::nullopt;
	const Node& node = out.nodes[id];
	if (node.kind != Node::Kind::ENUMERATION)
		return xstd::nullopt;
	return { node.enumeration == "T" };
}

#define is_type_decl(x)\
template<> bool is_type<A242::x>(const parse_express_from_memory_result&, const A242&, size_t)


#define compile_signature(x) A242::x* compile_##x(\
	const parse_express_from_memory_result& out,\
	A242& a242,\
	Read_String type,\
	const View<const size_t> parameters,\
	compile_feedback_t* feedback\
)

#define report_error_subtype(x)\
	if (feedback) feedback->error_type = true;\
	report_error(a242, #x " subtype not handled");\
	report_error(a242, type);

#define subtype_check_begin(type_name)\
	if (type != #type_name) {\
		size_t before_diagnostic_size = a242.diagnostic.size;

#define subtype_check_case(type_, subtype)\
	{\
		compile_feedback_t subfeedback;\
		auto ptr = compile_##subtype(out, a242, type, parameters, &subfeedback);\
		if (!subfeedback.error_type) {\
			if (ptr) a242.diagnostic.size = before_diagnostic_size;\
			return (A242::type_*)ptr;\
		}\
	}

#define subtype_check_end(type)\
		report_error_subtype(type);\
		return nullptr;\
	}


compile_signature(Cartesian_Point) {
	subtype_check_begin(CARTESIAN_POINT);
		// cylindrical_point
		// polar_point
		// spherical_point
	subtype_check_end(Cartesian_Point);

	A242::Cartesian_Point point;
	point.type_name = "CARTESIAN_POINT";

	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind == Node::Kind::STRING) {
		point.name = name.string;
	}

	const Node& number_list = out.nodes[parameters[1]];
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

is_type_decl(Cartesian_Point);

compile_signature(Point) {
	subtype_check_begin(POINT);
		subtype_check_case(Point, Cartesian_Point);
	subtype_check_end(Point);

	A242::Point point;
	point.type_name = "POINT";

	if (parameters.size != 1)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind == Node::Kind::STRING) {
		point.name = name.string;
	}

	A242::Point* ptr = a242.arena.take<A242::Point>(xstd::move(point));
	a242.points.push(ptr);
	return ptr;
}
compile_signature(Direction) {
	subtype_check_begin(DIRECTION);
	subtype_check_end(Direction);
	A242::Direction direction;
	direction.type_name = "DIRECTION";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind == Node::Kind::STRING) {
		direction.name = name.string;
	}

	const Node& number_list = out.nodes[parameters[1]];
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
	subtype_check_begin(VERTEX_POINT);
	subtype_check_end(Vertex_Point);
	A242::Vertex_Point vertex_point;
	vertex_point.type_name = "VERTEX_POINT";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind == Node::Kind::STRING) {
		vertex_point.name = name.string;
	}

	const Node& point_node = out.nodes[parameters[1]];
	vertex_point.vertex_geometry = get_call(Point, out, a242, point_node.integer);
	if (!vertex_point.vertex_geometry)
		return nullptr;

	A242::Vertex_Point* ptr = a242.arena.take<A242::Vertex_Point>(xstd::move(vertex_point));
	a242.vertex_points.push(ptr);
	return ptr;
}

compile_signature(Axis2_Placement_3d) {
	subtype_check_begin(AXIS2_PLACEMENT_3D);
	subtype_check_end(Axis2_Placement_3d);
	A242::Axis2_Placement_3d placement;
	placement.type_name = "AXIS2_PLACEMENT_3D";

	if (parameters.size < 2)
		return nullptr;
	
	{
		const Node& name = out.nodes[parameters[0]];
		if (name.kind == Node::Kind::STRING) {
			placement.name = name.string;
		} else {
			return nullptr;
		}
	}

	{
		const Node& location_node = out.nodes[parameters[1]];
		if (location_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		placement.location = get_call(Cartesian_Point, out, a242, location_node.integer);
		if (!placement.location)
			return nullptr;
	}

	if (parameters.size >= 3) {
		const Node& axis_node = out.nodes[parameters[2]];
		if (axis_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		placement.axis = get_call(Direction, out, a242, axis_node.integer);
	} else {
		// Idk what to do here
	}

	if (parameters.size >= 4) {
		const Node& ref_direction_node = out.nodes[parameters[3]];
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

compile_signature(Ellipse) {
	subtype_check_begin(ELLIPSE);
	subtype_check_end(Ellipse);
	A242::Ellipse ellipse;
	ellipse.type_name = "ELLIPSE";
	if (parameters.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[parameters[0]];
	ellipse.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& placement_node = out.nodes[parameters[1]];
	ellipse.position = get_call(Axis2_Placement_3d, out, a242, placement_node.integer);
	if (!ellipse.position)
		return nullptr;
	
	const Node& major_radius_node = out.nodes[parameters[2]];
	ellipse.major_radius = (float)major_radius_node.number;
	if (major_radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	const Node& minor_radius_node = out.nodes[parameters[3]];
	ellipse.minor_radius = (float)minor_radius_node.number;
	if (minor_radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	A242::Ellipse* ptr = a242.arena.take<A242::Ellipse>(xstd::move(ellipse));
	a242.ellipses.push(ptr);
	return ptr;
}

compile_signature(Circle) {
	subtype_check_begin(CIRCLE);
	subtype_check_end(Circle);
	A242::Circle circle;
	circle.type_name = "CIRCLE";
	if (parameters.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[parameters[0]];
	circle.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& placement_node = out.nodes[parameters[1]];
	circle.position = get_call(Axis2_Placement_3d, out, a242, placement_node.integer);
	if (!circle.position)
		return nullptr;
	
	const Node& radius_node = out.nodes[parameters[2]];
	circle.radius = (float)radius_node.number;
	if (radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	A242::Circle* ptr = a242.arena.take<A242::Circle>(xstd::move(circle));
	a242.circles.push(ptr);
	return ptr;
}

compile_signature(Conic) {
	subtype_check_begin(CONIC);
		subtype_check_case(Conic, Ellipse);
		// hyperbola,
		// parabola
		subtype_check_case(Conic, Circle);
	subtype_check_end(Conic);

	print("A conic by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Vector) {
	subtype_check_begin(VECTOR);
	subtype_check_end(Vector);

	A242::Vector vector;
	vector.type_name = "VECTOR";
	if (parameters.size != 3)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	vector.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& orientation_node = out.nodes[parameters[1]];
	vector.orientation = get_call(Direction, out, a242, orientation_node.integer);
	if (!vector.orientation)
		return nullptr;

	const Node& magnitude_node = out.nodes[parameters[2]];
	vector.magnitude = (float)magnitude_node.number;
	if (magnitude_node.kind != Node::Kind::NUMBER)
		return nullptr;
	
	A242::Vector* ptr = a242.arena.take<A242::Vector>(xstd::move(vector));
	a242.vectors.push(ptr);
	return ptr;
}

compile_signature(Line);
compile_signature(Bounded_Curve);

compile_signature(Curve) {
	subtype_check_begin(CURVE);
		// conic,
		// clothoid,
		// circular_involute,
		// pcurve,
		// surface_curve,
		// offset_curve_2d,
		// offset_curve_3d,
		// curve_replica
		subtype_check_case(Curve, Bounded_Curve);
		subtype_check_case(Curve, Line);
		subtype_check_case(Curve, Conic);
	subtype_check_end(Curve);

	print("A curve by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Oriented_Edge);
compile_signature(Edge_Curve);

compile_signature(B_Spline_Curve);

compile_signature(Bounded_Curve) {
	subtype_check_begin(BOUNDED_CURVE);
		// bezier_curve,
		subtype_check_case(Bounded_Curve, B_Spline_Curve);
		// offset_curve_2d,
		// offset_curve_3d,
	subtype_check_end(Bounded_Curve);

	print("A bounded curve by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(B_Spline_Curve_With_Knots);

compile_signature(B_Spline_Curve) {
	subtype_check_begin(B_SPLINE_CURVE);
		subtype_check_case(B_Spline_Curve, B_Spline_Curve_With_Knots);
		// >TODO_ITEM: bezier_curve,
		// >TODO_ITEM: quasi_uniform_bspline_curve,
		// >TODO_ITEM: uniform_curve,
	subtype_check_end(B_Spline_Curve);
	A242::B_Spline_Curve b_spline_curve;
	b_spline_curve.type_name = "B_SPLINE_CURVE";

	if (parameters.size != 6)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	b_spline_curve.name = name.string;

	const Node& degree_node = out.nodes[parameters[1]];
	if (degree_node.kind != Node::Kind::NUMBER)
		return nullptr;
	
	b_spline_curve.degree = (u32)degree_node.number;

	const Node& control_points_list = out.nodes[parameters[2]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	// We have to allocate the control points before we can fill them because of possible circular references
	b_spline_curve.control_points_list.size = control_points_list.list.size;
	b_spline_curve.control_points_list.data = a242.arena.alloc<A242::Cartesian_Point*>(
		b_spline_curve.control_points_list.size
	);
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		b_spline_curve.control_points_list.data[i] =
			get_call(Cartesian_Point, out, a242, point_node.integer);
		if (!b_spline_curve.control_points_list.data[i])
			return nullptr;
	}

	const Node& curve_form_node = out.nodes[parameters[3]];
	if (curve_form_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	if (curve_form_node.enumeration == "POLYLINE")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Polyline;
	else if (curve_form_node.enumeration == "CIRCULAR_ARC")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Circular_Arc;
	else if (curve_form_node.enumeration == "ELLIPSE_ARC")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Ellipse_Arc;
	else if (curve_form_node.enumeration == "PARABOLIC_ARC")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Parabolic_Arc;
	else if (curve_form_node.enumeration == "HYPERBOLIC_ARC")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Hyperbolic_Arc;
	else if (curve_form_node.enumeration == "UNSPECIFIED")
		b_spline_curve.curve_form = A242::B_Spline_Curve::Curve_Form::Unspecified;
	else
		return nullptr;

	const Node& closed_curve_node = out.nodes[parameters[4]];
	if (closed_curve_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	b_spline_curve.closed_curve = closed_curve_node.enumeration == "T";

	const Node& self_intersect_node = out.nodes[parameters[5]];
	if (self_intersect_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	b_spline_curve.self_intersect = self_intersect_node.enumeration == "T";

	A242::B_Spline_Curve* ptr = a242.arena.take<A242::B_Spline_Curve>(xstd::move(b_spline_curve));
	a242.b_spline_curves.push(ptr);

	return ptr;
}

compile_signature(B_Spline_Curve_With_Knots) {
	subtype_check_begin(B_SPLINE_CURVE_WITH_KNOTS);
	subtype_check_end(B_Spline_Curve_With_Knots);
	A242::B_Spline_Curve_With_Knots b_spline_curve_with_knots;
	b_spline_curve_with_knots.type_name = "B_SPLINE_CURVE_WITH_KNOTS";

	if (parameters.size != 9)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	b_spline_curve_with_knots.name = name.string;

	const Node& degree_node = out.nodes[parameters[1]];
	if (degree_node.kind != Node::Kind::NUMBER)
		return nullptr;
	
	b_spline_curve_with_knots.degree = (u32)degree_node.number;

	const Node& control_points_list = out.nodes[parameters[2]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	// We have to allocate the control points before we can fill them because of possible circular references
	b_spline_curve_with_knots.control_points_list.size = control_points_list.list.size;
	b_spline_curve_with_knots.control_points_list.data = a242.arena.alloc<A242::Cartesian_Point*>(
		b_spline_curve_with_knots.control_points_list.size
	);
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;

		b_spline_curve_with_knots.control_points_list.data[i] =
			get_call(Cartesian_Point, out, a242, point_node.integer);
		if (!b_spline_curve_with_knots.control_points_list.data[i])
			return nullptr;
	}

	const Node& curve_form_node = out.nodes[parameters[3]];
	if (curve_form_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	if (curve_form_node.enumeration == "POLYLINE")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Polyline;
	else if (curve_form_node.enumeration == "CIRCULAR_ARC")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Circular_Arc;
	else if (curve_form_node.enumeration == "ELLIPSE_ARC")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Ellipse_Arc;
	else if (curve_form_node.enumeration == "PARABOLIC_ARC")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Parabolic_Arc;
	else if (curve_form_node.enumeration == "HYPERBOLIC_ARC")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Hyperbolic_Arc;
	else if (curve_form_node.enumeration == "UNSPECIFIED")
		b_spline_curve_with_knots.curve_form = A242::B_Spline_Curve::Curve_Form::Unspecified;
	else
		return nullptr;

	const Node& closed_curve_node = out.nodes[parameters[4]];
	if (closed_curve_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	b_spline_curve_with_knots.closed_curve = closed_curve_node.enumeration == "T";

	const Node& self_intersect_node = out.nodes[parameters[5]];
	if (self_intersect_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	b_spline_curve_with_knots.self_intersect = self_intersect_node.enumeration == "T";

	const Node& knot_multiplicities_list = out.nodes[parameters[6]];
	if (knot_multiplicities_list.kind != Node::Kind::LIST)
		return nullptr;

	b_spline_curve_with_knots.knot_multiplicities.size = knot_multiplicities_list.list.size;
	b_spline_curve_with_knots.knot_multiplicities.data = a242.arena.alloc<u32>(
		b_spline_curve_with_knots.knot_multiplicities.size
	);
	for (size_t i = 0; i < knot_multiplicities_list.list.size; i += 1) {
		const Node& param = out.nodes[knot_multiplicities_list.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_curve_with_knots.knot_multiplicities.data[i] = (u32)param.number;
	}

	const Node& knots_list = out.nodes[parameters[7]];
	if (knots_list.kind != Node::Kind::LIST)
		return nullptr;

	b_spline_curve_with_knots.knots.size = knots_list.list.size;
	b_spline_curve_with_knots.knots.data = a242.arena.alloc<float>(
		b_spline_curve_with_knots.knots.size
	);
	for (size_t i = 0; i < knots_list.list.size; i += 1) {
		const Node& param = out.nodes[knots_list.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_curve_with_knots.knots.data[i] = (float)param.number;
	}

	const Node& knot_spec_node = out.nodes[parameters[8]];
	if (knot_spec_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	if (!get_knot_type(out, knot_spec_node.enumeration))
		return nullptr;
	b_spline_curve_with_knots.knot_spec = *get_knot_type(out, knot_spec_node.enumeration);

	A242::B_Spline_Curve_With_Knots* ptr = a242.arena.take<A242::B_Spline_Curve_With_Knots>(
		xstd::move(b_spline_curve_with_knots)
	);
	a242.b_spline_curves_with_knots.push(ptr);
	return ptr;
}

compile_signature(Edge) {
	subtype_check_begin(EDGE);
		// subedge
		subtype_check_case(Edge, Oriented_Edge);
		subtype_check_case(Edge, Edge_Curve);
	subtype_check_end(Edge);
	return nullptr;
}

compile_signature(Oriented_Edge) {
	subtype_check_begin(ORIENTED_EDGE);
	subtype_check_end(Oriented_Edge);
	A242::Oriented_Edge oriented_edge;
	oriented_edge.type_name = "ORIENTED_EDGE";

	if (parameters.size != 5)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	oriented_edge.name = name.string;

	const Node& edge_start = out.nodes[parameters[1]];
	// omitted

	const Node& edge_end = out.nodes[parameters[2]];
	// omitted

	const Node& edge_geometry = out.nodes[parameters[3]];
	oriented_edge.edge_element = get_call(Edge, out, a242, edge_geometry.integer);
	if (!oriented_edge.edge_element)
		return nullptr;

	const Node& orientation = out.nodes[parameters[4]];
	oriented_edge.orientation = orientation.enumeration == "T";
	if (orientation.kind != Node::Kind::ENUMERATION)
		return nullptr;

	A242::Oriented_Edge* ptr = a242.arena.take<A242::Oriented_Edge>(xstd::move(oriented_edge));
	a242.oriented_edges.push(ptr);
	return ptr;
}

compile_signature(Edge_Loop);
compile_signature(Loop) {
	subtype_check_begin(LOOP);
		subtype_check_case(Loop, Edge_Loop);
	subtype_check_end(Loop);

	return nullptr;
}

compile_signature(Face_Outer_Bound) {
	subtype_check_begin(FACE_OUTER_BOUND);
	subtype_check_end(Face_Outer_Bound);

	A242::Face_Outer_Bound face_outer_bound;
	face_outer_bound.type_name = "FACE_OUTER_BOUND";
	if (parameters.size != 3)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	face_outer_bound.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& bound_node = out.nodes[parameters[1]];
	face_outer_bound.bound = get_call(Loop, out, a242, bound_node.integer);
	if (!face_outer_bound.bound)
		return nullptr;

	const Node& orientation_node = out.nodes[parameters[2]];
	face_outer_bound.orientation = orientation_node.enumeration == "T";
	if (orientation_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Face_Outer_Bound* ptr =
		a242.arena.take<A242::Face_Outer_Bound>(xstd::move(face_outer_bound));
	// a242.face_outer_bounds.push(ptr);
	return ptr;
}

compile_signature(Face_Bound) {
	subtype_check_begin(FACE_BOUND);
		subtype_check_case(Face_Bound, Face_Outer_Bound);
	subtype_check_end(Face_Bound);

	A242::Face_Bound face_bound;
	face_bound.type_name = "FACE_BOUND";
	if (parameters.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[parameters[0]];
	face_bound.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& bound_node = out.nodes[parameters[1]];
	face_bound.bound = get_call(Loop, out, a242, bound_node.integer);
	if (!face_bound.bound)
		return nullptr;

	const Node& orientation_node = out.nodes[parameters[2]];
	face_bound.orientation = orientation_node.enumeration == "T";
	if (orientation_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Face_Bound* ptr = a242.arena.take<A242::Face_Bound>(xstd::move(face_bound));
	a242.face_bounds.push(ptr);
	return ptr;
}
compile_signature(Closed_Shell);
compile_signature(Connected_Face_Set) {
	subtype_check_begin(CONNECTED_FACE_SET);
		subtype_check_case(Connected_Face_Set, Closed_Shell);
	subtype_check_end(Connected_Face_Set);

	return nullptr;
}

compile_signature(Face);
compile_signature(Closed_Shell) {
	subtype_check_begin(CLOSED_SHELL);
	subtype_check_end(Closed_Shell);

	A242::Closed_Shell closed_shell;
	closed_shell.type_name = "CLOSED_SHELL";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	closed_shell.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& face_list = out.nodes[parameters[1]];
	if (face_list.kind != Node::Kind::LIST)
		return nullptr;
	closed_shell.cfs_faces.size = face_list.list.size;
	closed_shell.cfs_faces.data = a242.arena.alloc<A242::Face*>(closed_shell.cfs_faces.size);
	for (size_t i = 0; i < closed_shell.cfs_faces.size; i += 1) {
		const Node& face_node = out.nodes[face_list.list[i]];
		if (face_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;
		closed_shell.cfs_faces.data[i] = get_call(Face, out, a242, face_node.integer);
		if (!closed_shell.cfs_faces.data[i])
			print("A face in a closed shell was null\n");
		// if (!closed_shell.cfs_faces.data[i])
		// 	return nullptr;
	}

	A242::Closed_Shell* ptr = a242.arena.take<A242::Closed_Shell>(xstd::move(closed_shell));
	a242.closed_shells.push(ptr);
	return ptr;
}

compile_signature(Manifold_Solid_Brep) {
	subtype_check_begin(MANIFOLD_SOLID_BREP);
	subtype_check_end(Manifold_Solid_Brep);

	A242::Manifold_Solid_Brep brep;
	brep.type_name = "MANIFOLD_SOLID_BREP";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	brep.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& outer_node = out.nodes[parameters[1]];
	brep.outer = get_call(Closed_Shell, out, a242, outer_node.integer);
	if (outer_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
		return nullptr;

	A242::Manifold_Solid_Brep* ptr = a242.arena.take<A242::Manifold_Solid_Brep>(xstd::move(brep));
	a242.manifold_solid_breps.push(ptr);
	return ptr;
}

compile_signature(Cylindrical_Surface);
compile_signature(Toroidal_Surface);
compile_signature(Plane);
compile_signature(Conical_Surface);

compile_signature(Elementary_Surface) {
	subtype_check_begin(ELEMENTARY_SURFACE);
		subtype_check_case(Elementary_Surface, Conical_Surface);
		// >TODO_ITEM dupin cyclide surface
		// >TODO_ITEM spherical surface
		// if (type == "CYLINDRICAL_SURFACE")
		// 	print("azeaze");
		subtype_check_case(Elementary_Surface, Toroidal_Surface);
		subtype_check_case(Elementary_Surface, Cylindrical_Surface);
		subtype_check_case(Elementary_Surface, Plane);
	subtype_check_end(Elementary_Surface);
	return nullptr;
}

compile_signature(Conical_Surface) {
	subtype_check_begin(CONICAL_SURFACE);
	subtype_check_end(Conical_Surface);

	A242::Conical_Surface conical_surface;
	conical_surface.type_name = "CONICAL_SURFACE";
	if (parameters.size != 4)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	conical_surface.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& position_node = out.nodes[parameters[1]];
	conical_surface.position = get_call(Axis2_Placement_3d, out, a242, position_node.integer);
	if (!conical_surface.position)
		return nullptr;

	if (auto radius = get_number(out, parameters[2]); radius)
		conical_surface.radius = *radius;
	else
		return nullptr;

	if (auto semi_angle = get_number(out, parameters[3]); semi_angle)
		conical_surface.semi_angle = *semi_angle;
	else
		return nullptr;

	A242::Conical_Surface* ptr = a242.arena.take<A242::Conical_Surface>(xstd::move(conical_surface));
	a242.conical_surfaces.push(ptr);
	return ptr;
}

compile_signature(Toroidal_Surface) {
	subtype_check_begin(TOROIDAL_SURFACE);
		// >TODO_ITEM degenerate_toroidal_surface
	subtype_check_end(Toroidal_Surface);

	A242::Toroidal_Surface toroidal_surface;
	toroidal_surface.type_name = "TOROIDAL_SURFACE";
	if (parameters.size != 4)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	toroidal_surface.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& position_node = out.nodes[parameters[1]];
	toroidal_surface.position = get_call(Axis2_Placement_3d, out, a242, position_node.integer);
	if (!toroidal_surface.position)
		return nullptr;

	if (auto major_radius = get_number(out, parameters[2]); major_radius)
		toroidal_surface.major_radius = *major_radius;
	else
		return nullptr;

	if (auto minor_radius = get_number(out, parameters[3]); minor_radius)
		toroidal_surface.minor_radius = *minor_radius;
	else
		return nullptr;


	A242::Toroidal_Surface* ptr = a242.arena.take<A242::Toroidal_Surface>(xstd::move(toroidal_surface));
	a242.toroidal_surfaces.push(ptr);
	return ptr;
}

compile_signature(Bounded_Surface);

compile_signature(Surface) {
	subtype_check_begin(SURFACE);
		subtype_check_case(Surface, Bounded_Surface);
		subtype_check_case(Surface, Elementary_Surface);
		// >TODO_ITEM offset_surface
		// >TODO_ITEM oriented_surface
		// >TODO_ITEM surface_replica
		// >TODO_ITEM swept_surface
	subtype_check_end(Surface);
	return nullptr;
}
compile_signature(Face_Surface);
compile_signature(B_Spline_Surface);

compile_signature(Bounded_Surface) {
	subtype_check_begin(BOUNDED_SURFACE);
		subtype_check_case(Bounded_Surface, Face_Surface);
		subtype_check_case(Bounded_Surface, B_Spline_Surface);
	subtype_check_end(Bounded_Surface);

	print("A bounded surface by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Rational_B_Spline_Surface);
compile_signature(B_Spline_Surface_With_Knots);
compile_signature(Uniform_Surface);
compile_signature(Quasi_Uniform_Surface);
compile_signature(Bezier_Surface);

compile_signature(B_Spline_Surface) {
	if (type.size == 0) {
		// Look for the rational b spline surface
		A242::Rational_B_Spline_Surface* rational_b_spline_surface = nullptr;
		for (size_t i = 0; i < parameters.size; i += 1) {
			const Node& record = out.nodes[parameters[i]];
			if (record.kind != Node::Kind::SIMPLE_RECORD)
				continue;

			const Token& record_token = out.tokens[record.simple_record.keyword_token];
			const Node& parameter_node = out.nodes[record.simple_record.parameters];
			if (record_token.text != "RATIONAL_B_SPLINE_SURFACE")
				continue;
			
			rational_b_spline_surface = compile_Rational_B_Spline_Surface(
				out, a242, type, parameter_node.list.view(), nullptr
				);
			if (!rational_b_spline_surface)
				return nullptr;
			break;
		}

		// We are a complex entity, that's only possible for rational b-spline surface
		const char* with_knots[] = {
			"REPRESENTATION_ITEM",
			"GEOMETRIC_REPRESENTATION_ITEM",
			"SURFACE",
			"BOUNDED_SURFACE",
			"B_SPLINE_SURFACE",
			"B_SPLINE_SURFACE_WITH_KNOTS"
		};
		View<const size_t> with_knots_list = flatten_complex<6>(out, with_knots, parameters);
		auto ptr = compile_B_Spline_Surface_With_Knots(
			out, a242, "B_SPLINE_SURFACE_WITH_KNOTS", with_knots_list, nullptr
		);
		if (ptr) {
			rational_b_spline_surface->with_knots = *(A242::B_Spline_Surface_With_Knots_Data*)ptr;
			return (A242::B_Spline_Surface*)rational_b_spline_surface;
		}

		const char* uniform[] = {
			"REPRESENTATION_ITEM",
			"GEOMETRIC_REPRESENTATION_ITEM",
			"SURFACE",
			"BOUNDED_SURFACE",
			"B_SPLINE_SURFACE",
			"UNIFORM_B_SURFACE"
		};
		View<const size_t> uniform_list = flatten_complex<6>(out, uniform, parameters);
		auto uniform_ptr = compile_Uniform_Surface(
			out, a242, "UNIFORM_B_SURFACE", uniform_list, nullptr
		);
		if (uniform_ptr) {
			rational_b_spline_surface->uniform_surface = *(A242::Uniform_Surface_Data*)uniform_ptr;
			return (A242::B_Spline_Surface*)rational_b_spline_surface;
		}

		const char* quasi_uniform[] = {
			"REPRESENTATION_ITEM",
			"GEOMETRIC_REPRESENTATION_ITEM",
			"SURFACE",
			"BOUNDED_SURFACE",
			"B_SPLINE_SURFACE",
			"QUASI_UNIFORM_B_SURFACE"
		};
		View<const size_t> quasi_list = flatten_complex<6>(out, quasi_uniform, parameters);
		auto quasi_ptr = compile_Quasi_Uniform_Surface(
			out, a242, "QUASI_UNIFORM_B_SURFACE", quasi_list, nullptr
		);
		if (quasi_ptr) {
			rational_b_spline_surface->quasi_uniform_surface =
				*(A242::Quasi_Uniform_Surface_Data*)quasi_ptr;
			return (A242::B_Spline_Surface*)rational_b_spline_surface;
		}

		const char* bezier[] = {
			"REPRESENTATION_ITEM",
			"GEOMETRIC_REPRESENTATION_ITEM",
			"SURFACE",
			"BOUNDED_SURFACE",
			"B_SPLINE_SURFACE",
			"BEZIER_SURFACE"
		};
		View<const size_t> bezier_list = flatten_complex<6>(out, bezier, parameters);
		auto bezier_ptr = compile_Bezier_Surface(
			out, a242, "BEZIER_SURFACE", bezier_list, nullptr
		);
		if (bezier_ptr) {
			rational_b_spline_surface->bezier_surface = *(A242::Bezier_Surface_Data*)bezier_ptr;
			return (A242::B_Spline_Surface*)rational_b_spline_surface;
		}

		return (A242::B_Spline_Surface*)rational_b_spline_surface;
	}

	subtype_check_begin(B_SPLINE_SURFACE);
		subtype_check_case(B_Spline_Surface, B_Spline_Surface_With_Knots);
		subtype_check_case(B_Spline_Surface, Uniform_Surface);
		subtype_check_case(B_Spline_Surface, Quasi_Uniform_Surface);
		subtype_check_case(B_Spline_Surface, Bezier_Surface);
	subtype_check_end(B_Spline_Surface);

	print("A B-spline surface by itself should never be instantiated\n");
	return nullptr;
}

compile_signature(Rational_B_Spline_Surface) {
	if (type.size != 0) {
		subtype_check_begin(RATIONAL_B_SPLINE_SURFACE);
		subtype_check_end(Rational_B_Spline_Surface);
	}

	A242::Rational_B_Spline_Surface rational_b_spline_surface;
	rational_b_spline_surface.type_name = "RATIONAL_B_SPLINE_SURFACE";

	if (parameters.size != 1)
		return nullptr;

	const Node& list_list_weights = out.nodes[parameters[0]];
	if (list_list_weights.kind != Node::Kind::LIST)
		return nullptr;

	rational_b_spline_surface.weights_sizes.size = list_list_weights.list.size;
	rational_b_spline_surface.weights_sizes.data = a242.arena.alloc<u32>(
		rational_b_spline_surface.weights_sizes.size
	);

	size_t n = 0;
	for (size_t i = 0; i < list_list_weights.list.size; i += 1) {
		const Node& weights_list = out.nodes[list_list_weights.list[i]];
		if (weights_list.kind != Node::Kind::LIST)
			return nullptr;
		n += weights_list.list.size;
		rational_b_spline_surface.weights_sizes.data[i] = (u32)n;
	}

	rational_b_spline_surface.weights_datas.size = n;
	rational_b_spline_surface.weights_datas.data = a242.arena.alloc<float>(
		rational_b_spline_surface.weights_datas.size
	);
	n = 0;
	for (size_t i = 0; i < list_list_weights.list.size; i += 1) {
		const Node& weights_list = out.nodes[list_list_weights.list[i]];

		for (size_t j = 0; j < weights_list.list.size; j += 1) {
			const Node& weight_node = out.nodes[weights_list.list[j]];
			if (weight_node.kind != Node::Kind::NUMBER)
				return nullptr;
			rational_b_spline_surface.weights_datas.data[n] = (float)weight_node.number;
			n += 1;
		}
	}

	A242::Rational_B_Spline_Surface* ptr =
		a242.arena.take<A242::Rational_B_Spline_Surface>(xstd::move(rational_b_spline_surface));
	a242.rational_b_spline_surfaces.push(ptr);
	return ptr;
}

compile_signature(B_Spline_Surface_With_Knots) {
	subtype_check_begin(B_SPLINE_SURFACE_WITH_KNOTS);
	subtype_check_end(B_Spline_Surface_With_Knots);

	A242::B_Spline_Surface_With_Knots b_spline_surface_with_knots;
	b_spline_surface_with_knots.type_name = "B_SPLINE_SURFACE_WITH_KNOTS";

	if (parameters.size != 13)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	b_spline_surface_with_knots.name = name.string;
	
	if (!get_number(out, parameters[1]))
		return nullptr;
	b_spline_surface_with_knots.u_degree = (u32)*get_number(out, parameters[1]);

	if (!get_number(out, parameters[2]))
		return nullptr;
	b_spline_surface_with_knots.v_degree = (u32)*get_number(out, parameters[2]);

	const Node& control_points_list = out.nodes[parameters[3]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	b_spline_surface_with_knots.control_points_sizes.size = control_points_list.list.size;
	b_spline_surface_with_knots.control_points_sizes.data = a242.arena.alloc<u32>(
		b_spline_surface_with_knots.control_points_sizes.size
	);
	size_t n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::LIST)
			return nullptr;
		n += out.nodes[control_points_list.list[i]].list.size;
		b_spline_surface_with_knots.control_points_sizes.data[i] = (u32)n;
	}

	b_spline_surface_with_knots.control_points_datas.size = n;
	b_spline_surface_with_knots.control_points_datas.data =
		a242.arena.alloc<A242::Cartesian_Point*>(n);
	n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& list = out.nodes[control_points_list.list[i]];
		if (list.kind != Node::Kind::LIST)
			return nullptr;

		for (size_t j = 0; j < list.list.size; j += 1) {
			const Node& point_node = out.nodes[list.list[j]];
			if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
				return nullptr;

			A242::Cartesian_Point* point =
				get_call(Cartesian_Point, out, a242, point_node.integer);
			if (!point)
				return nullptr;
			b_spline_surface_with_knots.control_points_datas.data[n] = point;
			n += 1;
		}
	}

	if (!get_bool(out, parameters[5]).has_value)
		return nullptr;
	b_spline_surface_with_knots.u_closed = *get_bool(out, parameters[5]);
	if (!get_bool(out, parameters[6]).has_value)
		return nullptr;
	b_spline_surface_with_knots.v_closed = *get_bool(out, parameters[6]);
	if (!get_bool(out, parameters[7]).has_value)
		return nullptr;
	b_spline_surface_with_knots.self_intersect = *get_bool(out, parameters[7]);

	const Node& u_knot_multiplicities_list = out.nodes[parameters[8]];
	if (u_knot_multiplicities_list.kind != Node::Kind::LIST)
		return nullptr;
	b_spline_surface_with_knots.u_knot_multiplicities.size = u_knot_multiplicities_list.list.size;
	b_spline_surface_with_knots.u_knot_multiplicities.data = a242.arena.alloc<u32>(
		b_spline_surface_with_knots.u_knot_multiplicities.size
	);
	for (size_t i = 0; i < u_knot_multiplicities_list.list.size; i += 1) {
		const Node& param = out.nodes[u_knot_multiplicities_list.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_surface_with_knots.u_knot_multiplicities.data[i] = (u32)param.number;
	}

	const Node& v_knot_multiplicities_list = out.nodes[parameters[9]];
	if (v_knot_multiplicities_list.kind != Node::Kind::LIST)
		return nullptr;
	b_spline_surface_with_knots.v_knot_multiplicities.size = v_knot_multiplicities_list.list.size;
	b_spline_surface_with_knots.v_knot_multiplicities.data = a242.arena.alloc<u32>(
		b_spline_surface_with_knots.v_knot_multiplicities.size
	);
	for (size_t i = 0; i < v_knot_multiplicities_list.list.size; i += 1) {
		const Node& param = out.nodes[v_knot_multiplicities_list.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_surface_with_knots.v_knot_multiplicities.data[i] = (u32)param.number;
	}

	const Node& u_knots = out.nodes[parameters[10]];
	if (u_knots.kind != Node::Kind::LIST)
		return nullptr;
	b_spline_surface_with_knots.u_knots.size = u_knots.list.size;
	b_spline_surface_with_knots.u_knots.data = a242.arena.alloc<float>(
		b_spline_surface_with_knots.u_knots.size
	);
	for (size_t i = 0; i < u_knots.list.size; i += 1) {
		const Node& param = out.nodes[u_knots.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_surface_with_knots.u_knots.data[i] = (float)param.number;
	}

	const Node& v_knots = out.nodes[parameters[11]];
	if (v_knots.kind != Node::Kind::LIST)
		return nullptr;
	b_spline_surface_with_knots.v_knots.size = v_knots.list.size;
	b_spline_surface_with_knots.v_knots.data = a242.arena.alloc<float>(
		b_spline_surface_with_knots.v_knots.size
	);
	for (size_t i = 0; i < v_knots.list.size; i += 1) {
		const Node& param = out.nodes[v_knots.list[i]];
		if (param.kind != Node::Kind::NUMBER)
			return nullptr;
		b_spline_surface_with_knots.v_knots.data[i] = (float)param.number;
	}

	const Node& knot_spec_node = out.nodes[parameters[12]];
	if (knot_spec_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	if (!get_knot_type(out, knot_spec_node.enumeration))
		return nullptr;
	b_spline_surface_with_knots.knot_spec = *get_knot_type(out, knot_spec_node.enumeration);

	A242::B_Spline_Surface_With_Knots* ptr =
		a242.arena.take<A242::B_Spline_Surface_With_Knots>(xstd::move(b_spline_surface_with_knots));
	a242.b_spline_surfaces_with_knots.push(ptr);
	return ptr;
}

compile_signature(Uniform_Surface) {
	subtype_check_begin(UNIFORM_SURFACE);
	subtype_check_end(Uniform_Surface);

	A242::Uniform_Surface uniform_surface;
	uniform_surface.type_name = "UNIFORM_SURFACE";

	if (parameters.size != 9)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	uniform_surface.name = name.string;
	
	if (!get_number(out, parameters[1]))
		return nullptr;
	uniform_surface.u_degree = (u32)*get_number(out, parameters[1]);

	if (!get_number(out, parameters[2]))
		return nullptr;
	uniform_surface.v_degree = (u32)*get_number(out, parameters[2]);

	const Node& control_points_list = out.nodes[parameters[3]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	uniform_surface.control_points_sizes.size = control_points_list.list.size;
	uniform_surface.control_points_sizes.data = a242.arena.alloc<u32>(
		uniform_surface.control_points_sizes.size
	);
	size_t n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::LIST)
			return nullptr;
		n += out.nodes[control_points_list.list[i]].list.size;
		uniform_surface.control_points_sizes.data[i] = (u32)n;
	}

	uniform_surface.control_points_datas.size = n;
	uniform_surface.control_points_datas.data =
		a242.arena.alloc<A242::Cartesian_Point*>(n);
	n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& list = out.nodes[control_points_list.list[i]];
		if (list.kind != Node::Kind::LIST)
			return nullptr;

		for (size_t j = 0; j < list.list.size; j += 1) {
			const Node& point_node = out.nodes[list.list[j]];
			if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
				return nullptr;

			A242::Cartesian_Point* point =
				get_call(Cartesian_Point, out, a242, point_node.integer);
			if (!point)
				return nullptr;
			uniform_surface.control_points_datas.data[n] = point;
			n += 1;
		}
	}

	if (!get_bool(out, parameters[4]).has_value)
		return nullptr;
	uniform_surface.u_closed = *get_bool(out, parameters[4]);
	if (!get_bool(out, parameters[5]).has_value)
		return nullptr;
	uniform_surface.v_closed = *get_bool(out, parameters[5]);
	if (!get_bool(out, parameters[6]).has_value)
		return nullptr;
	uniform_surface.self_intersect = *get_bool(out, parameters[6]);

	A242::Uniform_Surface* ptr =
		a242.arena.take<A242::Uniform_Surface>(xstd::move(uniform_surface));
	a242.uniform_surfaces.push(ptr);
	return ptr;
}

compile_signature(Quasi_Uniform_Surface) {
	subtype_check_begin(QUASI_UNIFORM_SURFACE);
	subtype_check_end(Quasi_Uniform_Surface);

	A242::Quasi_Uniform_Surface quasi_uniform_surface;
	quasi_uniform_surface.type_name = "QUASI_UNIFORM_SURFACE";

	if (parameters.size != 9)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	quasi_uniform_surface.name = name.string;
	
	if (!get_number(out, parameters[1]))
		return nullptr;
	quasi_uniform_surface.u_degree = (u32)*get_number(out, parameters[1]);

	if (!get_number(out, parameters[2]))
		return nullptr;
	quasi_uniform_surface.v_degree = (u32)*get_number(out, parameters[2]);

	const Node& control_points_list = out.nodes[parameters[3]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	quasi_uniform_surface.control_points_sizes.size = control_points_list.list.size;
	quasi_uniform_surface.control_points_sizes.data = a242.arena.alloc<u32>(
		quasi_uniform_surface.control_points_sizes.size
	);
	size_t n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::LIST)
			return nullptr;
		n += out.nodes[control_points_list.list[i]].list.size;
		quasi_uniform_surface.control_points_sizes.data[i] = (u32)n;
	}

	quasi_uniform_surface.control_points_datas.size = n;
	quasi_uniform_surface.control_points_datas.data = a242.arena.alloc<A242::Cartesian_Point*>(n);
	n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& list = out.nodes[control_points_list.list[i]];
		if (list.kind != Node::Kind::LIST)
			return nullptr;

		for (size_t j = 0; j < list.list.size; j += 1) {
			const Node& point_node = out.nodes[list.list[j]];
			if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
				return nullptr;

			A242::Cartesian_Point* point =
				get_call(Cartesian_Point, out, a242, point_node.integer);
			if (!point)
				return nullptr;
			quasi_uniform_surface.control_points_datas.data[n] = point;
			n += 1;
		}
	}

	if (!get_bool(out, parameters[4]).has_value)
		return nullptr;
	quasi_uniform_surface.u_closed = *get_bool(out, parameters[4]);
	if (!get_bool(out, parameters[5]).has_value)
		return nullptr;
	quasi_uniform_surface.v_closed = *get_bool(out, parameters[5]);
	if (!get_bool(out, parameters[6]).has_value)
		return nullptr;
	quasi_uniform_surface.self_intersect = *get_bool(out, parameters[6]);

	A242::Quasi_Uniform_Surface* ptr =
		a242.arena.take<A242::Quasi_Uniform_Surface>(xstd::move(quasi_uniform_surface));
	a242.quasi_uniform_surfaces.push(ptr);
	return ptr;
}

compile_signature(Bezier_Surface) {
	subtype_check_begin(BEZIER_SURFACE);
	subtype_check_end(Bezier_Surface);

	A242::Bezier_Surface bezier_surface;
	bezier_surface.type_name = "BEZIER_SURFACE";

	if (parameters.size != 9)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	bezier_surface.name = name.string;
	
	if (!get_number(out, parameters[1]))
		return nullptr;
	bezier_surface.u_degree = (u32)*get_number(out, parameters[1]);

	if (!get_number(out, parameters[2]))
		return nullptr;
	bezier_surface.v_degree = (u32)*get_number(out, parameters[2]);

	const Node& control_points_list = out.nodes[parameters[3]];
	if (control_points_list.kind != Node::Kind::LIST)
		return nullptr;

	bezier_surface.control_points_sizes.size = control_points_list.list.size;
	bezier_surface.control_points_sizes.data = a242.arena.alloc<u32>(
		bezier_surface.control_points_sizes.size
	);
	size_t n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& point_node = out.nodes[control_points_list.list[i]];
		if (point_node.kind != Node::Kind::LIST)
			return nullptr;
		n += out.nodes[control_points_list.list[i]].list.size;
		bezier_surface.control_points_sizes.data[i] = (u32)n;
	}

	bezier_surface.control_points_datas.size = n;
	bezier_surface.control_points_datas.data = a242.arena.alloc<A242::Cartesian_Point*>(n);
	n = 0;
	for (size_t i = 0; i < control_points_list.list.size; i += 1) {
		const Node& list = out.nodes[control_points_list.list[i]];
		if (list.kind != Node::Kind::LIST)
			return nullptr;

		for (size_t j = 0; j < list.list.size; j += 1) {
			const Node& point_node = out.nodes[list.list[j]];
			if (point_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
				return nullptr;

			A242::Cartesian_Point* point =
				get_call(Cartesian_Point, out, a242, point_node.integer);
			if (!point)
				return nullptr;
			bezier_surface.control_points_datas.data[n] = point;
			n += 1;
		}
	}

	if (!get_bool(out, parameters[4]).has_value)
		return nullptr;
	bezier_surface.u_closed = *get_bool(out, parameters[4]);
	if (!get_bool(out, parameters[5]).has_value)
		return nullptr;
	bezier_surface.v_closed = *get_bool(out, parameters[5]);
	if (!get_bool(out, parameters[6]).has_value)
		return nullptr;
	bezier_surface.self_intersect = *get_bool(out, parameters[6]);

	A242::Bezier_Surface* ptr =
		a242.arena.take<A242::Bezier_Surface>(xstd::move(bezier_surface));
	a242.bezier_surfaces.push(ptr);
	return ptr;
}

compile_signature(Advanced_Face) {
	subtype_check_begin(ADVANCED_FACE);
	subtype_check_end(Advanced_Face);

	A242::Advanced_Face advanced_face;
	advanced_face.type_name = "ADVANCED_FACE";

	if (parameters.size != 4)
		return nullptr;
	
	const Node& name = out.nodes[parameters[0]];
	advanced_face.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& bounds_list = out.nodes[parameters[1]];
	if (bounds_list.kind != Node::Kind::LIST)
		return nullptr;
	advanced_face.bounds.size = bounds_list.list.size;
	advanced_face.bounds.data = a242.arena.alloc<A242::Face_Bound*>(advanced_face.bounds.size);
	for (size_t i = 0; i < advanced_face.bounds.size; i += 1) {
		const Node& bound_node = out.nodes[bounds_list.list[i]];
		if (bound_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;
		advanced_face.bounds.data[i] = get_call(Face_Bound, out, a242, bound_node.integer);
		if (!advanced_face.bounds.data[i])
			return nullptr;
	}

	const Node& face_geometry_node = out.nodes[parameters[2]];
	advanced_face.face_geometry = get_call(Surface, out, a242, face_geometry_node.integer);
	if (!advanced_face.face_geometry)
		return nullptr;

	const Node& same_sense_node = out.nodes[parameters[3]];
	advanced_face.same_sense = same_sense_node.enumeration == "T";
	if (same_sense_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Advanced_Face* ptr = a242.arena.take<A242::Advanced_Face>(xstd::move(advanced_face));
	a242.advanced_faces.push(ptr);
	return ptr;
}

compile_signature(Face_Surface) {
	subtype_check_begin(FACE_SURFACE);
		subtype_check_case(Face_Surface, Advanced_Face);
	subtype_check_end(Face_Surface);

	A242::Face_Surface face_surface;
	face_surface.type_name = "FACE_SURFACE";
	if (parameters.size != 3)
		return nullptr;
	
	const Node& name = out.nodes[parameters[0]];
	face_surface.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& face_node = out.nodes[parameters[1]];
	if (face_node.kind != Node::Kind::LIST)
		return nullptr;
	face_surface.bounds.size = face_node.list.size;
	face_surface.bounds.data = a242.arena.alloc<A242::Face_Bound*>(face_surface.bounds.size);
	for (size_t i = 0; i < face_surface.bounds.size; i += 1) {
		const Node& bound_node = out.nodes[face_node.list[i]];
		if (bound_node.kind != Node::Kind::ENTITY_INSTANCE_NAME)
			return nullptr;
		face_surface.bounds.data[i] = get_call(Face_Bound, out, a242, bound_node.integer);
		if (!face_surface.bounds.data[i])
			return nullptr;
	}

	const Node& face_geometry_node = out.nodes[parameters[2]];
	face_surface.face_geometry = get_call(Surface, out, a242, face_geometry_node.integer);
	if (!face_surface.face_geometry)
		return nullptr;

	const Node& same_sense_node = out.nodes[parameters[3]];
	face_surface.same_sense = same_sense_node.enumeration == "T";
	if (same_sense_node.kind != Node::Kind::ENUMERATION)
		return nullptr;

	A242::Face_Surface* ptr = a242.arena.take<A242::Face_Surface>(xstd::move(face_surface));
	a242.face_surfaces.push(ptr);
	return ptr;
}

compile_signature(Face) {
	subtype_check_begin(FACE);
		subtype_check_case(Face, Face_Surface);
	subtype_check_end(Face);

	A242::Face face;
	face.type_name = "FACE";
	if (parameters.size != 1)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	face.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	A242::Face* ptr = a242.arena.take<A242::Face>(xstd::move(face));
	// a242.faces.push(ptr);
	return ptr;
}

compile_signature(Line) {
	subtype_check_begin(LINE);
	subtype_check_end(Line);

	A242::Line line;
	line.type_name = "LINE";
	if (parameters.size != 3)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	line.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& point_node = out.nodes[parameters[1]];
	line.point = get_call(Cartesian_Point, out, a242, point_node.integer);
	if (!line.point)
		return nullptr;

	const Node& direction_node = out.nodes[parameters[2]];
	line.line_direction = get_call(Vector, out, a242, direction_node.integer);
	if (!line.line_direction)
		return nullptr;

	A242::Line* ptr = a242.arena.take<A242::Line>(xstd::move(line));
	a242.lines.push(ptr);
	return ptr;
}

compile_signature(Edge_Loop) {
	subtype_check_begin(EDGE_LOOP);
	subtype_check_end(Edge_Loop);

	A242::Edge_Loop edge_loop;
	edge_loop.type_name = "EDGE_LOOP";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	edge_loop.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& edge_list = out.nodes[parameters[1]];
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
	subtype_check_begin(EDGE_CURVE);
	subtype_check_end(Edge_Curve);

	A242::Edge_Curve edge_curve;
	edge_curve.type_name = "EDGE_CURVE";
	if (parameters.size != 5)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	edge_curve.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;
	
	const Node& edge_start_node = out.nodes[parameters[1]];
	edge_curve.edge_start = get_call(Vertex_Point, out, a242, edge_start_node.integer);
	if (!edge_curve.edge_start)
		return nullptr;
	
	const Node& edge_end_node = out.nodes[parameters[2]];
	edge_curve.edge_end = get_call(Vertex_Point, out, a242, edge_end_node.integer);
	if (!edge_curve.edge_end)
		return nullptr;

	const Node& edge_geometry_node = out.nodes[parameters[3]];
	edge_curve.edge_geometry = get_call(Curve, out, a242, edge_geometry_node.integer);
	if (!edge_curve.edge_geometry)
		return nullptr;

	const Node& same_sense_node = out.nodes[parameters[4]];
	edge_curve.same_sense = same_sense_node.enumeration == "T";
	if (same_sense_node.kind != Node::Kind::ENUMERATION)
		return nullptr;
	
	A242::Edge_Curve* ptr = a242.arena.take<A242::Edge_Curve>(xstd::move(edge_curve));
	a242.edge_curves.push(ptr);
	return ptr;
}

compile_signature(Cylindrical_Surface) {
	subtype_check_begin(CYLINDRICAL_SURFACE);
	subtype_check_end(Cylindrical_Surface);
	A242::Cylindrical_Surface surface;
	surface.type_name = "CYLINDRICAL_SURFACE";
	if (parameters.size != 3)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	surface.name = name.string;
	if (name.kind != Node::Kind::STRING)
		return nullptr;

	const Node& placement_node = out.nodes[parameters[1]];
	surface.position = get_call(Axis2_Placement_3d, out, a242, placement_node.integer);
	if (!surface.position)
		return nullptr;

	const Node& radius_node = out.nodes[parameters[2]];
	surface.radius = (float)radius_node.number;
	if (radius_node.kind != Node::Kind::NUMBER)
		return nullptr;

	A242::Cylindrical_Surface* ptr =
		a242.arena.take<A242::Cylindrical_Surface>(xstd::move(surface));
	a242.cylindrical_surfaces.push(ptr);
	return ptr;
}

compile_signature(Plane) {
	subtype_check_begin(PLANE);
	subtype_check_end(Plane);
	A242::Plane plane;
	plane.type_name = "PLANE";
	if (parameters.size != 2)
		return nullptr;

	const Node& name = out.nodes[parameters[0]];
	if (name.kind == Node::Kind::STRING) {
		plane.name = name.string;
	}

	const Node& location_node = out.nodes[parameters[1]];
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
		if (is_type<A242::Manifold_Solid_Brep>(out, a242, i)) {
			get_call(Manifold_Solid_Brep, out, a242, i);
		}
	}

	// for (size_t i = 0; i < a242.edge_loops.size; i += 1) {
	// 	A242::Edge_Loop* edge_loop = a242.edge_loops[i];
	// 	print("Edge Loop: ");
	// 	print(edge_loop->name);

	// 	print("(");
	// 	for (size_t j = 0; j < edge_loop->edge_list.size; j += 1) {
	// 		print(" ");
	// 		A242::Oriented_Edge* oriented_edge = edge_loop->edge_list.data[j];
	// 		print("Oriented Edge: ");
	// 		print(oriented_edge->name);
	// 		print(" ");
	// 	}
	// 	print(")\n");
	// }

}

#pragma once

#include "Memory.hpp"
#include "EXPRESS.hpp"
#include "Templates.hpp"
struct A242 {
	// #include "A242_struct.hpp"

	template<size_t H>
	struct Item {
		static constexpr u32 hash = (u32)H;
		u32 id = (u32)H;
		const char* type_name = "";
	};
	template<size_t H>
	using Item_Pack = xstd::Template_Pack<Item<H>>;

	#define struct_A242(x, ...)\
		struct x##_Data;\
		using x##_Pack = xstd::Unique_Pack<typename xstd::Flatten_Pack<xstd::Template_Pack<\
			xstd::Template_Pack<x##_Data>,\
			__VA_ARGS__\
		>\
		>::type>::type;\
		struct x##_Name {\
			static constexpr const char NAME[] = #x;\
		};\
		using x##_Pack_Inherited = xstd::Template_Pack<Item<case_insenstive_hash(#x)>, x##_Pack, x##_Name>;\
		using x = xstd::inherit<xstd::Flatten_Pack<x##_Pack_Inherited>::type>;\
		struct x##_Data

	struct_A242(Representation_Item) {
		Read_String name;
	};

	struct_A242(Geometric_Representation_Item, Representation_Item_Pack) {
	};

	struct_A242(Point, Geometric_Representation_Item_Pack) {
	};

	struct_A242(Cartesian_Point, Point_Pack) {
		float x;
		float y;
		float z;
	};

	struct_A242(Topological_Representation_Item, Representation_Item_Pack) {
	};

	struct_A242(Vertex, Topological_Representation_Item_Pack) {
	};

	struct_A242(Vertex_Point, Vertex_Pack, Geometric_Representation_Item_Pack) {
		ref<Point> vertex_geometry = nullptr;
	};

	struct_A242(Placement, Geometric_Representation_Item_Pack) {
		ref<Cartesian_Point> location;
	};
	struct_A242(Direction, Geometric_Representation_Item_Pack) {
		float x;
		float y;
		float z;
	};
	struct_A242(Axis2_Placement_3d, Placement_Pack) {
		Direction* axis = nullptr;
		Direction* ref_direction = nullptr;
	};

	struct_A242(Surface, Geometric_Representation_Item_Pack) {
	};
	struct_A242(Bounded_Surface, Surface_Pack) {
	};
	enum class Knot_Type {
		Uniform_Knots,
		Quasi_Uniform_Knots,
		Piecewise_Bezier_Knots,
		Unspecified
	};
	struct_A242(B_Spline_Surface, Bounded_Surface_Pack) {
		u32 u_degree;
		u32 v_degree;
		View<Cartesian_Point*> control_points_datas;
		View<u32>              control_points_sizes;
		enum class Surface_Form {
			Unspecified,
			Plane,
			Cylindrical,
			Conical,
			Spherical,
			Toroidal,
			Surface_Of_Revolution,
			Ruled_Surface,
			Generalized_Cone,
			Quadric_Surface,
			Surface_Of_Extrusion
		};
		Surface_Form surface_form;

		bool u_closed;
		bool v_closed;
		bool self_intersect;
	};
	struct_A242(B_Spline_Surface_With_Knots, B_Spline_Surface_Pack) {
		View<u32> u_knot_multiplicities;
		View<u32> v_knot_multiplicities;
		View<f32> u_knots;
		View<f32> v_knots;

		Knot_Type knot_spec;
	};
	struct_A242(Uniform_Surface, B_Spline_Surface_Pack) {
	};
	struct_A242(Quasi_Uniform_Surface, B_Spline_Surface_Pack) {
	};
	struct_A242(Bezier_Surface, B_Spline_Surface_Pack) {
	};
	struct_A242(Rational_B_Spline_Surface, B_Spline_Surface_Pack) {
		B_Spline_Surface_With_Knots_Data with_knots;
		Uniform_Surface_Data             uniform_surface;
		Quasi_Uniform_Surface_Data       quasi_uniform_surface;
		Bezier_Surface_Data              bezier_surface;

		View<f32> weights_datas;
		View<u32> weights_sizes;
	};
	struct_A242(Elementary_Surface, Surface_Pack) {
		ref<Axis2_Placement_3d> position = nullptr;
	};
	struct_A242(Toroidal_Surface, Elementary_Surface_Pack) {
		float major_radius;
		float minor_radius;
	};
	struct_A242(Conical_Surface, Elementary_Surface_Pack) {
		float radius;
		float semi_angle;
	};
	struct_A242(Plane, Elementary_Surface_Pack) {
	};
	struct_A242(Cylindrical_Surface, Elementary_Surface_Pack) {
		float radius;
	};

	struct_A242(Detailed_Geometric_Model_Element, Representation_Item_Pack) {
	};
	struct_A242(Curve, Detailed_Geometric_Model_Element_Pack) {
	};
	struct_A242(Bounded_Curve, Curve_Pack) {
	};
	struct_A242(B_Spline_Curve, Bounded_Curve_Pack) {
		u32 degree;
		View<Cartesian_Point*> control_points_list;

		enum class Curve_Form {
			Polyline,
			Circular_Arc,
			Ellipse_Arc,
			Parabolic_Arc,
			Hyperbolic_Arc,
			Unspecified
		};
		Curve_Form curve_form;

		bool closed_curve;
		bool self_intersect;
	};
	struct_A242(B_Spline_Curve_With_Knots, B_Spline_Curve_Pack) {
		View<u32> knot_multiplicities;
		View<f32> knots;

		Knot_Type knot_spec;
	};
	struct_A242(Conic, Curve_Pack) {
		ref<Axis2_Placement_3d> position = nullptr;
	};
	struct_A242(Circle, Conic_Pack) {
		float radius;
	};
	struct_A242(Ellipse, Conic_Pack) {
		float major_radius;
		float minor_radius;
	};
	struct_A242(Edge, Topological_Representation_Item_Pack) {
		ref<Vertex_Point> edge_start = nullptr;
		ref<Vertex_Point> edge_end = nullptr;
	};
	struct_A242(Edge_Curve, Edge_Pack, Geometric_Representation_Item_Pack) {
		ref<Curve> edge_geometry = nullptr;
		bool same_sense = false;
	};
	struct_A242(Oriented_Edge, Edge_Pack) {
		ref<Edge> edge_element = nullptr;
		bool orientation = false;
	};
	
	struct_A242(Loop, Topological_Representation_Item_Pack) {
	};
	struct_A242(Path, Topological_Representation_Item_Pack) {
		View<Oriented_Edge*> edge_list;
	};
	struct_A242(Edge_Loop, Loop_Pack, Path_Pack) {
	};

	struct_A242(Vector, Geometric_Representation_Item_Pack) {
		ref<Direction> orientation = nullptr;
		float magnitude;
	};
	struct_A242(Line, Curve_Pack) {
		ref<Cartesian_Point> point = nullptr;
		ref<Vector> line_direction = nullptr;
	};
	struct_A242(Face_Bound, Loop_Pack) {
		ref<Loop> bound = nullptr;
		bool orientation = false;
	};
	struct_A242(Face_Outer_Bound, Face_Bound_Pack) {
	};
	struct_A242(Face, Topological_Representation_Item_Pack) {
		View<Face_Bound*> bounds;
	};
	struct_A242(Face_Surface, Face_Pack, Geometric_Representation_Item_Pack) {
		ref<Surface> face_geometry = nullptr;
		bool same_sense = false;
	};
	struct_A242(Advanced_Face, Face_Surface_Pack) {
	};

	struct_A242(Connected_Face_Set, Topological_Representation_Item_Pack) {
		View<Face*> cfs_faces;
	};
	struct_A242(Closed_Shell, Connected_Face_Set_Pack) {
	};

	struct_A242(Solid_Model, Geometric_Representation_Item_Pack) {
	};
	struct_A242(Manifold_Solid_Brep, Solid_Model_Pack) {
		ref<Closed_Shell> outer;
	};
#undef struct_A242

	DynArray<Manifold_Solid_Brep*> manifold_solid_breps;
	DynArray<Closed_Shell*> closed_shells;
	DynArray<Advanced_Face*> advanced_faces;
	DynArray<Face_Surface*> face_surfaces;
	DynArray<Loop*> loops;
	DynArray<Face_Bound*> face_bounds;
	DynArray<Vector*> vectors;
	DynArray<Line*> lines;
	DynArray<Edge_Loop*> edge_loops;
	DynArray<Oriented_Edge*> oriented_edges;
	DynArray<Cylindrical_Surface*> cylindrical_surfaces;
	DynArray<Edge_Curve*> edge_curves;
	DynArray<Curve*> curves;
	DynArray<Circle*> circles;
	DynArray<Ellipse*> ellipses;
	DynArray<Conical_Surface*> conical_surfaces;
	DynArray<B_Spline_Curve*> b_spline_curves;
	DynArray<B_Spline_Surface_With_Knots*> b_spline_surfaces_with_knots;
	DynArray<B_Spline_Curve_With_Knots*> b_spline_curves_with_knots;
	DynArray<Uniform_Surface*> uniform_surfaces;
	DynArray<Quasi_Uniform_Surface*> quasi_uniform_surfaces;
	DynArray<Rational_B_Spline_Surface*> rational_b_spline_surfaces;
	DynArray<Bezier_Surface*> bezier_surfaces;
	DynArray<Toroidal_Surface*> toroidal_surfaces;
	DynArray<Point*> points;
	DynArray<Vertex_Point*> vertex_points;
	DynArray<Cartesian_Point*> cartesian_points;
	DynArray<Direction*> directions;
	DynArray<Axis2_Placement_3d*> axis2_placement_3ds;
	DynArray<Plane*> planes;
	Stable_Arena arena;
	Fixed_Array<size_t> instance_name_to_node;
	Fixed_Array<u32*> instance_name_to_items;
	DynArray<Read_String> diagnostic;

	void clear() {
		*this = {};
	}
};

struct AZ {};

template<typename T>
T* lookup(const parse_express_from_memory_result& out, const A242& a242, size_t id) {
	if (id >= a242.instance_name_to_items.size)
		return nullptr;
	u32 hash = *a242.instance_name_to_items[id];
	if (hash != T::hash)
		return nullptr;

	return (T*)a242.instance_name_to_items[id];
}
template<typename T>
extern bool is_type(
	const parse_express_from_memory_result& out, const A242& a242, size_t instance_name
);
extern void compile_express_from_memory(const parse_express_from_memory_result& out, A242& a242);

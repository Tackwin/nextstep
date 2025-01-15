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
		using x##_Pack_Inherited = xstd::Template_Pack<Item<case_insenstive_hash(#x)>, x##_Pack>;\
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
	struct_A242(Elementary_Surface, Surface_Pack) {
		ref<Axis2_Placement_3d> position = nullptr;
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
	struct_A242(Conic, Curve_Pack) {
		ref<Axis2_Placement_3d> position = nullptr;
	};
	struct_A242(Circle, Conic_Pack) {
		float radius;
	};
	struct_A242(Edge, Topological_Representation_Item_Pack) {
		ref<Vertex_Point> edge_start = nullptr;
		ref<Vertex_Point> edge_end = nullptr;
	};
	struct_A242(Edge_Curve, Edge_Pack, Geometric_Representation_Item_Pack) {
		ref<Curve> edge_geometry = nullptr;
		bool same_sense = false;
	};
#undef struct_A242

	DynArray<Cylindrical_Surface*> cylindrical_surfaces;
	DynArray<Edge_Curve*> edge_curves;
	DynArray<Curve*> curves;
	DynArray<Circle*> circles;
	DynArray<Point*> points;
	DynArray<Vertex_Point*> vertex_points;
	DynArray<Cartesian_Point*> cartesian_points;
	DynArray<Direction*> directions;
	DynArray<Axis2_Placement_3d*> axis2_placement_3ds;
	DynArray<Plane*> planes;
	Stable_Arena arena;
	Fixed_Array<size_t> instance_name_to_node;
	Fixed_Array<u32*> instance_name_to_items;

	void clear() {
		*this = {};
	}
};

extern void compile_express_from_memory(const parse_express_from_memory_result& out, A242& a242);

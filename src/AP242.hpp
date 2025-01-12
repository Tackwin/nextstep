#pragma once

#include "Memory.hpp"
#include "EXPRESS.hpp"
struct A242 {
	#include "A242_struct.hpp"

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

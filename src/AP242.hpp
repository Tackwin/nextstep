#pragma once

#include "Memory.hpp"
#include "EXPRESS.hpp"

struct Cartesian_Point {
	float x;
	float y;
	float z;
};

struct A242 {
	DynArray<Cartesian_Point> cartesian_points;
};

extern void parse_express_from_memory(const parse_express_from_memory_result& out, A242& a242);


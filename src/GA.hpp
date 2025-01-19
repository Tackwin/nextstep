#pragma once

#include "Templates.hpp"

namespace GA {

}

struct Elementf {
	float elements[16];
	union {
		float e;
		float e0;
		float e1;
		float e2;
		float e3;
		float e01;
		float e02;
		float e03;
		float e12;
		float e31;
		float e23;
		float e021;
		float e013;
		float e032;
		float e123;
		float e0123;
	};

	Elementf();
	Elementf(const Elementf& other);
	Elementf& operator=(const Elementf& other);
	Elementf(Elementf&& other);
	Elementf& operator=(Elementf&& other);
};

extern Elementf point(float x, float y, float z);
extern Elementf vector(float x, float y, float z);
extern Elementf plane(float a, float b, float c, float d);
extern Elementf plane_from_normal(float x, float y, float z);
extern Elementf plane_from_normal(const Elementf& normal);

extern void axpy(float a, const Elementf& x, Elementf& y);
extern Elementf operator*(float b, const Elementf& a);
extern Elementf operator*(const Elementf& a, float b);
extern Elementf operator/(const Elementf& a, float b);
extern Elementf operator+(const Elementf& a, const Elementf& b);
extern Elementf operator-(const Elementf& a, const Elementf& b);

extern Elementf dot_f(const Elementf& a, const Elementf& b);
static constexpr xstd::infix_operator<Elementf, decltype(dot_f), Elementf> dot = { dot_f };

extern Elementf wedge_f(const Elementf& a, const Elementf& b);
static constexpr xstd::infix_operator<Elementf, decltype(wedge_f), Elementf> wedge = { wedge_f };

extern Elementf project_point_into_plane(const Elementf& point, const Elementf& plane);

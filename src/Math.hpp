#pragma once

#include "Templates.hpp"

struct Vector3f {
	float x;
	float y;
	float z;
};

constexpr double PI = 3.14159265358979323846;
extern float fmod(float x, float y);
extern float abs(float x);
extern float cos(float x);
extern float sin(float x);
extern float acos(float x);
extern float sqrt(float x);


extern Vector3f operator-(const Vector3f& a, const Vector3f& b);
extern Vector3f operator+(const Vector3f& a, const Vector3f& b);
extern Vector3f operator*(float b, const Vector3f& a);
extern Vector3f operator*(const Vector3f& a, float b);
extern Vector3f operator/(const Vector3f& a, float b);


extern float dot_f(const Vector3f& a, const Vector3f& b);
static constexpr xstd::infix_operator<Vector3f, decltype(dot_f), float> dot = { dot_f };

extern Vector3f cross_f(const Vector3f& a, const Vector3f& b);
static constexpr xstd::infix_operator<Vector3f, decltype(cross_f), Vector3f> cross = { cross_f };

extern Vector3f project_point_into_plane(const Vector3f& point, const Vector3f& normal);

extern float angle_between_f(const Vector3f& a, const Vector3f& b);
static constexpr xstd::infix_operator<Vector3f, decltype(angle_between_f), float> angle_between = {
	angle_between_f
};

extern float angle_between_unit_f(const Vector3f& a, const Vector3f& b);
static constexpr xstd::infix_operator<Vector3f, decltype(angle_between_unit_f), float>
	angle_between_unit = { angle_between_unit_f };

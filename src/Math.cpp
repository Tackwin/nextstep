#include "Math.hpp"
#include "Common.hpp"

float dot_f(const Vector3f& a, const Vector3f& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vector3f cross_f(const Vector3f& a, const Vector3f& b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}
float angle_between_f(const Vector3f& a, const Vector3f& b) {
	return acos((a <dot> b) / ((a <dot> a) * (b <dot> b)));
}
float angle_between_unit_f(const Vector3f& a, const Vector3f& b) {
	return acos(a <dot> b);
}
Vector3f operator-(const Vector3f& a, const Vector3f& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}
Vector3f operator+(const Vector3f& a, const Vector3f& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}
Vector3f operator*(float b, const Vector3f& a) {
	return { a.x * b, a.y * b, a.z * b };
}
Vector3f operator*(const Vector3f& a, float b) {
	return b * a;
}
Vector3f operator/(const Vector3f& a, float b) {
	return { a.x / b, a.y / b, a.z / b };
}

Vector3f project_point_into_plane(const Vector3f& point, const Vector3f& normal) {
	return point - (normal <dot> point) * normal;
}

float fmod(float x, float y) {
	return x - y * (int)(x / y);
}

float cos_taylor_literal_6terms_pi(float x)
{
	x = fmod(x, (float)(2 * PI));
	char sign = 1;
	if (x > PI)
	{
		x -= (float)PI;
		sign = -1;
	}
	return sign *
		(1 -
		((x * x) / (2)) +
		((x * x * x * x) / (24)) -
		((x * x * x * x * x * x) / (720)) +
		((x * x * x * x * x * x * x * x) / (40320)) -
		((x * x * x * x * x * x * x * x * x * x) / (3628800)) +
		((x * x * x * x * x * x * x * x * x * x * x * x) / (479001600))
	);
}
float cos(float x) {
	return cos_taylor_literal_6terms_pi(x);
}
float sin(float x) {
	return cos(PI / 2 - x);
}

float abs(float x) {
	return x < 0 ? -x : x;
}
float acos(float x) {
	float negate = float(x < 0);
	x = abs(x);
	float ret = -0.0187293;
	ret = ret * x;
	ret = ret + 0.0742610;
	ret = ret * x;
	ret = ret - 0.2121144;
	ret = ret * x;
	ret = ret + PI / 2;
	ret = ret * sqrt(1.0-x);
	ret = ret - 2 * negate * ret;
	return negate * PI + ret;
}
float sqrt(float x) {
	i32 i = *(i32*)&x;
	i = 0x1fbd3f7d + (i >> 1);
	float y = *(float*)&i;
	y = (((y * y) + x) / y) * 0.5f;
	return y;
}
float asin(float x) {
	return PI / 2 - acos(x);
}

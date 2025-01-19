#include "GA.hpp"

Elementf::Elementf() {
	for (size_t i = 0; i < 16; i += 1) {
		elements[i] = 0;
	}
}
Elementf::Elementf(const Elementf& other) {
	*this = other;
}
Elementf& Elementf::operator=(const Elementf& other) {
	if (this == &other)
		return *this;
	for (size_t i = 0; i < 16; i += 1) {
		elements[i] = other.elements[i];
	}
	return *this;
}
Elementf::Elementf(Elementf&& other) {
	*this = other;
}
Elementf& Elementf::operator=(Elementf&& other) {
	if (this == &other)
		return *this;
	for (size_t i = 0; i < 16; i += 1) {
		elements[i] = other.elements[i];
	}
	return *this;
}

Elementf point(float x, float y, float z) {
	Elementf e;
	e.e032 = x;
	e.e013 = y;
	e.e021 = z;
	e.e123 = 1;
	return e;
}
Elementf vector(float x, float y, float z) {
	Elementf e;
	e.e032 = x;
	e.e013 = y;
	e.e021 = z;
	return e;
}
Elementf plane(float a, float b, float c, float d) {
	Elementf e;
	e.e1 = a;
	e.e2 = b;
	e.e3 = c;
	e.e0 = d;
	return e;
}
Elementf plane_from_normal(float x, float y, float z) {
	Elementf e;
	e.e1 = x;
	e.e2 = y;
	e.e3 = z;
	return e;
}
Elementf plane_from_normal(const Elementf& normal) {
	Elementf e;
	e.e1 = normal.e032;
	e.e2 = normal.e013;
	e.e3 = normal.e021;
	return e;
}


void axpy(float a, const Elementf& x, Elementf& y) {
	for (size_t i = 0; i < 15; i += 1) {
		y.elements[i] = a * x.elements[i] + y.elements[i];
	}
}

Elementf operator*(float b, const Elementf& a) {
	Elementf c;
	for (size_t i = 0; i < 15; i += 1) {
		c.elements[i] = b * a.elements[i];
	}
	return c;
}
Elementf operator*(const Elementf& a, float b) {
	return b * a;
}

Elementf operator+(const Elementf& a, const Elementf& b) {
	Elementf e = a;
	axpy(1, b, e);
	return e;
}
Elementf operator-(const Elementf& a, const Elementf& b) {
	Elementf e = a;
	axpy(-1, b, e);
	return e;
}

Elementf operator*(const Elementf& a, const Elementf& b) {
	Elementf c;

	#define mscalar(x)\
		+ a.e * x
	#define mvector(x, y, z, w)\
		+ a.e0 * x + a.e1 * y + a.e2 * z + a.e3 * w
	#define mbivector(x, y, z, w, u, v)\
		+ a.e01 * x + a.e02 * y + a.e03 * z + a.e12 * w + a.e31 * u + a.e23 * v
	#define mtrivector(x, y, z, w)\
		+ a.e021 * x + a.e013 * y + a.e032 * z + a.e123 * w
	#define mantiscalar(x)\
		+ a.e0123 * x

	c.e =
		mscalar(b.e)
		mvector(0, b.e1, b.e2, b.e3)
		mbivector(0, 0, b.e01, b.e02, b.e03, b.e12)
		mtrivector(0, 0, 0, b.e0123)
		mantiscalar(0);
	c.e0 =
		mscalar(b.e0)
		mvector(b.e, b.e01, b.e02, b.e03)
		mbivector(-b.e1, -b.e2, -b.e3, b.e021, b.e013, b.e032)
		mtrivector(b.e12, b.e31, b.e23, -b.e0123)
		mantiscalar(b.e123);
	c.e1 =
		mscalar(b.e1)
		mvector(0, b.e, b.e12, -b.e31)
		mbivector(0, 0, 0, -b.e2, b.e3, -b.e123)
		mtrivector(0, 0, 0, -b.e23)
		mantiscalar(0);
	c.e2 =
		mscalar(b.e2)
		mvector(0, -b.e12, b.e, b.e23)
		mbivector(0, 0, 0, b.e1, -b.e123, -b.e3)
		mtrivector(0, 0, 0, -b.e31)
		mantiscalar(0);
	c.e3 =
		mscalar(b.e3)
		mvector(0, b.e31, -b.e23, b.e)
		mbivector(0, 0, 0, -b.e123, -b.e1, +b.e2)
		mtrivector(0, 0, 0, -b.e12)
		mantiscalar(0);
	c.e01 =
		mscalar(b.e01)
		mvector(-b.e1, b.e0, -b.e021, b.e013)
		mbivector(b.e, b.e12, -b.e31, -b.e02, b.e03, -b.e0123)
		mtrivector(-b.e2, b.e3, -b.e123, b.e032)
		mantiscalar(-b.e23);
	c.e02 =
		mscalar(b.e02)
		mvector(-b.e2, b.e021, b.e0, -b.e032)
		mbivector(-b.e12, -b.e31, b.e23, b.e, -b.e123, -b.e01)
		mtrivector(b.e1, -b.e3, b.e123, -b.e013)
		mantiscalar(b.e31);
	c.e03 =
		mscalar(b.e03)
		mvector(-b.e3, -b.e013, b.e032, b.e0)
		mbivector(b.e31, -b.e23, b.e, -b.e0123, -b.e01, b.e02)
		mtrivector(-b.e123, -b.e1, b.e2, b.e021)
		mantiscalar(-b.e12);
	c.e12 =
		mscalar(b.e12)
		mvector(0, -b.e2, b.e1, b.e123)
		mbivector(0, 0, 0, b.e, -b.e23, b.e31)
		mtrivector(0, 0, 0, b.e3)
		mantiscalar(0);
	c.e31 =
		mscalar(b.e31)
		mvector(0, b.e3, b.e123, -b.e1)
		mbivector(0, 0, 0, b.e23, b.e, -b.e12)
		mtrivector(0, 0, 0, b.e2)
		mantiscalar(0);
	c.e23 =
		mscalar(b.e23)
		mvector(0, b.e123, -b.e3, b.e2)
		mbivector(0, 0, 0, -b.e31, b.e12, b.e)
		mtrivector(0, 0, 0, b.e1)
		mantiscalar(0);
	c.e021 =
		mscalar(b.e021)
		mvector(-b.e12, b.e02, -b.e01, b.e0123)
		mbivector(-b.e2, b.e1, b.e123, -b.e0, -b.e032, b.e013)
		mtrivector(b.e, -b.e23, b.e31, -b.e03)
		mantiscalar(b.e03);
	c.e013 =
		mscalar(b.e013)
		mvector(-b.e31, -b.e03, -b.e0123, b.e01)
		mbivector(b.e3, b.e123, -b.e1, b.e032, -b.e0, -b.e021)
		mtrivector(b.e23, b.e013, -b.e12, b.e02)
		mantiscalar(b.e2);
	c.e032 =
		mscalar(b.e032)
		mvector(-b.e23, -b.e0123, b.e03, -b.e02)
		mbivector(b.e123, -b.e3, b.e2, -b.e013, b.e021, -b.e0)
		mtrivector(-b.e31, b.e12, b.e, -b.e01)
		mantiscalar(b.e1);
	c.e123 =
		mscalar(b.e123)
		mvector(0, b.e23, b.e31, b.e12)
		mbivector(0, 0, 0, b.e3, b.e2, b.e1)
		mtrivector(0, 0, 0, b.e)
		mantiscalar(0);
	c.e0123 = 0;
	for (size_t i = 0; i < 16; i += 1) {
		float sign = +1;
		if (i == 1 || i == 2 || i == 3 || i == 4) {
			sign = -1;
		}
		c.elements[i] += sign * a.elements[i] * b.elements[15 - i];
	}
	return c;
}

Elementf operator/(const Elementf& a, float b) {
	Elementf e;
	for (size_t i = 0; i < 16; i += 1) {
		e.elements[i] = a.elements[i] / b;
	}
	return e;
}

Elementf dot_f(const Elementf& a, const Elementf& b) {
	return (a * b + b * a) / 2;
}

Elementf wedge_f(const Elementf& a, const Elementf& b) {
	return (a * b - b * a) / 2;
}

extern Elementf project_point_into_plane(const Elementf& point, const Elementf& plane) {
	return (plane <dot> point) * point;
}
#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Plane.h"

struct Line {
	Vec3 v; // direction vector v
	Vec3 m; // moment vector m
	Line() = default;
	Line(float vx, float vy, float vz, float mx, float my, float mz) : v(vx, vy, vz), m(mx, my, mz) {}
	Line(const Vec3& _v, const Vec3& _m) { // can't use initializer list because we need to copy initialize
		v = _v;
		m = _m;
	}
	Line& operator*=(const Trans4&);
	friend Line operator*(const Line&, const Trans4&); // transform a plane
	// dont define an operator ^= because we dont want to chain the operations and that's what that is used for
	friend Line operator^(const Point3&, const Point3&); // wedge product of two points is a line containing those two points
	friend Plane operator^(const Line&, const Point3&); // the wedge product of a line and a point is a plane
	friend Plane operator^(const Point3&, const Line&);
	friend Vec4 operator^(const Line&, const Plane&); // antiwedge product of a line and a plane is the point of intersection
	friend Vec4 operator^(const Plane&, const Line&); // antiwedge product of a line and a plane is the point of intersection
	friend float operator^(const Line&, const Line&); // antiwedge product of a line and a line is their distance
};


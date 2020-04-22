#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Line.h"

struct Plane {
	float x;
	float y;
	float z; 
	float w;
	Plane() = default;
	Plane(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	Plane(const Vec3& n, float d) : x(n.x), y(n.y), z(n.z), w(d) {}
	const Vec3& get_normal() const;
	Plane& operator*=(const Trans4&);
	friend Plane operator*(Plane, const Trans4&); // transform a plane
	friend Line operator^(const Plane&, const Plane&); // antiwedge product of two planes is their line of intersection
	friend float operator^(const Point3&, const Plane&); // anti wedge product between a point and a plane is the distance p is from f
	friend float operator^(const Plane&, const Point3&); // andtiwedge product between a point and a plane is the distance p is from f
};
float dot(const Plane&, const Vec3&);
float dot(const Plane&, const Point3&);
Trans4 make_reflection(const Plane&);

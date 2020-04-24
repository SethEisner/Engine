#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Line.h"

struct Plane;

// implicit plane dot(n, p) + d = 0 
struct alignas(16) Plane {
	Plane() = default;
	Plane(float _x, float _y, float _z, float _d) : n(_x, _y, _z), d(_d) {
		n.normalize();
	}
	Plane(const Vec3& _n, const Point3& _q) : n(_n), d(-dot(n,_q)) { // plane with normal _n that intersects point _q
		n.normalize();
	} // can't use initializer list because we need to copy initialize
	Plane(const Point3& p, const Point3& q, const Point3& r) : n(cross(p-q, r-q)), d(-dot(n,q)) { // plane that contains the three points
		n.normalize();
	}
	Plane(const Plane& f) : n(f.n), d(f.d) {} // dont need to normalize because the normal vector should already be normalized
	Plane& operator=(const Plane&) = default;
	inline Plane& operator*=(const Trans4& h) { // need to pass in the inverted transform
		float a = this->n.x * h(0, 0) + this->n.y * h(1, 0) + this->n.z * h(2, 0);
		float b = this->n.x * h(0, 1) + this->n.y * h(1, 1) + this->n.z * h(2, 1);
		float c = this->n.x * h(0, 2) + this->n.y * h(1, 2) + this->n.z * h(2, 2);
		float d = this->n.x * h(0, 3) + this->n.y * h(1, 3) + this->n.z * h(2, 3) + this->d;

		this->n.x = a;
		this->n.y = b;
		this->n.z = c;
		this->n.z = d;

		return *this;
	}
	friend inline Plane operator*(Plane l, const Trans4& h) { // need to pass in the inverted transform 
		l *= h;
		return l;
	}
	friend inline float dot(const Plane& f, const Vec3& v) {
		return f.n.x * v.x + f.n.y * v.y + f.n.z * v.z; //dot(f.n, v);
	}
	friend inline float dot(const Plane& f, const Point3& p) {
		// do the dot product and then add the distance to the origin
		return f.n.x * p.x + f.n.y * p.y + f.n.z * p.z + f.d;
	}
	friend inline float distance(const Plane& f, const Point3& p) { // distance is same as dot product, just add overload because why not?
		return dot(f, p);
	}
	friend inline bool intersection(const Plane& f, const Line& l, Point3& intersection_point) { // point of intersection between a line and a plane
		float fv = dot(f, l.get_direction());
		if (fv * fv > EPSILON * EPSILON) { // they're not parallel
			float fp = dot(f, l.get_point());
			intersection_point = l.get_point() -  ((fp / fv) * l.get_direction());
			return true;
		}
		return false;
	}
	inline Vec3 get_normal() {
		return n;
	}
	inline float get_distance() { // returns d which is the distance the plane is from the origin
		return d;
	}
private:
	Vec3 n; // normal vector
	float d; // distance from origin d = -dot(n,q)
};

/*struct Plane {
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
*/
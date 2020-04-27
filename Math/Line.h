#pragma once
#include "Vector.h"
#include "Matrix.h"
//#include "Plane.h"

struct Line;

// a line is a direction and a point
struct Line {
	Line() = default;
	Line(float vx, float vy, float vz, float x, float y, float z) : dir(vx, vy, vz), p(x, y, z) {}
	Line(const Vec3& _v, const Point3& _p) : dir(_v), p(_p) { // line with direction _v and starting at point _p
		dir.normalize();
	} // can't use initializer list because we need to copy initialize
	Line(const Point3& p, const Point3& q) : dir(q - p), p(p) { // line that intersects the two points
		dir.normalize();
	}
	Line(const Line& l) : dir(l.dir), p(l.p) {} // dont need to normalize because the direction vector should already be normalized
	Line& operator=(const Line&) = default;
	inline Line& operator*=(const Trans4& h) {
		// transform the direction and point normally
		this->dir = h * this->dir;
		this->p = h * this->p;
		return *this;
	}
	friend inline Line operator*(const Trans4& h, const Line& l) {
		return Line(normalize(h * l.dir), h * l.p); // create a new line from the two transformations
	}
	inline Point3 point_at(float t) {
		return this->p + this->dir * t;
	}
	inline Point3 point_at(float t) const {
		return this->p + this->dir * t;
	}
	// dont want the user to mess with the direction because we assume that it is always normalized
	const Vec3 get_direction() const{
		return dir;
	}
	const Point3 get_point() const {
		return p;
	}
	friend float distance(const Line& l, const Point3& q) {
		Vec3 uxv = cross(q - l.p, l.dir);
		return sqrtf(dot(uxv, uxv)); // take sqrt of squared magnitude to get the final distance
		// dont need to divide by magnitude of dir because it is always normalized
	}
	friend float distance(const Line& l1, const Line& l2) {
		Vec3 dp = l2.p - l1.p;
		float v12 = dot(l1.dir, l1.dir);
		float v22 = dot(l2.dir, l2.dir);
		float v1v2 = dot(l1.dir, l2.dir);

		float det = v1v2 * v1v2 - v12 * v22;
		// check if the lines are parallel. determinant of zero means the lines are linearly dependent and therefore parallel
		if (det * det > EPSILON * EPSILON) {
			det = 1.0f / det;
			float dpv1 = dot(dp, l1.dir);
			float dpv2 = dot(dp, l2.dir);
			float t1 = (v1v2 * dpv2 - v22 * dpv1) * det;
			float t2 = (v12 * dpv2 - v1v2 * dpv1) * det;
			return magnitude(dp + l2.dir * t2 - l1.dir * t1);
		}
		Vec3 a = cross(dp, l1.dir);
		return sqrtf(dot(a, a)); // dont need to normalize because v12 is always 1 when dir is normalized
	}
	friend bool intersection(const Line& l1, const Line& l2, Point3& intersection) { // modify the intersection argument if one exists
		Mat3 V(l1.dir.x, -l2.dir.x, 1.0f,
			   l1.dir.y, -l2.dir.y, 1.0f, 
			   l1.dir.z, -l2.dir.z, 1.0f );
		Vec3 P(l2.p.x - l1.p.x, l2.p.y - l1.p.y, l2.p.z - l1.p.z);
		float _det = det(V);
		if (_det * _det > EPSILON * EPSILON) { // matrix is invertible if det != 0
			Vec3 t = inverse(V) * P;
			intersection = l1.point_at(t.x); // x coordinate of 
			return true;
		}
		return false;
	}
private:
	Vec3 dir;
	Point3 p;
};
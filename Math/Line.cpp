#include "Line.h"
#include "Matrix.h"
#include "Vector.h"

/*
Line& Line::operator*=(const Trans4& h) {
	// get the column vectors of the transform h
	Vec3 _v1 = cross(Vec3(h(0, 1), h(1, 1), h(2, 1)), Vec3(h(0, 2), h(1, 2), h(2, 2)));
	Vec3 _v2 = cross(Vec3(h(0, 2), h(1, 2), h(2, 2)), Vec3(h(0, 0), h(1, 0), h(2, 0)));
	Vec3 _v3 = cross(Vec3(h(0, 0), h(1, 0), h(2, 0)), Vec3(h(0, 1), h(1, 1), h(2, 1)));
	Mat3 adj(_v1.x, _v1.y, _v1.z,
			 _v2.x, _v2.y, _v2.z,
			 _v3.x, _v3.y, _v3.z);
	const Point3& t = h.get_translation();
	Vec3 _v = h * this->dir;
	Vec3 _m = adj * this->m + cross(t, _v);
	this->v = _v;
	this->m = _m;
	return *this;

}
Line operator*(Line l, const Trans4& h) { // transform a plane
	l *= h;
	return l;
}
// dont define an operator ^= because we dont want to chain the operations and that's what that is used for
Line operator^(const Point3& p, const Point3& q) { // wedge product of two points is a line containing those two points
	return Line(q.x-p.x, 
				q.y - p.y, 
				p.z - p.z, 
				p.y * q.z - p.z * q.y, 
				p.z * q.x - p.x * q.z, 
				p.x * q.y - p.y * q.x);
}
Plane operator^(const Line& l, const Point3& p) { // the wedge product of a line and a point is a plane
	return Plane(l.v.y * p.z - l.v.z * p.y + l.m.x,
				 l.v.z * p.x - l.v.x * p.z + l.m.y,
				 l.v.x * p.y - l.v.y * p.x + l.m.z,
				 -l.m.x * p.x - l.m.y * p.y - l.m.z * p.z);
}
Plane operator^(const Point3& p, const Line& l) {
	return (l ^ p);
}
Vec4 operator^(const Line& l, const Plane& f) { // antiwedge product of a line and a plane is the point of intersection
	return Vec4(l.m.y * f.z - l.m.z * f.y + l.v.x * f.w,
				l.m.z * f.x - l.m.x * f.z + l.v.y * f.w,
				l.m.x * f.y - l.m.y * f.x + l.v.z * f.w,
				-l.v.x * f.x - l.v.y * f.y - l.v.z * f.z);
}
Vec4 operator^(const Plane& f, const Line& l) { // antiwedge product of a line and a plane is the point of intersection
	return (l ^ f);
}
float operator^(const Line& l1, const Line& l2) { // antiwedge product of a line and a line is their distance
	return (-(dot(l1.v, l2.m) + dot(l2.v, l1.m)));
}
*/
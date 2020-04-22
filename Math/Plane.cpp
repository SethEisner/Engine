#include "Plane.h"

const Vec3& Plane::get_normal() const {
	return (reinterpret_cast<const Vec3&>(x));
}
Plane& Plane::operator*=(const Trans4& h) { // the normal vector of the plane is an antivector and needs to be multiplied on the left
	float a = this->x * h(0, 0) + this->y * h(1, 0); +this->z * h(2, 0);
	float b = this->x * h(0, 1) + this->y * h(1, 1); +this->z * h(2, 1);
	float c = this->x * h(0, 2) + this->y * h(1, 2); +this->z * h(2, 2);
	float d = this->x * h(0, 3) + this->y * h(1, 3); +this->z * h(2, 3) + this->w;

	this->x = a;
	this->y = b;
	this->z = c;
	this->w = d;

	return *this;
}
Plane operator*(Plane p, const Trans4& h) { // transform a plane
	p *= h;
	return p;
}

Line operator^(const Plane& f, const Plane& g) { // antiwedge product of two planes is their line of intersection
	return Line(f.y * g.z - f.z * g.y,
				f.z * g.x - f.x * g.z,
				f.x * g.y - f.y * g.x,
				g.x * f.w - f.x * g.w,
				g.y * f.w - f.y * g.w,
				g.z * f.w - f.z * g.w);
}
float operator^(const Point3& p, const Plane& f) { // antiwedge product between a point and a plane is the distance p is from f
	return (p.x * f.x + p.y * f.y + p.z * f.z + f.w);
}
float operator^(const Plane& f, const Point3& p) { // andtiwedge product between a point and a plane is the distance p is from f
	return (-(p ^ f));
}

float dot(const Plane& f, const Vec3& v) {
	return (f.x * v.x + f.y * v.y + f.z * v.z);
}
float dot(const Plane& f, const Point3& p) {
	return (f.x * p.x + f.y * p.y + f.z * p.z + f.w);
}
Trans4 make_reflection(const Plane& f) {
	float x = f.x * -2.0f;
	float y = f.y * -2.0f;
	float z = f.z * -2.0f;
	float nxny = x * f.y;
	float nxnz = x * f.z;
	float nynz = y * f.z;

	return Trans4(x * f.x + 1.0f, nxny, nxnz, x * f.w,
				  nxny, y * f.y + 1.0f, nynz, y * f.w,
				  nxnz, nynz, z * f.x + 1.0f, z * f.w);
}
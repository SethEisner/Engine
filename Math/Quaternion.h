#pragma once
#include "Vector.h"
#include "Matrix.h"

struct Quaternion
{
	float x;
	float y;
	float z;
	float w;
	// make all constructors convert the created quaternion into a unit quaternion because multiplication 
	Quaternion(); // cannot convert this to a unit quaternion
	Quaternion(float _x, float _y, float _z, float _w);
	explicit Quaternion(const Vec3& v, float rads); // create a quaternion from a unit vector and a scalar radians
	Quaternion& operator=(const Quaternion& q) = default;
	~Quaternion() = default;
	const Vec3& get_vec() const;
	Mat3 get_rotation_matrix() const; // create a pure rotation matrix from a quaternions
	void set_rotation_matrix(const Trans4&); // create a quaternion from a pure rotation transformation matrix
	void set_rotation_matrix(const Mat3& m); // create a quaternion from a pure rotation matrix
	Quaternion& operator*=(const Quaternion& rhs); // multiplication of two unit quaternions results in a new unit quaternion
	friend Quaternion operator*(Quaternion lhs, const Quaternion& rhs); 
};
Quaternion normalize(const Quaternion& q); // noprmalize the quaternion into a unit quaternion 
float magnitude(const Quaternion& q); // get the magnitude of this quaternion
Vec3 transform(const Vec3& v, const Quaternion& q); // transform a vector v by UNIT quaternion q (performs sandwich product) 


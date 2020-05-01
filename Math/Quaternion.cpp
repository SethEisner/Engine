#include "Quaternion.h"

Quaternion::Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {} // q = 1 represents a rotation of zero degrees and is a unit quaternion so no need to normalize
Quaternion::Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {
	normalize(*this);
}
Quaternion::Quaternion(const Vec3& v, float rads) { // create a quaternion from a unit vector and a scalar radians
	float sin2 = sinf(rads * 0.5f);
	x = v.x * sin2;
	y = v.y * sin2;
	z = v.z * sin2;
	w = cosf(rads * 0.5f);
}
const Vec3& Quaternion::get_vec() const { // returns the imaginary part of the quaternion, NOT the vector the quaternion rotates around
	return reinterpret_cast<const Vec3&>(x);
}
Mat3 Quaternion::get_rotation_matrix() const { // create a pure rotation matrix from a unit quaternion
	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;
	float w2 = w * w;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	return Mat3(1.0f - 2.0f * (y2 + z2), 2.0f * (xy - wz), 2.0f * (xz + wy),
				2.0f * (xy + wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz - wx),
				2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (x2 + y2));
}
void Quaternion::set_rotation_matrix(const Trans4& m) {
	float m00 = m(0, 0);
	float m11 = m(1, 1);
	float m22 = m(2, 2);
	float sum = m00 + m11 + m22;
	if (sum > 0.0f) { // when sum is positive then |w| >= 1/2 so division isn's a problem
		w = sqrtf(sum + 1.0f) * 0.5f;
		float f = 0.25f / w;
		x = (m(2, 1) - m(1, 2)) * f;
		y = (m(0, 2) - m(2, 0)) * f;
		z = (m(1, 0) - m(0, 1)) * f;
	}
	else if ((m00 > m11) && (m00 > m22)) { // if sum < 0 then we look for the largest number on the diagonal and then divide by that 
		x = sqrtf(m00 - m11 - m22 + 1.0f) * 0.5f;
		float f = 0.25f / x;
		y = (m(1, 0) - m(0, 1)) * f;
		z = (m(0, 2) + m(2, 0)) * f;
		w = (m(2, 1) - m(1, 2)) * f;
	}
	else if (m11 > m22) {
		y = sqrtf(m11 - m00 - m22 + 1.0f) * 0.5f;
		float f = 0.25f / x;
		x = (m(1, 0) + m(0, 1)) * f;
		z = (m(2, 1) + m(1, 2)) * f;
		w = (m(0, 2) - m(2, 0)) * f;
	}
	else {
		z = sqrtf(m22 - m00 - m11 + 1.0f) * 0.5f;
		float f = 0.25f / z;
		x = (m(0, 2) + m(2, 0)) * 0.5f;
		y = (m(2, 1) + m(1, 2)) * 0.5f;
		w = (m(1, 0) - m(0, 1)) * 0.5f;
	}
}
void Quaternion::set_rotation_matrix(const Mat3& m) { // set quaternion to a unit quaternion from a pure rotation matrix
	this->set_rotation_matrix(Trans4(m)); // convert the mat3 to a transform matrix and then get the rotation
}
Quaternion normalize(const Quaternion& q) { // normalize the quaternion into a unit quaternion 
	float inv = 1.0f / magnitude(q);
	return Quaternion(q.x * inv, q.y * inv, q.z * inv, q.w * inv);
}
float magnitude(const Quaternion& q) { // get the magnitude of this quaternion
	const Vec3& v = q.get_vec();
	return sqrtf(dot(v, v) + q.w * q.w);
}
Quaternion& Quaternion::operator*=(const Quaternion& rhs) { // multiplication of two unit quaternions results in a new unit quaternion
	float a = this->w * rhs.x + this->x * rhs.w + this->y * rhs.z - this->z * rhs.y;
	float b = this->w * rhs.y - this->x * rhs.z + this->y * rhs.w + this->z * rhs.x;
	float c = this->w * rhs.z + this->x * rhs.y - this->y * rhs.x + this->z * rhs.w;
	float d = this->w * rhs.w - this->x * rhs.x - this->y * rhs.y - this->z * rhs.z;
	this->x = a;
	this->y = b;
	this->z = c;
	this->w = d;
	return *this;
}
Quaternion operator*(Quaternion lhs, const Quaternion& rhs) {
	lhs *= rhs;
	return lhs;
}
// the quaternion we give this function can be the product of two quaternions to combine multiple rotations into one transform call
// q = p_parent * q_child => q * v * q^-1 // applies parent first then child 
Vec3 transform(const Vec3& v, const Quaternion& q) { // transform a vector v by UNIT quaternion q by performing the sandwich product with q*
	// simplified method so we dont need to convert v to a pure quaternion and do two quaternion multiplication
	const Vec3& b = q.get_vec();
	float b2 = dot(b, b);
	// returns the transformed vector
	return (v * (q.w * q.w - b2) + b * (dot(v, b) * 2.0f) + cross(b, v) * (q.w * 2.0f));
}
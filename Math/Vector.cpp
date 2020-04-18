#include "Vector.h"
#include "../Utilities/Utilities.h"

// vec2 functions
Vec2& Vec2::operator+=(const Vec2& rhs) { // Vec2 += Vec2
	this->x += rhs.x;
	this->y += rhs.y;
	return *this;
}
Vec2 operator+(Vec2 lhs, const Vec2& rhs) { // Vec2 + Vec2
	lhs += rhs;
	return lhs;
}
Vec2& Vec2::operator-=(const Vec2& rhs) { // Vec2 -= Vec2
	this->x -= rhs.x;
	this->y -= rhs.y;
	return *this;
}
Vec2 operator-(Vec2 lhs, const Vec2& rhs) { // Vec2 - Vec2
	lhs -= rhs;
	return lhs;
}
Vec2& Vec2::operator*=(const float rhs) { // Vec2 *= 9.0f
	this->x *= rhs;
	this->y *= rhs;
	return *this;
}
Vec2 operator*(Vec2 lhs, const float rhs) { // Vec2 * 9.0f
	lhs *= rhs;
	return lhs;
}
Vec2 operator*(const float lhs, Vec2 rhs) { // 9.0f * Vec2
	rhs *= lhs;
	return rhs;
}
float dot(const Vec2& lhs, const Vec2& rhs) {
	return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}
float cross(const Vec2& lhs, const Vec2& rhs) {
	return (lhs.x * rhs.y) - (lhs.y * rhs.x);
}
Vec2 normalize(const Vec2& rhs) {
	if (rhs.x * rhs.x + rhs.y * rhs.y > EPSILON) { // we dont have a super small basically zero vector
		float mag = fast_invsqrt(rhs.x * rhs.x + rhs.y * rhs.y);
		return Vec2(rhs.x * mag, rhs.y * mag);
	}
	return Vec2(0.0f, 0.0f);
}
Vec2 perp(const Vec2& u) {
	return Vec2(-u.x, u.y);
}
Vec2 projection(const Vec2& a, const Vec2& b) { // project a onto b
	return (dot(b, b) < EPSILON) ? (Vec2(0.f, 0.0f)) : (dot(a, b) / dot(b, b) * b);
}
Vec2 rejection(const Vec2& a, const Vec2& b) { // reject a onto b
	return (a - projection(a, b));
}

// vec3 functions
Vec3::Vec3(const Vec4& v4) : x(v4.x), y(v4.y), z(v4.z) { // convert from homogenous coordinates to a Vec3
	float div;
	(v4.w == 0.0f) ? div = 1.0f : div = 1.0f / v4.w; // check if the Vec4 is a vector or a point
	x *= div;
	y *= div;
	z *= div;
}
// Vec3& Vec3::operator=(const Vec4& v4) { // when assigning from a Vec4 we need to homogenize the point implciti assignment like this seems super bug prone
// 	float div;
// 	(v4.w == 0.0f) ? div = 1.0f : div = 1.0f / v4.w; // check if the Vec4 is a vector or a point
// 	this->x = v4.x * div;
// 	this->y = v4.y * div;
// 	this->z = v4.z * div;
// 	return *this;
// }
Vec3& Vec3::operator+=(const Vec3& rhs) { // Vec3 += Vec3
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	return *this;
}
Vec3 operator+(Vec3 lhs, const Vec3& rhs) { // Vec3 + Vec3
	lhs += rhs;
	return lhs;
}
Vec3& Vec3::operator-=(const Vec3& rhs) { // Vec3 -= Vec3
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	return *this;
}
Vec3 operator-(Vec3 lhs, const Vec3& rhs) { // Vec3 - Vec3
	lhs -= rhs;
	return lhs;
}
Vec3& Vec3::operator*=(const float rhs) { // Vec3 *= 9.0f
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	return *this;
}
Vec3 operator*(Vec3 lhs, const float rhs) { // Vec3 * 9.0f
	lhs *= rhs;
	return lhs;
}
Vec3 operator*(const float lhs, Vec3 rhs) { // 9.0f * Vec3
	rhs *= lhs;
	return rhs;
}
float dot(const Vec3& lhs, const Vec3& rhs) {
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}
Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
	return Vec3((lhs.y * rhs.z) - (lhs.z * rhs.y),
		(lhs.z * rhs.x) - (lhs.x * rhs.z),
		(lhs.x * rhs.y) - (lhs.y * rhs.x));
}
Vec3 normalize(const Vec3& rhs) {
	if (rhs.x * rhs.x + rhs.y * rhs.y + rhs.z * rhs.z > EPSILON) { // we dont have a super small basically zero vector
		float mag = fast_invsqrt(rhs.x * rhs.x + rhs.y * rhs.y + rhs.z * rhs.z);
		return Vec3(rhs.x * mag, rhs.y * mag, rhs.z * mag);
	}
	return Vec3(0.0f, 0.0f, 0.0f);
}
Vec3 projection(const Vec3& a, const Vec3& b) { // project a onto b
	return (dot(b, b) < EPSILON) ? (Vec3(0.f, 0.0f, 0.0f)) : (dot(a, b) / dot(b, b) * b); // make sure we dont divide by a super small number due to precision errors
}
Vec3 rejection(const Vec3& a, const Vec3& b) { // reject a onto b
	return (a - projection(a, b));
}
Vec3 homogenize(const Vec3& rhs) { // for 2d version of homogenous coordinates
	float div;
	Vec3 result;
	(rhs.z == 0) ? (div = 1.0f, result.z = 0.f) : (div = 1.0 / rhs.z, result.z = 1.0f);
	result.x = rhs.x * div;
	result.y = rhs.y * div;
	return result;
}

// Vec4 functions
Vec4& Vec4::operator+=(const Vec4& rhs) { // Vec4 lhs += (Vec4) rhs -> vector // adding two homogeneous coordinates (cant add points)
	this->x += rhs.x;
	this->y += rhs.y;
	this->z += rhs.z;
	this->w += rhs.w;
	return *this;
}
Vec4 operator+(Vec4 lhs, const Vec4& rhs) { // Vec4 + Vec4 -> vector
	lhs += rhs;
	return lhs;
}
Vec4& Vec4::operator-=(const Vec4& rhs) { // point - point -> vector so w needs to always become 0.0f
	this->x -= rhs.x;
	this->y -= rhs.y;
	this->z -= rhs.z;
	this->w -= rhs.w;
	return *this;
}
Vec4 operator-(Vec4 lhs, const Vec4& rhs) {
	lhs -= rhs;
	return lhs;
}
Vec4& Vec4::operator*=(const float rhs) {
	this->x *= rhs;
	this->y *= rhs;
	this->z *= rhs;
	this->w *= rhs;
	return *this;
}
Vec4 operator*(Vec4 lhs, const float rhs) { // Vec3 * 9.0f
	lhs *= rhs;
	return lhs;
}
Vec4 operator*(const float lhs, Vec4 rhs) { // 9.0f * Vec3
	rhs *= lhs;
	return rhs;
}
float dot(const Vec4& lhs, const Vec4& rhs) {
	return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}
Vec4 normalize(const Vec4& rhs) {
	// w coordinate is zero so it doenst change the magnitude for a vector in homogeneous coordinates
	if (rhs.x * rhs.x + rhs.y * rhs.y + rhs.z * rhs.z + rhs.w * rhs.w > EPSILON) { // we dont have a super small basically zero vector
		float mag = fast_invsqrt(rhs.x * rhs.x + rhs.y * rhs.y + rhs.z * rhs.z + rhs.w * rhs.w);
		return Vec4(rhs.x * mag, rhs.y * mag, rhs.z * mag, rhs.w * mag);
	}
	return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
	
}
Vec4 projection(const Vec4& a, const Vec4& b) {
	return (dot(b, b) < EPSILON) ? (Vec4(0.f, 0.0f, 0.0f, 0.0f)) : (dot(a, b) / dot(b, b) * b);
}
Vec4 rejection(const Vec4& a, const Vec4& b) { // reject a onto b
	return (a - projection(a, b));
}
Vec4 homogenize(const Vec4& rhs) {
	float div;
	Vec4 result;
	(rhs.w * rhs.w < EPSILON) ? (div = 1.0f, result.w = 0.f) : (div = 1.0 / rhs.w, result.w = 1.0f); // set's the w here too
	result.x = rhs.x * div;
	result.y = rhs.y * div;
	result.z = rhs.z * div;
	return result;
}
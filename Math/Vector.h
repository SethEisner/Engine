#pragma once
#include <math.h>

struct Vec2;
struct Vec3;
struct Vec4;

struct Vec2 {
	float x;
	float y;
	Vec2() : x(0.0f), y(0.0f) {}
	explicit Vec2(float _x, float _y) : x(_x), y(_y) {}
	~Vec2() = default;
	Vec2& operator=(const Vec2&) = default; //use the default copy assignment operator
private: // private so we can't call them directly
	Vec2& operator+=(const Vec2& rhs);
	friend Vec2 operator+(Vec2 lhs, const Vec2& rhs);
	Vec2& operator-=(const Vec2& rhs);
	friend Vec2 operator-(Vec2 lhs, const Vec2& rhs);
	Vec2& operator*=(const float rhs);
	friend Vec2 operator*(Vec2 lhs, const float rhs);
	friend Vec2 operator*(const float lhs, Vec2 rhs);
};
float dot(const Vec2& lhs, const Vec2& rhs);
float cross(const Vec2& lhs, const Vec2& rhs);
Vec2 normalize(const Vec2& rhs);

struct Vec3 { // also works as a point type
	float x;
	float y;
	float z;
	Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vec3(const Vec4& v4);
	explicit Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	~Vec3() = default;
	Vec3& operator=(const Vec3&) = default; //use the default copy assignment operator
	Vec3& operator=(const Vec4& v4);
private: // private so we can't call them directly
	Vec3& operator+=(const Vec3& rhs);
	friend Vec3 operator+(Vec3 lhs, const Vec3& rhs);
	Vec3& operator-=(const Vec3& rhs);
	friend Vec3 operator-(Vec3 lhs, const Vec3& rhs);
	Vec3& operator*=(const float rhs);
	friend Vec3 operator*(Vec3 lhs, const float rhs);
	friend Vec3 operator*(const float lhs, Vec3 rhs);
};
float dot(const Vec3& lhs, const Vec3& rhs);
Vec3 cross(const Vec3& lhs, const Vec3& rhs);
Vec3 normalize(const Vec3& rhs);
Vec3 homogenize(const Vec3& rhs); // can turn a 2d point into a homogeneous coordinate

struct Vec4 { // homogeneous coordinates vec w=0 point w = 1
	float x;
	float y;
	float z;
	float w;
	Vec4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {} // create a default initialized vector 4
	Vec4(const Vec3& rhs, float _w) : x(rhs.x), y(rhs.y), z(rhs.z), w(_w) {} // create a vector 4 from a vector 3
	explicit Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	~Vec4() = default;
	Vec4& operator=(const Vec4&) = default; //use the default copy assignment operator
private:
	// adding two points makes no sense so these Vec4 must be vectors (w = 0) and returns a vector (w = 0)
	Vec4& operator+=(const Vec4& rhs);
	friend Vec4 operator+(Vec4 lhs, const Vec4& rhs);
	Vec4& operator-=(const Vec4& rhs);
	friend Vec4 operator-(Vec4 lhs, const Vec4& rhs);
};
Vec4 normalize(const Vec4& rhs);
Vec4 homogenize(const Vec4& rhs);
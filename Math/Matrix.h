#pragma once
#include "Vector.h"

struct Mat2;
struct Mat3;
struct Mat4;

struct Mat2 {
public:
	float n[2][2];
	Mat2() : n{{1.0f, 0.0f}, {0.0f, 1.0f}} {} // default to the identity matrix
	Mat2(float a, float b, float c, float d) : n{{a, b}, {c, d}} {}
	Mat2& operator=(const Mat2&) = default;
	~Mat2() = default;
	const Vec2& operator[](int index); // get a row
	const Vec2& operator[](int index) const; // get a row
	const float operator()(int i, int j); // get an element
	const float operator()(int i, int j) const; // get an element
private:
	Mat2& operator+=(const Mat2& rhs); // matrix matrix addition
	friend Mat2 operator+(Mat2 lhs, const Mat2& rhs);
	Mat2& operator-=(const Mat2& rhs); // matrix matrix subtraction
	friend Mat2 operator-(Mat2 lhs, const Mat2& rhs);
	Mat2& operator*=(const Mat2& rhs); // matrix matrix multiplication
	friend Mat2 operator*(Mat2 lhs, const Mat2& rhs);
	friend Vec2 operator*(const Mat2& lhs, const Vec2& rhs); // matrix vector multiplication
	Mat2& operator*=(const float rhs); // matric scalar multiplication
	friend Mat2 operator*(Mat2 lhs, const float rhs);
	friend Mat2 operator*(const float lhs, Mat2 rhs);
};
float det(const Mat2&);
Mat2 transpose(const Mat2&);
Mat2 inverse(const Mat2&);
Mat2 orthogonalize(const Mat2&);

struct Mat3 {
public:
	float n[3][3];
	Mat3() : n{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} {}
	Mat3(float a, float b, float c, float d, float e, float f, float g, float h, float i) : n{ {a, b, c}, {d, e, f}, {g, h, i} } {}
	Mat3& operator=(const Mat3&) = default;
	~Mat3() = default;
	const Vec3& operator[](int index); // get a row
	const Vec3& operator[](int index) const; // get a row
	const float operator()(int i, int j); // get an element
	const float operator()(int i, int j) const; // get an element
private:
	Mat3& operator+=(const Mat3& rhs); // matrix matrix addition
	friend Mat3 operator+(Mat3 lhs, const Mat3& rhs);
	Mat3& operator-=(const Mat3& rhs); // matrix matrix subtraction
	friend Mat3 operator-(Mat3 lhs, const Mat3& rhs);
	Mat3& operator*=(const Mat3& rhs); // matrix matrix multiplication
	friend Mat3 operator*(Mat3 lhs, const Mat3& rhs);
	friend Vec3 operator*(const Mat3& lhs, const Vec3& rhs); // matrix vector multiplication
	Mat3& operator*=(const float rhs); // matric scalar multiplication
	friend Mat3 operator*(Mat3 lhs, const float rhs);
	friend Mat3 operator*(const float lhs, Mat3 rhs);
};
float det(const Mat3&);
Mat3 transpose(const Mat3&);
Mat3 inverse(const Mat3&);
Mat3 orthogonalize(const Mat3&);

struct Mat4 {
public:
	Mat4() : n{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f} } {}
	Mat4(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p) 
		: n{ {a, b, c, d}, {e, f, g, h},  {i, j, k, l}, {m, n, o, p} } {}
	~Mat4() = default;
	const Vec4& operator[](int index); // get a row
	const Vec4& operator[](int index) const; // get a row
	const float operator()(int i, int j); // get an element
	const float operator()(int i, int j) const; // get an element
private:
	float n[4][4];
	Mat4& operator+=(const Mat4& rhs); // matrix matrix addition
	friend Mat4 operator+(Mat4 lhs, const Mat4& rhs);
	Mat4& operator-=(const Mat4& rhs); // matrix matrix subtraction
	friend Mat4 operator-(Mat4 lhs, const Mat4& rhs);
	Mat4& operator*=(const Mat4& rhs); // matrix matrix multiplication
	friend Mat4 operator*(Mat4 lhs, const Mat4& rhs);
	friend Vec4 operator*(const Mat4& lhs, const Vec4& rhs); // matrix vector multiplication
	Mat4& operator*=(const float rhs); // matric scalar multiplication
	friend Mat4 operator*(Mat4 lhs, const float rhs);
	friend Mat4 operator*(const float lhs, Mat4 rhs);
};
float det(const Mat4&);
Mat4 transpose(const Mat4&);
Mat4 inverse(const Mat4&);
Mat4 orthogonalize(const Mat4& m);

struct Transform4 : Mat4 {

private:
	float n[3][4]; // bottom row is always [0, 0, 0, 1]
};
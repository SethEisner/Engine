#pragma once
#include "Vector.h"

struct Mat2;
struct Mat3;
struct Mat4;

struct Mat2 {
public:
	Mat2() : n{{1.0f, 0.0f}, {0.0f, 1.0f}} {} // default to the identity matrix
	Mat2(float a, float b, float c, float d) : n{{a, b}, {c, d}} {}
	Mat2& operator=(const Mat2&) = default;
	~Mat2() = default;
	const Vec2& operator[](int index); // get a row
	const Vec2& operator[](int index) const; // get a row
	const float operator()(int i, int j); // get an element
	const float operator()(int i, int j) const; // get an element
private:
	float n[2][2];
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
Mat2 orthogonalize(const Mat2 m);

struct Mat3 {
public:
	Mat3() : n{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} {}
	Mat3(float a, float b, float c, float d, float e, float f, float g, float h, float i) : n{ {a, b, c}, {d, e, f}, {g, h, i} } {}
	~Mat3() = default;
private:
	float n[3][3];
};

struct Mat4 {
public:
	Mat4() : n{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f} } {}
	Mat4(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l, float m, float n, float o, float p) 
		: n{ {a, b, c, d}, {e, f, g, h},  {i, j, k, l}, {m, n, o, p} } {}
	~Mat4() = default;
private:
	float n[4][4];

};

struct Transform4 : Mat4 {

private:
	float n[3][4]; // bottom row is always [0, 0, 0, 1]
};
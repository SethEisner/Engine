#include "Matrix.h"
//#include <math.h>

Mat2& Mat2::operator+=(const Mat2& rhs) { // matrix matrix addition
	this->n[0][0] += rhs(0, 0);
	this->n[0][1] += rhs(0, 1);
	this->n[1][0] += rhs(1, 0);
	this->n[1][1] += rhs(1, 1);
	return *this;
}
Mat2 operator+(Mat2 lhs, const Mat2& rhs) {
	lhs += rhs;
	return lhs;
}
Mat2& Mat2::operator-=(const Mat2& rhs) { // matrix matrix subtraction
	this->n[0][0] -= rhs(0, 0);
	this->n[0][1] -= rhs(0, 1);
	this->n[1][0] -= rhs(1, 0);
	this->n[1][1] -= rhs(1, 1);
	return *this;
}
Mat2 operator-(Mat2 lhs, const Mat2& rhs) {
	lhs -= rhs;
	return lhs;
}
Mat2& Mat2::operator*=(const Mat2& rhs) { // matrix matrix multiplication
	float a = (*this)(0, 0) * rhs(0, 0) + (*this)(0, 1) * rhs(1, 0);
	float b = (*this)(0, 0) * rhs(0, 1) + (*this)(0, 1) * rhs(1, 1);
	float c = (*this)(1, 0) * rhs(0, 0) + (*this)(1, 1) * rhs(1, 0);
	float d = (*this)(1, 0) * rhs(0, 1) + (*this)(1, 1) * rhs(1, 1);
	this->n[0][0] = a;
	this->n[0][1] = b;
	this->n[1][0] = c;
	this->n[1][1] = d;
	return *this;
}
Mat2 operator*(Mat2 lhs, const Mat2& rhs) {
	lhs *= rhs;
	return lhs;
}
Vec2 operator*(const Mat2& lhs, const Vec2& rhs) {
	return Vec2(lhs(0, 0) * rhs.x + lhs(0, 1) * rhs.y, lhs(1, 0) * rhs.x + lhs(1, 1) * rhs.y);
}
Mat2& Mat2::operator*=(const float rhs) { // matric scalar multiplication
	this->n[0][0] *= rhs;
	this->n[0][1] *= rhs;
	this->n[1][0] *= rhs;
	this->n[1][1] *= rhs;
	return *this;
}
Mat2 operator*(Mat2 lhs, const float rhs) {
	lhs *= rhs;
	return lhs;
}
Mat2 operator*(const float lhs, Mat2 rhs) {
	rhs *= lhs;
	return rhs;
}
const Vec2& Mat2::operator[](int index) { // get a row
	return (*reinterpret_cast<Vec2*>(n[index]));
}
const Vec2& Mat2::operator[](int index) const { // get a row
	return (*reinterpret_cast<const Vec2*>(n[index]));
}
const float Mat2::operator()(int i, int j) { // get an element
	return this->n[i][j];
}
const float Mat2::operator()(int i, int j) const { // get an element from a constant matrix
	return this->n[i][j];
}
float det(const Mat2& m) {
	return (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0));
}
Mat2 transpose(const Mat2& m) {
	return Mat2(m(0, 0), m(1, 0), m(0, 1), m(1, 1));
}
Mat2 inverse(const Mat2& m) {
	return  ((1.0f / det(m)) * Mat2(m(1, 1), -(m(0, 1)), -m(1, 0), m(0, 0)));
}
Mat2 orthogonalize(const Mat2 m) { // gram-schmidt algorithm for 2x2 matrix
	Vec2 u1(m[0]);
	Vec2 u2(rejection(m[1], u1));
	Vec2 e1(normalize(u1));
	Vec2 e2(normalize(u2));
	return Mat2(e1.x, e1.y, e2.x, e2.y);
}
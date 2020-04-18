#include "Matrix.h"
//#include <math.h>

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
float det(const Mat2& m) {
	return (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0));
}
Mat2 transpose(const Mat2& m) {
	return Mat2(m(0, 0), m(1, 0), m(0, 1), m(1, 1));
}
Mat2 inverse(const Mat2& m) {
	return  ((1.0f / det(m)) * Mat2(m(1, 1), -(m(0, 1)), -m(1, 0), m(0, 0)));
}
Mat2 orthogonalize(const Mat2& m) { // gram-schmidt algorithm for 2x2 matrix
	Vec2 u1(m[0]);
	Vec2 u2 = rejection(m[1], u1); //reject m[1] onto u1
	if (dot(u2, u2) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u2 = m[1];
	}
	Vec2 e1(normalize(u1));
	Vec2 e2(normalize(u2));
	return Mat2(e1.x, e1.y, e2.x, e2.y);
}

const Vec3& Mat3::operator[](int index) { // get a row
	return (*reinterpret_cast<const Vec3*>(n[index]));
}
const Vec3& Mat3::operator[](int index) const { // get a row
	return (*reinterpret_cast<const Vec3*>(n[index]));
}
const float Mat3::operator()(int i, int j) { // get an element
	return this->n[i][j];
}
const float Mat3::operator()(int i, int j) const { // get an element
	return this->n[i][j];
}
Mat3& Mat3::operator+=(const Mat3& rhs) { // matrix matrix addition
	this->n[0][0] += rhs(0, 0);
	this->n[0][1] += rhs(0, 1);
	this->n[0][2] += rhs(0, 2);
	this->n[1][0] += rhs(1, 0);
	this->n[1][1] += rhs(1, 1);
	this->n[1][2] += rhs(1, 2);
	this->n[2][0] += rhs(2, 0);
	this->n[2][1] += rhs(2, 1);
	this->n[2][2] += rhs(2, 2);
	return *this;
}
Mat3 operator+(Mat3 lhs, const Mat3& rhs){
	lhs += rhs;
	return lhs;
}
Mat3& Mat3::operator-=(const Mat3& rhs) { // matrix matrix subtraction
	this->n[0][0] -= rhs(0, 0);
	this->n[0][1] -= rhs(0, 1);
	this->n[0][2] -= rhs(0, 2);
	this->n[1][0] -= rhs(1, 0);
	this->n[1][1] -= rhs(1, 1);
	this->n[1][2] -= rhs(1, 2);
	this->n[2][0] -= rhs(2, 0);
	this->n[2][1] -= rhs(2, 1);
	this->n[2][2] -= rhs(2, 2);
	return *this;
}
Mat3 operator-(Mat3 lhs, const Mat3& rhs) {
	lhs -= rhs;
	return lhs;
}
Mat3& Mat3::operator*=(const Mat3& rhs) { // matrix matrix multiplication
	float a = (*this)(0, 0) * rhs(0, 0) + (*this)(0, 1) * rhs(1, 0) + (*this)(0, 2) * rhs(2, 0);
	float b = (*this)(0, 0) * rhs(0, 1) + (*this)(0, 1) * rhs(1, 1) + (*this)(0, 2) * rhs(2, 1);
	float c = (*this)(0, 0) * rhs(0, 2) + (*this)(0, 1) * rhs(1, 2) + (*this)(0, 2) * rhs(2, 2);
	float d = (*this)(1, 0) * rhs(0, 0) + (*this)(1, 1) * rhs(1, 0) + (*this)(1, 2) * rhs(2, 0);
	float e = (*this)(1, 0) * rhs(0, 1) + (*this)(1, 1) * rhs(1, 1) + (*this)(1, 2) * rhs(2, 1);
	float f = (*this)(1, 0) * rhs(0, 2) + (*this)(1, 1) * rhs(1, 2) + (*this)(1, 2) * rhs(2, 2);
	float g = (*this)(2, 0) * rhs(0, 0) + (*this)(2, 1) * rhs(1, 0) + (*this)(2, 2) * rhs(2, 0);
	float h = (*this)(2, 0) * rhs(0, 1) + (*this)(2, 1) * rhs(1, 1) + (*this)(2, 2) * rhs(2, 1);
	float i = (*this)(2, 0) * rhs(0, 2) + (*this)(2, 1) * rhs(1, 2) + (*this)(2, 2) * rhs(2, 2);

	this->n[0][0] = a;
	this->n[0][1] = b;
	this->n[0][2] = c;
	this->n[1][0] = d;
	this->n[1][1] = e;
	this->n[1][2] = f;
	this->n[2][0] = g;
	this->n[2][1] = h;
	this->n[2][2] = i;
	
	return *this;
}
Mat3 operator*(Mat3 lhs, const Mat3& rhs){
	lhs *= rhs;
	return lhs;
}
Vec3 operator*(const Mat3& lhs, const Vec3& rhs) { // matrix vector multiplication
	float a = lhs(0, 0) * rhs.x + lhs(0, 1) * rhs.y + lhs(0, 2) * rhs.z;
	float b = lhs(1, 0) * rhs.x + lhs(1, 1) * rhs.y + lhs(1, 2) * rhs.z;
	float c = lhs(2, 0) * rhs.x + lhs(2, 1) * rhs.y + lhs(2, 2) * rhs.z;
	return Vec3(a,b,c);
}
Mat3& Mat3::operator*=(const float rhs) { // matric scalar multiplication
	this->n[0][0] *= rhs;
	this->n[0][1] *= rhs;
	this->n[0][2] *= rhs;
	this->n[1][0] *= rhs;
	this->n[1][1] *= rhs;
	this->n[1][2] *= rhs;
	this->n[2][0] *= rhs;
	this->n[2][1] *= rhs;
	this->n[2][2] *= rhs;
	return *this;
}
Mat3 operator*(Mat3 lhs, const float rhs){
	lhs *= rhs;
	return lhs;
}
Mat3 operator*(const float lhs, Mat3 rhs){
	rhs *= lhs;
	return rhs;
}
float det(const Mat3& m) {
	return (m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1))) + 
		   (m(0, 1) * (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2))) + 
		   (m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)));
}
Mat3 transpose(const Mat3& m) {
	return Mat3(m(0,0), m(1,0), m(2,0), m(0,1), m(1,1), m(2,1), m(0,2), m(1,2), m(2,2));
}
Mat3 inverse(const Mat3& m) {
	const Vec3& a = m[0];
	const Vec3& b = m[1];
	const Vec3& c = m[2];
	Vec3 r0 = cross(b, c);
	Vec3 r1 = cross(c, a);
	Vec3 r2 = cross(a, b);
	float inv = 1.0f / dot(r2, c); // triple scalar
	return inv * Mat3(r0.x, r1.x, r0.z, r0.y, r1.y, r1.z, r2.x, r2.y, r2.z);
}
Mat3 orthogonalize(const Mat3& m) {
	Vec3 u1 = m[0];
	Vec3 u2 = m[1] - projection(m[1],u1);
	Vec3 u3 = m[2] - projection(m[2], u1) - projection(m[2], u2);
	if (dot(u2, u2) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u2 = m[1];
	}
	if (dot(u3, u3) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u3 = m[2];
	}
	Vec3 e1(normalize(u1));
	Vec3 e2(normalize(u2));
	Vec3 e3(normalize(u3));
	return Mat3(e1.x, e1.y, e1.z, e2.x, e2.y, e2.z, e3.x, e3.y, e3.z);
}


const Vec4& Mat4::operator[](int index) { // get a row
	return (*reinterpret_cast<const Vec4*>(n[index]));
}
const Vec4& Mat4::operator[](int index) const { // get a row
	return (*reinterpret_cast<const Vec4*>(n[index]));
}
const float Mat4::operator()(int i, int j) { // get an element
	return this->n[i][j];
}
const float Mat4::operator()(int i, int j) const { // get an element
	return this->n[i][j];
}
Mat4& Mat4::operator+=(const Mat4& rhs) { // matrix matrix addition
	this->n[0][0] += rhs(0, 0);
	this->n[0][1] += rhs(0, 1);
	this->n[0][2] += rhs(0, 2);
	this->n[0][3] += rhs(0, 3);
	this->n[1][0] += rhs(1, 0);
	this->n[1][1] += rhs(1, 1);
	this->n[1][2] += rhs(1, 2);
	this->n[1][3] += rhs(1, 3);
	this->n[2][0] += rhs(2, 0);
	this->n[2][1] += rhs(2, 1);
	this->n[2][2] += rhs(2, 2);
	this->n[2][3] += rhs(2, 3);
	this->n[3][0] += rhs(3, 0);
	this->n[3][1] += rhs(3, 1);
	this->n[3][2] += rhs(3, 2);
	this->n[3][3] += rhs(3, 3);
	return *this;
}
Mat4 operator+(Mat4 lhs, const Mat4& rhs) {
	lhs += rhs;
	return lhs;
}
Mat4& Mat4::operator-=(const Mat4& rhs) { // matrix matrix subtraction
	this->n[0][0] -= rhs(0, 0);
	this->n[0][1] -= rhs(0, 1);
	this->n[0][2] -= rhs(0, 2);
	this->n[0][3] -= rhs(0, 3);
	this->n[1][0] -= rhs(1, 0);
	this->n[1][1] -= rhs(1, 1);
	this->n[1][2] -= rhs(1, 2);
	this->n[1][3] -= rhs(1, 3);
	this->n[2][0] -= rhs(2, 0);
	this->n[2][1] -= rhs(2, 1);
	this->n[2][2] -= rhs(2, 2);
	this->n[2][3] -= rhs(2, 3);
	this->n[3][0] -= rhs(3, 0);
	this->n[3][1] -= rhs(3, 1);
	this->n[3][2] -= rhs(3, 2);
	this->n[3][3] -= rhs(3, 3);
	return *this;
}
Mat4 operator-(Mat4 lhs, const Mat4& rhs) {
	lhs -= rhs;
	return lhs;
}
Mat4& Mat4::operator*=(const Mat4& rhs) { // matrix matrix multiplication
	//float a = (*this)(0,0)*rhs(0,0) + (*this)(0,1)*rhs(1,0)

	float a = (*this)(0, 0) * rhs(0, 0) + (*this)(0, 1) * rhs(1, 0) + (*this)(0, 2) * rhs(2, 0) + (*this)(0, 3) * rhs(3, 0);
	float b = (*this)(0, 0) * rhs(0, 1) + (*this)(0, 1) * rhs(1, 1) + (*this)(0, 2) * rhs(2, 1) + (*this)(0, 3) * rhs(3, 1);
	float c = (*this)(0, 0) * rhs(0, 2) + (*this)(0, 1) * rhs(1, 2) + (*this)(0, 2) * rhs(2, 2) + (*this)(0, 3) * rhs(3, 2);
	float d = (*this)(0, 0) * rhs(0, 3) + (*this)(0, 1) * rhs(1, 3) + (*this)(0, 2) * rhs(2, 3) + (*this)(0, 3) * rhs(3, 3);

	float e = (*this)(1, 0) * rhs(0, 0) + (*this)(1, 1) * rhs(1, 0) + (*this)(1, 2) * rhs(2, 0) + (*this)(1, 3) * rhs(3, 0);
	float f = (*this)(1, 0) * rhs(0, 1) + (*this)(1, 1) * rhs(1, 1) + (*this)(1, 2) * rhs(2, 1) + (*this)(1, 3) * rhs(3, 1);
	float g = (*this)(1, 0) * rhs(0, 2) + (*this)(1, 1) * rhs(1, 2) + (*this)(1, 2) * rhs(2, 2) + (*this)(1, 3) * rhs(3, 2);
	float h = (*this)(1, 0) * rhs(0, 3) + (*this)(1, 1) * rhs(1, 3) + (*this)(1, 2) * rhs(2, 3) + (*this)(1, 3) * rhs(3, 3);

	float i = (*this)(2, 0) * rhs(0, 0) + (*this)(2, 1) * rhs(1, 0) + (*this)(2, 2) * rhs(2, 0) + (*this)(2, 3) * rhs(3, 0);
	float j = (*this)(2, 0) * rhs(0, 1) + (*this)(2, 1) * rhs(1, 1) + (*this)(2, 2) * rhs(2, 1) + (*this)(2, 3) * rhs(3, 1);
	float k = (*this)(2, 0) * rhs(0, 2) + (*this)(2, 1) * rhs(1, 2) + (*this)(2, 2) * rhs(2, 2) + (*this)(2, 3) * rhs(3, 2);
	float l = (*this)(2, 0) * rhs(0, 3) + (*this)(2, 1) * rhs(1, 3) + (*this)(2, 2) * rhs(2, 3) + (*this)(2, 3) * rhs(3, 3);

	float m = (*this)(3, 0) * rhs(0, 0) + (*this)(3, 1) * rhs(1, 0) + (*this)(3, 2) * rhs(2, 0) + (*this)(3, 3) * rhs(3, 0);
	float n = (*this)(3, 0) * rhs(0, 1) + (*this)(3, 1) * rhs(1, 1) + (*this)(3, 2) * rhs(2, 1) + (*this)(3, 3) * rhs(3, 1);
	float o = (*this)(3, 0) * rhs(0, 2) + (*this)(3, 1) * rhs(1, 2) + (*this)(3, 2) * rhs(2, 2) + (*this)(3, 3) * rhs(3, 2);
	float p = (*this)(3, 0) * rhs(0, 3) + (*this)(3, 1) * rhs(1, 3) + (*this)(3, 2) * rhs(2, 3) + (*this)(3, 3) * rhs(3, 3);

	this->n[0][0] = a;
	this->n[0][1] = b;
	this->n[0][2] = c;
	this->n[0][3] = d;
	this->n[1][0] = e;
	this->n[1][1] = f;
	this->n[1][2] = g;
	this->n[1][3] = h;
	this->n[2][0] = i;
	this->n[2][1] = j;
	this->n[2][2] = k;
	this->n[2][3] = l;
	this->n[3][0] = m;
	this->n[3][1] = n;
	this->n[3][2] = o;
	this->n[3][3] = p;

	return *this;
}
Mat4 operator*(Mat4 lhs, const Mat4& rhs) {
	lhs *= rhs;
	return lhs;
}
Vec4 operator*(const Mat4& lhs, const Vec4& rhs) { // matrix vector multiplication
	return Vec4((lhs(0, 0) * rhs.x + lhs(0, 1) * rhs.y + lhs(0, 2) * rhs.z + lhs(0, 3) * rhs.w),
				(lhs(1, 0) * rhs.x + lhs(1, 1) * rhs.y + lhs(1, 2) * rhs.z + lhs(1, 3) * rhs.w),
				(lhs(2, 0) * rhs.x + lhs(2, 1) * rhs.y + lhs(2, 2) * rhs.z + lhs(2, 3) * rhs.w),
				(lhs(3, 0) * rhs.x + lhs(3, 1) * rhs.y + lhs(3, 2) * rhs.z + lhs(3, 3) * rhs.w));
}
Mat4& Mat4::operator*=(const float rhs) { // matric scalar multiplication
	this->n[0][0] *= rhs;
	this->n[0][1] *= rhs;
	this->n[0][2] *= rhs;
	this->n[0][3] *= rhs;
	this->n[1][0] *= rhs;
	this->n[1][1] *= rhs;
	this->n[1][2] *= rhs;
	this->n[1][3] *= rhs;
	this->n[2][0] *= rhs;
	this->n[2][1] *= rhs;
	this->n[2][2] *= rhs;
	this->n[2][3] *= rhs;
	this->n[3][0] *= rhs;
	this->n[3][1] *= rhs;
	this->n[3][2] *= rhs;
	this->n[3][3] *= rhs;
	return *this;
}
Mat4 operator*(Mat4 lhs, const float rhs) {
	lhs *= rhs;
	return lhs;
}
Mat4 operator*(const float lhs, Mat4 rhs) {
	rhs *= lhs;
	return rhs;
}
float det(const Mat4&) {
	return 0.0f;
}
Mat4 transpose(const Mat4&) {
	return Mat4();
}
Mat4 inverse(const Mat4&) {
	return Mat4();
}
Mat4 orthogonalize(const Mat4& m) {
	Vec4 u1 = m[0];
	Vec4 u2 = m[1] - projection(m[1], u1);
	Vec4 u3 = m[2] - projection(m[2], u1) - projection(m[2], u2);
	Vec4 u4 = m[3] - projection(m[3], u1) - projection(m[3], u2) - projection(m[3], u3);
	float t = dot(u3, u3);
	float v = dot(u4, u4);
	if (dot(u2, u2) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u2 = m[1];
	}
	if (dot(u3, u3) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u3 = m[2];
	}
	if (dot(u4, u4) < EPSILON) { // row is linearly independent so we can skip it by setting it back to the matrix's row
		u4 = m[3];
	}
	Vec4 e1(normalize(u1));
	Vec4 e2(normalize(u2));
	Vec4 e3(normalize(u3));
	Vec4 e4(normalize(u4));

	return Mat4(e1.x, e1.y, e1.z, e1.w, 
				e2.x, e2.y, e2.z, e2.w, 
				e3.x, e3.y, e3.z, e3.w, 
				e4.x, e4.y, e4.z, e4.w);
}
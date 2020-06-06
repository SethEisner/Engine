#pragma once

#include <windows.h>
#include <DirectXMath.h>
#include <cstdint>

class MathHelper {
public:
	static float randf() {
		return static_cast<float>(::rand()) / static_cast<float>(RAND_MAX);
	}
	static float randf(float a, float b) {
		return a + randf() * (b - a);
	}
	static int rand(int a, int b) {
		return a + ::rand() % ((b - a) + 1);
	}
	
#undef min
	template <typename T>
	static T min(const T& a, const T& b){
		return a < b ? a : b;
	}
#undef max
	template <typename T>
	static T max(const T& a, const T& b) {
		return a > b ? a : b;
	}
	template <typename T>
	static T lerp(const T& a, const T& b) {
		return a + (b - a) * t;
	}
	template <typename T>
	static T clamp(const T& x, const T& low, const T& high) {
		return x < low ? low : (x > high ? high : x);
	}
	static float angle_from_xy(float x, float y);
	static DirectX::XMVECTOR spherical_to_cartesian(float radius, float theta, float phi) {
		return DirectX::XMVectorSet(radius*sinf(phi) * cosf(theta), 
									radius * cosf(phi), 
									radius * sinf(phi) * sinf(theta), 
									1.0f );
	}
	static DirectX::XMFLOAT4X4 identity_4x4() {
		static DirectX::XMFLOAT4X4 id(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
			return id;
	}
	static DirectX::XMVECTOR rand_unit_vec3();
	static DirectX::XMVECTOR rand_hemipshere_unit_vec3(DirectX::XMVECTOR);
	static const float INFINITY_;
	static const float PI;

};
#include "Utilities.h"
#include <assert.h>
#include <Windows.h>
#include <DirectXMath.h>
float fast_invsqrt(float x) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = x * 0.5F;
	y = x;
	i = *(long*)&y;                       // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);               // what the fuck? 
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));   // 1st iteration
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
	return y;
}

float degrees(float rads) { // convert radians to degrees;
	return rads * deg_per_rad;
}
float radians(float degrees) { // convert degrees to radians;
	return degrees * rad_per_deg;
}

void print_matrix(const DirectX::XMFLOAT4X4& mat) {
	OutputDebugStringA(((std::to_string(mat._11) + ", " + std::to_string(mat._12) + ", " + std::to_string(mat._13) + ", " + std::to_string(mat._14) + "\n") +
	(std::to_string(mat._21) + ", " + std::to_string(mat._22) + ", " + std::to_string(mat._23) + ", " + std::to_string(mat._24) + "\n") + 
	(std::to_string(mat._31) + ", " + std::to_string(mat._32) + ", " + std::to_string(mat._33) + ", " + std::to_string(mat._34) + "\n") +
	(std::to_string(mat._41) + ", " + std::to_string(mat._42) + ", " + std::to_string(mat._43) + ", " + std::to_string(mat._44) + "\n\n")).c_str());
}
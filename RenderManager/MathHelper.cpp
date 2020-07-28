#include "MathHelper.h"
#include <float.h>
#include <cmath>

const float MathHelper::INFINITY_ = FLT_MAX;
const float MathHelper::PI = 3.1415926535f;

float MathHelper::angle_from_xy(float x, float y) {
	float theta = 0.0f;
	if (x >= 0.0f) {
		theta = atanf(y / x);
		if (theta < 0.0f) theta += 2.0f * PI;
	}
	else {
		theta = atanf(y / x) + PI;
	}
	return theta;
}
DirectX::XMVECTOR MathHelper::rand_unit_vec3() {
	using namespace DirectX;
	XMVECTOR one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR zero = XMVectorZero();
	while (true) {
		XMVECTOR v = XMVectorSet(MathHelper::randf(-1.0f, 1.0f), MathHelper::randf(-1.0f, 1.0f), MathHelper::randf(-1.0f, 1.0f), 0.0f);
		// ignore the point if it is ouside the unit sphere
		if (XMVector3Greater(XMVector3LengthSq(v), one)) continue;
		return XMVector3Normalize(v);
	}
}
DirectX::XMVECTOR MathHelper::rand_hemipshere_unit_vec3(DirectX::XMVECTOR n) {
	using namespace DirectX;
	XMVECTOR one = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR zero = XMVectorZero();
	while (true) {
		XMVECTOR v = XMVectorSet(MathHelper::randf(-1.0f, 1.0f), MathHelper::randf(-1.0f, 1.0f), MathHelper::randf(-1.0f, 1.0f), 0.0f);
		if (XMVector3Greater(XMVector3LengthSq(v), one)) continue; // ignore the point if it outside the unit sphere
		if (XMVector3Less(XMVector3Dot(n, v), zero)) continue; // ignmore the point if it is in the negative hemisphere
		return XMVector3Normalize(v);
	}
}
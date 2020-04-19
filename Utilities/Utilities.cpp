#include "Utilities.h"

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
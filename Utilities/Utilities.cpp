#include "Utilities.h"
#include <assert.h>

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

// inline void assert_if_failed(HRESULT hr) {
// 	assert(!FAILED(hr));
// }
DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number) :
	error_code(hr),
	function_name(function_name),
	filename(filename),
	line_number(line_number)
{
}
std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(error_code);
	std::wstring msg = err.ErrorMessage();

	return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
}
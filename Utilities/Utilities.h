#pragma once
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <comdef.h>
#include <string>
#include <D3Dcommon.h>
#include <Windows.h>
#include <wrl/client.h>
#include <D3DCompiler.h>
#include <d3d12.h>
//#include <stringutils.h>
/*TODO: 
add random number generator
add faster trigonometric functions (may not be necessary with /fp:fast)
*/

float fast_invsqrt(float);
static constexpr float pi = 3.14159256359;
static constexpr float rad_per_deg = 0.01745329252;
static constexpr float deg_per_rad = 57.2957795131;
float degrees(float rads); // convert radians to degrees;
float radians(float degrees); // convert degrees to radians;

// string hashing macro
#define H1(s,i,x)   (x*65599u+(uint8_t)s[(i)<strlen(s)?strlen(s)-1-(i):strlen(s)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))
#define H256(s,i,x) H64(s,i,H64(s,i+64,H64(s,i+128,H64(s,i+192,x))))
#define HASH(s)    ((uint32_t)(H256(s,0,0)^(H256(s,0,0)>>16)))

// void assert_if_failed(HRESULT);
inline void assert_if_failed(HRESULT hr) {
// #if defined(_DEBUG)
// 	assert(!FAILED(hr));
// #endif
	if (FAILED(hr)) {
		_com_error err(hr);
		const std::wstring msg = err.ErrorMessage();
		const std::wstring outputMsg = L" error: " + msg; 
		MessageBox(0, outputMsg.c_str(), 0, 0);
		assert(false);
	}
}
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number);
	std::wstring ToString() const;
	HRESULT error_code = S_OK;
	std::wstring function_name;
	std::wstring filename;
	int line_number = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);          \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const std::wstring&, const D3D_SHADER_MACRO*, const std::string&, const std::string&);
Microsoft::WRL::ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device*, ID3D12GraphicsCommandList*, const void*, uint64_t, Microsoft::WRL::ComPtr<ID3D12Resource>&);
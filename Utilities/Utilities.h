#pragma once
#include <string.h>
#include <stdint.h>
#include <Windows.h>
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

inline void assert_if_failed(HRESULT);
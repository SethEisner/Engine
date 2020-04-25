#include "Globals.h"

LinearAllocator linear_allocator{ 1024 * 1024 };


// void* operator new (size_t size) {
// 	void* p = linear_allocator.allocate_aligned(size, alignof(size));
// 	return p;
// }
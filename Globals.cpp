#include "Globals.h"

LinearAllocator linear_allocator(1024 * 1024);
StackAllocator stack_allocator(1024);
//							 (size, count)
/*PoolAllocator pool4_allocator(4, 1024); // pool allocator with size of 4 bytes needs to use offsets instead of addresses
PoolAllocator pool8_allocator(8, 1024);
PoolAllocator pool16_allocator(16, 1024);
PoolAllocator pool32_allocator(32, 1024);
PoolAllocator pool64_allocator(64, 1024);
PoolAllocator pool128_allocator(128, 1024);
PoolAllocator pool256_allocator(256, 1024);
*/
//StackAllocator bottom_stack_allocator = reinterpret_cast<StackAllocator>(BottomStackAllocator(1024));
//LinearAllocator stack_allocator(1024);
//PoolAllocator pool_allocator_array[0]{}
// void* operator new (size_t size) {
// 	void* p = linear_allocator.allocate_aligned(size, alignof(size));
// 	return p;
// }
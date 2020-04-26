#pragma once
#include "Memory/LinearAllocator.h"
#include "Memory/StackAllocator.h"

extern LinearAllocator linear_allocator;// (1024);
extern StackAllocator stack_allocator;
/*
// making the user specify which allocator we use makes the free function a lot easier
extern PoolAllocator pool4_allocator; // pool allocator with size of 4 bytes needs to use offsets instead of addresses
extern PoolAllocator pool8_allocator;
extern PoolAllocator pool16_allocator;
extern PoolAllocator pool32_allocator;
extern PoolAllocator pool64_allocator;
extern PoolAllocator pool128_allocator;
extern PoolAllocator pool256_allocator;
*/
// 4, 8, 16, 32, 64, 128, 256
//PoolAllocator pool_allocator_array[7];

// void* operator new (size_t size);
template <class Allocator>
void* operator new (size_t bytes, size_t alignment, Allocator& allocator) {
	return allocator.allocate(bytes, alignment);
}
//template <class Allocator>
void* operator new (size_t bytes, size_t alignment, PoolAllocator& _pool_allocator) {
	_pool_allocator.allocate();
}
// allocate count number of contiguous objects aligned to the given alignment 
template <class Allocator>
void* operator new (size_t bytes, size_t count, size_t alignment, Allocator& allocator) {
	return allocator.allocate(bytes * count, alignment);
}
// need to ensure that what we allocate will fit in the pool
void* operator new (size_t bytes, size_t count, size_t alignment, PoolAllocator& _pool_allocator) {
	// assert that the total size we want to allocate will fit within the pool
	assert(bytes * count <= _pool_allocator.m_pool_size);
	return _pool_allocator.allocate();
}
template <class Allocator>
void free(void* ptr, Allocator& allocator) {
	allocator.free(ptr);
}



#define NEW(type, allocator) new(static_cast<size_t>(alignof(type)), allocator) type
#define NEW_ARRAY(type, count, allocator) new(static_cast<size_t>(count), static_cast<size_t>(alignof(type)), allocator) type
#define FREE(object, allocator) free(object, allocator)
// wont need FREE_ARRAY until we create a general allocator
//#define FREE_ARRAY(object, count, allocator) free(object, count, allocator)

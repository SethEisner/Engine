#pragma once
#include "Memory/LinearAllocator.h"

extern LinearAllocator linear_allocator;// (1024);

// void* operator new (size_t size);
template <class Allocator>
void* operator new (size_t bytes, size_t alignment, Allocator& allocator) {
	return allocator.allocate_aligned(bytes, alignment);
}
template <class Allocator>
void* operator new (size_t bytes, size_t count, size_t alignment, Allocator& allocator) {
	return allocator.allocate_aligned(bytes * count, alignment);
}
// template <typename T, class Allocator>
// void _delete(T* ptr, Allocator& allocator) {
// 	ptr->~T();
// 	allocator.free(ptr);
// }
#define NEW(type, allocator) new (static_cast<size_t>(alignof(type)), allocator) type
#define NEW_ARRAY(type, count, allocator) new (static_cast<size_t>(count), static_cast<size_t>(alignof(type)), allocator) type

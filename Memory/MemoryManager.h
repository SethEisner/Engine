#pragma once
#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "GeneralAllocator.h"


// typedef size_t Handle; defined in general allocator

class MemoryManager {
public:
	MemoryManager();
	~MemoryManager();
	LinearAllocator* get_linear_allocator();
	StackAllocator* get_stack_allocator();
	PoolAllocator* get_pool_allocator(size_t size);
	GeneralAllocator* get_general_allocator();
	void free(Handle);
	void defragment();
private:
	LinearAllocator* m_linear;
	StackAllocator* m_stack;
	PoolAllocator* m_pool4;
	PoolAllocator* m_pool8;
	PoolAllocator* m_pool16;
	PoolAllocator* m_pool32;
	PoolAllocator* m_pool64;
	PoolAllocator* m_pool128;
	PoolAllocator* m_pool256;
	GeneralAllocator* m_general;
};

extern MemoryManager* memory_manager;

void* operator new (size_t bytes, size_t alignment, LinearAllocator* allocator);
void* operator new (size_t bytes, size_t alignment, StackAllocator* allocator);
void* operator new (size_t bytes, size_t alignment, PoolAllocator* _pool_allocator);
void* operator new (size_t bytes, size_t alignment, GeneralAllocator* allocator);

void operator delete(void* ptr) noexcept;

template <class Allocator>
void free(void* ptr, Allocator* allocator) {
	allocator->free(ptr);
}
#define COMMA , // allows passing templated types to the NEW* macros
#define NEW(type, allocator) new(static_cast<size_t>(alignof(type)), allocator) type // calls placement new
template<typename T, typename Allocator>
static inline T* new_array_helper(size_t count, size_t alignment, Allocator* allocator) {
	T* ptr = reinterpret_cast<T*>(allocator->allocate(sizeof(T) * count, alignment));
	const T* const end = ptr + count;
	while (ptr != end) new (ptr++) T; // call placement new on each element
	return ptr - count;
}
template <typename T>
static inline T* new_array(size_t count, size_t alignment, LinearAllocator* allocator) {
	return new_array_helper<T>(count, alignment, allocator);
}
template <typename T>
static inline T* new_array(size_t count, size_t alignment, StackAllocator* allocator) {
	return new_array_helper<T>(count, alignment, allocator);
}
template <typename T>
static inline Handle new_array(size_t count, size_t alignment, GeneralAllocator* allocator) {
	return allocator->register_allocated_mem(new_array_helper<T>(count, alignment, allocator)); // return the handle to the data we allocated
}
#define NEW_ARRAY(type, count, allocator) new_array<type>(static_cast<size_t>(count), static_cast<size_t>(alignof(type)), allocator)
#define FREE(object, allocator) free(object, allocator)

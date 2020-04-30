#pragma once
#include "LinearAllocator.h"
#include "StackAllocator.h"
#include "PoolAllocator.h"
#include "GeneralAllocator.h"


typedef const size_t Handle;

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
void* operator new(size_t bytes, size_t alignment, PoolAllocator* _pool_allocator);
void* operator new (size_t bytes, size_t alignment, GeneralAllocator* allocator);
// allocate count number of contiguous objects aligned to the given alignment 
void* operator new (size_t bytes, size_t count, size_t alignment, LinearAllocator* allocator);
void* operator new (size_t bytes, size_t count, size_t alignment, StackAllocator* allocator);
void* operator new (size_t bytes, size_t count, size_t alignment, GeneralAllocator* allocator);

void operator delete(void* ptr) noexcept; // dont do anything here. if we throw while making memory then the engine wont ever run anyway

template <class Allocator>
void free(void* ptr, Allocator* allocator) {
	allocator->free(ptr);
}

#define NEW(type, allocator) new(static_cast<size_t>(alignof(type)), allocator) type
#define NEW_ARRAY(type, count, allocator) new(static_cast<size_t>(count), static_cast<size_t>(alignof(type)), allocator) type
#define FREE(object, allocator) free(object, allocator)

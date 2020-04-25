#pragma once
#include <stdint.h> 
#include <stdlib.h> // for malloc
#include <assert.h>
#include <mutex>
#include "../Globals.h"

inline uintptr_t align_address(uintptr_t address, size_t alignment) {
	const size_t mask = alignment - 1;
	assert((alignment & mask) == 0); // alignment must be a power of two
	return (address + alignment) & ~mask; // negated mask has all 1s in upper part and all 0s in lower part.
}

template <typename T>
static inline T* align_pointer(T* p_mem, size_t alignment) {
	const uintptr_t address = reinterpret_cast<uintptr_t>(p_mem);
	const uintptr_t aligned_address = align_address(address, alignment);
	return reinterpret_cast<T*>(aligned_address);
}

void* operator new (size_t size, size_t alignment) {
	void* p = linear_allocator.allocate_aligned(size, alignment);
}

// allocator should manage it's own memory
class LinearAllocator {
public:
	explicit LinearAllocator(size_t size) { // size is number of bytes to allocate
		m_begin = reinterpret_cast<uint8_t*>(malloc(size));
		m_end = static_cast<uint8_t*>(m_begin + size + 1); // point to one past the end to be consistent with c++ iterator behavior
		m_current = m_begin; // make m_current point to what m_begin points to
	}
	// make it so we can never assign allocators
	LinearAllocator& operator=(LinearAllocator) = delete;
	LinearAllocator& operator=(LinearAllocator&) = delete;
	LinearAllocator& operator=(LinearAllocator&&) = delete;
	~LinearAllocator() {
		free(static_cast<void*>(m_begin));
	}
	// if we enable the allocation of unaligned memory, how will we know how to free it?
	void* allocate_aligned(size_t size, size_t alignment) { // allocate aligned memory
		// allocate more bytes than we need to give us a range we can shift within
		size_t total_bytes = size + alignment;
		assert(m_current + total_bytes < m_end);
		// start of critical section
		m_lock.lock();
		uint8_t* p_mem = m_current;
		m_current += total_bytes;
		m_lock.unlock();

		uint8_t* p_aligned_mem = align_pointer(p_mem, alignment);
		// align the pointer to be in the second half of the allocated memory if it is already aligned. use the first half to store the alignment info for freeing
		if (p_aligned_mem == p_mem) p_aligned_mem += alignment;
		ptrdiff_t shift = p_aligned_mem - p_mem;
		assert(shift > 0 && shift <= 256); // shift cannot be greater than 256 because the minimum alignment space is 1 byte
		p_aligned_mem[-1] = static_cast<uint8_t>(shift & 0xFF);
		return p_aligned_mem;
	}
	//void* allocate(size_t size, size_t alligment, size_t offset); // probably dont want to handle an offset for now
	inline void free(void* p) {} // does nothing
	inline void reset() { // resets the pointers to initial state to effectively clear the memory
		m_lock.lock();
		m_current = m_begin;
		m_lock.unlock();
	}
private:
	std::mutex m_lock; // could easily make it lock free, but this will have basically no overhead
	uint8_t* m_begin;
	uint8_t* m_end; // points to one past the end
	uint8_t* m_current;
}

// void* operator new(size_t size, size_t alignment) {
// 	void* p = allocate_aligned(size, alignment);
// }
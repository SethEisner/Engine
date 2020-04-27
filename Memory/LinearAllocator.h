#pragma once
#include <stdint.h> 
#include <stdlib.h> // for malloc
#include <assert.h>
#include <mutex>
#include "memory.h"

class LinearAllocator {
	typedef uint8_t byte;
public:
	explicit LinearAllocator(size_t size) { // size is number of bytes to allocate
		m_begin = reinterpret_cast<byte*>(malloc(size));
		assert(m_begin != nullptr);
		m_end = static_cast<byte*>(m_begin + size); // point to one past the end to be consistent with c++ iterator behavior
		m_current = m_begin; // make m_current point to what m_begin points to
	}
	// make it so we can never assign allocators
	LinearAllocator& operator=(LinearAllocator) = delete;
	LinearAllocator& operator=(LinearAllocator&) = delete;
	LinearAllocator& operator=(LinearAllocator&&) = delete;
	~LinearAllocator() {
		std::free(m_begin);
	}
	// if we enable the allocation of unaligned memory, how will we know how to free it?
	void* allocate(size_t size, size_t alignment) { // allocates aligned memory
		// allocate more bytes than we need to give us a range we can shift within
		size_t total_bytes = size + alignment;
		assert(m_current + total_bytes < m_end);
		// start of critical section
		m_lock.lock();
		byte* p_mem = m_current;
		m_current += total_bytes;
		m_lock.unlock();
		return align(p_mem, alignment);
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
	byte* m_begin;
	byte* m_end; // points to one past the end
	byte* m_current;
};
#pragma once
#include <assert.h>
#include "memory.h"
#include <mutex>
#include <stdint.h>
#include <stdlib.h>



class StackAllocator { // double ended stack allocator
	typedef uint8_t byte;
public:
	explicit StackAllocator(size_t size) { // size is number of bytes to allocate
		//m_stack = malloc(size);
		m_begin = reinterpret_cast<byte*>(malloc(size));
		assert(m_begin != nullptr);
		m_end = static_cast<byte*>(m_begin + size); // point to one past the end to be consistent with c++ iterator behavior
		m_current = m_begin; // make m_current point to what m_begin points to
	}
	// make it so we can never assign allocators
	StackAllocator& operator=(StackAllocator) = delete;
	StackAllocator& operator=(StackAllocator&) = delete;
	StackAllocator& operator=(StackAllocator&&) = delete;
	~StackAllocator() {
		std::free(m_begin); // call global free when we destroy the stack
	}
	//virtual void* allocate(size_t, size_t) = 0; // make the allocate function be a pure virtual so we always go the child's appropriate
	void* allocate(size_t size, size_t alignment) {
		// allocate more bytes than we need to give us a range we can shift within
		size_t total_bytes = size + alignment;
		// assert that the bottom stack will not overlap the top stack from this allocation
		assert(m_current + total_bytes < m_end);
		// start of critical section
		m_lock.lock();
		byte* p_mem = m_current;
		m_current += total_bytes;
		m_lock.unlock();
		return align(p_mem, alignment);
	}
	void free(void* addr) {
		assert(addr && addr < m_current);
		byte* p_aligned_mem = reinterpret_cast<byte*>(addr);
		ptrdiff_t shift = p_aligned_mem[-1];
		if (shift == 0) shift = 256;
		byte* p_mem = p_aligned_mem - shift;
		m_lock.lock();
		m_current = p_mem;
		m_lock.unlock();
	}
	inline void reset() { // resets the pointers to initial state to effectively clear the memory
		m_lock.lock();
		m_current = m_begin;
		m_lock.unlock();
	}
protected:
	std::mutex m_lock;
	//void* m_stack;
	byte* m_begin; // begin of bottom stack
	byte* m_end; // begin of the top of the stack (grows downwards) 
	byte* m_current;
};

// void* allocate_top(size_t size, size_t alignment) { // return the pointer to the lower side so we can iterate normally
// 	size_t total_bytes = size + alignment;
// 	// assert that the top will not overlap the bottom stack from this allocation
// 	assert(m_current_top - total_bytes > m_current);
// 	m_lock.lock();
// 	// make m_top_prev store the address of m_current_top
// 	m_top_previous = m_current_top;
// 	byte* p_mem = m_current_top - total_bytes;
// 	m_current_top -= (total_bytes);
// 	m_lock.unlock();
// 	assert(m_current < m_current_top); // need to assert afterwards because another thother thread could have updated the bottom pointer while we slept
// 	return align(p_mem, alignment);
// }
#pragma once
#include <assert.h>
#include <stdint.h>
#include <malloc.h>
#include <mutex>

class PoolAllocator {
	typedef uint8_t byte;
public:
	// align to the given size
	PoolAllocator(size_t size, size_t count) : m_mem(malloc(size * (count + 1))), m_pool(nullptr), m_chunk_size(size) {
		// allocate 1 more element than we need and use that to align the pointer
		assert(m_mem != nullptr);
		m_pool = align_pointer(m_mem, size); // align the pointer directly, dont need to worry about storing the shift amount because we store the original pointer for freeing
		m_free = static_cast<uint8_t*>(m_pool);
		assert(size >= 2); //
		assert(count <= 1u << 16); // make sure the max possible offset will fit in our minimum size
		uint16_t offset = static_cast<uint16_t>(size);
		for (size_t i = 0; i < count * size; i += size) {
			set_offset(offset, m_free + i);
		}
	}
	~PoolAllocator() { 
		std::free(m_mem);
	}
	inline void* allocate() {
		// get the offset of t
		m_lock.lock();
		void* old_free = m_free;
		int16_t offset = get_offset(m_free);
		m_free += offset; // store the address of the next free pool item
		m_lock.unlock();
		return old_free;
	}
	void free(void* ptr) { // add the given chunk to the front of the freelist, and set it's offset accordingly
		assert(ptr != nullptr); // assert that we are not freeing a null ptr
		m_lock.lock();
		byte* old_free = m_free;
		byte* current = static_cast<byte*>(ptr);
		int16_t offset = static_cast<int16_t>(old_free - current);
		set_offset(offset, current);
		m_free = current;
		int16_t temp = get_offset(m_free);
		m_lock.unlock();
		return;
	}
	inline size_t get_chunk_size() {
		return m_chunk_size;
	}
//private:
	std::mutex m_lock;
	void* m_mem;
	void* m_pool; // dont edit this copy fo the pointer because we need it to free
	byte* m_free; // pointer to the freelist
	const size_t m_chunk_size;
	int16_t get_offset(byte* ptr) {
		// litte endian
		uint8_t lower = *(ptr);
		uint16_t upper = *(ptr + 1) << 8;
		// big endian
		// uint16_t upper = *(ptr) << 8;
		// uint16_t lower = *(ptr + 1);
		return upper + lower;
	}
	inline void set_offset(uint16_t offset, byte* ptr) { // set the offset value at the ptr
		// endian independent
		byte upper = (offset >> 8) & 0xFF;
		byte lower = offset & 0xFF;
		// little endian
		*(ptr) = lower;     // store upper byte of offset
		*(ptr + 1) = upper; // store lower byte of offset in next byte
		// big endian
		// *(ptr) = upper;     // store upper byte of offset
		// *(ptr + 1) = lower; // store lower byte of offset in next byte
	}
};


#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "memory.h"
#include <mutex>

class GeneralAllocator {
	typedef uint8_t byte;
	typedef size_t Handle;
	struct FreeBlockInfo {
		byte* m_next_free; // stores the address of the next free block
		uint32_t m_free_size; // stores the size of this free block in bytes;
		FreeBlockInfo(byte* _next, uint32_t _size) : m_next_free(_next), m_free_size(_size) {};
	};
public:
	GeneralAllocator() = delete;
	// malloc already aligns to 8 bytes so memory isnt awful
	GeneralAllocator(size_t size) : m_begin(static_cast<byte*>(malloc(size))), m_last_handle(0) {
		assert(size < 0xFFFFFFFF); // assert that size is 4 bytes
		m_current = static_cast<byte*>(m_begin);
		m_end = static_cast<byte*>(m_begin) + size;
		for (size_t index = 0; index != m_handle_table_size; index++) {
			m_handle_table[index] = nullptr; // store nullpointers in the table
		}
		FreeBlockInfo free_block(m_end, static_cast<uint32_t>(size));
		fill_free_block(m_current, free_block);
	}
	~GeneralAllocator() {
		delete m_begin;
	};
	Handle allocate(size_t size, size_t alignment) {
		// allocate aligned memory from the memory pool and set the handle
		// allocate 4 extra bytes -> first 4 is for number of bytes in this allocation, last 1 byte is for how much we needed to shift to get the allocation correct
		// in the raw memory, we need to write where the next free segment is and how big this free segment is, need both numbers because we cannot get one from the other
	}
	void Free(Handle handle) {
		void* ptr = m_handle_table[handle];
		m_handle_table[handle] = nullptr;
	}
private:
	void fill_free_block(byte* free_block, const FreeBlockInfo& _info) {
		// store each byte in little endian
		*free_block = *_info.m_next_free;
		*(free_block + 8) = _info.m_free_size;
		//*free_block =        (*_info.m_next_free) & 0xFF;
		//*(free_block + 1) = ((*_info.m_next_free) >> 8) & 0xFF;
		//*(free_block + 2) = ((*_info.m_next_free) >> 16) & 0xFF;
		//*(free_block + 3) = ((*_info.m_next_free) >> 24) & 0xFF;
		//*(free_block + 4) = ((*_info.m_next_free) >> 32) & 0xFF;
		//*(free_block + 5) = ((*_info.m_next_free) >> 40) & 0xFF;
		//*(free_block + 6) = ((*_info.m_next_free) >> 48) & 0xFF;
		//*(free_block + 7) = ((*_info.m_next_free) >> 56) & 0xFF;
		//*(free_block + 8) =   (_info.m_free_size) & 0xFF;
		//*(free_block + 9) =  ((_info.m_free_size) >> 8) & 0xFF;
		//*(free_block + 10) = ((_info.m_free_size) >> 16) & 0xFF;
		//*(free_block + 11) = ((_info.m_free_size) >> 24) & 0xFF;
	}
	// handle table is array of pointers
	std::mutex m_lock;
	static const size_t m_handle_table_size = 128;
	void* m_handle_table[m_handle_table_size]; // be able to store 128 handles
	size_t m_last_handle; // sotores the index of the last handle, use it to begin iterating from to find a new handle;
	byte* m_begin;
	byte* m_current;
	byte* m_end;
};
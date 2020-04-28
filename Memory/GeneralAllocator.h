#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "memory.h"
#include <mutex>

class GeneralAllocator {
	typedef uint8_t byte;
	typedef int32_t Handle;
	struct FreeBlockInfo {
		size_t m_size;		// the size of the free block
		byte* m_prev_free;  // stores the address of the first byte of the next free block
		byte* m_next_free;  // stores the size of this free block in bytes;
		//size_t m_size;		// the size of the free block (needed for reverse traversal) // doesnt 
		FreeBlockInfo(size_t _size, byte* _prev, byte* _next) : m_size(_size), m_prev_free(_prev), m_next_free(_next) {};
	};
public:
	GeneralAllocator() = delete;
	// malloc already aligns to 8 bytes so memory isnt awful
	GeneralAllocator(size_t size) : m_begin(static_cast<byte*>(malloc(size))), m_last_handle(0) {
		assert(size % 32 == 0); // size must be even multiple of 32 because that is the minimum size of a free-chunk (may not make sense to force this requirement)
		m_current = static_cast<byte*>(m_begin);
		m_end = static_cast<byte*>(m_begin) + size;
		for (size_t index = 0; index != m_handle_table_size; index++) {
			m_handle_table[index] = nullptr; // store nullpointers in the table
		}
		// make the allocated memory be one giant freeblock
		FreeBlockInfo free_block(size, nullptr, nullptr);
		fill_free_block(m_current, free_block);
	}
	~GeneralAllocator() {
		delete m_begin;
	};
	
	Handle allocate(size_t size, size_t alignment) {
		// allocate aligned memory from the memory pool and set the handle
		// allocate 4 extra bytes -> first 4 is for number of bytes in this allocation, last 1 byte is for how much we needed to shift to get the allocation correct
		// in the raw memory, we need to write where the next free segment is and how big this free segment is, need both numbers because we cannot get one from the other
		// total allocated size is 8 + 8 + 8 + size + alignment
		// if size + alignment < 8 bytes allocate a minimum of 8 bytes
		std::lock_guard<std::mutex> lock(m_lock); // can assert and/or return without worry
		Handle handle;
		size_t sum = size + alignment;
		if (sum < 8) { sum = 8; alignment = 8; } //  make alignment 8?
		//byte* next_free_block;

		size_t total_bytes = 8 + 8 + 8 + sum; // total size the allocated block will take up

		 // manipulate free_block pointer to find a suitable free blocks
		byte* free_block = m_current;
		size_t free_size = get_free_size(free_block);
		
		
		// free_size needs to be 32 bytes larger than what we need, unless it fills it up exactly (optimization to add later)
		byte* prev_free_block = static_cast<byte*>(get_prev_free(&free_block));
		byte* next_free_block = static_cast<byte*>(get_next_free(&free_block));
		if (!(free_size + 32 >= total_bytes)) { // current free block will not fit
			// search backwards for a freeblock
			while (prev_free_block && !(free_size + 32 >= total_bytes)) { // keep searching until prev_free is either a nullptr, or the the free_block is large enough
				prev_free_block = static_cast<byte*>(get_prev_free(&prev_free_block));
				free_size = get_free_size(prev_free_block);
			}
			free_block = static_cast<byte*>(prev_free_block);
			// if prev_free_block is not a nullptr, then we found a block that can fit already and this while loop will be skipped
			if (!prev_free_block) {
				// search forwards for a freeblock
				while (next_free_block && !(free_size + 32 >= total_bytes)) {
					next_free_block = static_cast<byte*>(get_next_free(&next_free_block));
					free_size = get_free_size(next_free_block);
				}
				free_block = static_cast<byte*>(next_free_block);
			}
			if (!free_block) return -1;
		}
		// from here on out we have found a block that will fit what we need
		// free_block holds the pointer to the free block we can store into
		
		free_size = get_free_size(free_block); // probably unnecessary because freesize is set in a loop
		byte* _prev = get_prev_free(&free_block);
		byte* _next = get_next_free(&free_block);
		FreeBlockInfo remaining_free((free_size - total_bytes), get_prev_free(&free_block), get_next_free(&free_block)); // get the info that will be stored in the leftover bytes
		set_free_size(free_block, total_bytes); // set the first and last 8 bytes to be the size of the allocation
		byte* user_mem = free_block + 24;
		void* p_aligned = align(user_mem, alignment); // add 24 so we skip over the first 3 8 byte values
		// set the remaining free block
		byte* remaining_free_block = free_block + total_bytes;
		set_next_free(&free_block, remaining_free_block); // previous pointer stays the same
		byte* po = get_next_free(&free_block);
		fill_free_block(remaining_free_block, remaining_free); // edits the last 8 bytes with the new size so we dont need to do it ourselves
		m_current = remaining_free_block;
		handle = get_free_handle();
		m_handle_table[handle] = p_aligned;
		
		return handle;
	
	}
	void free(Handle handle) {
		m_lock.lock();
		// clear entry into handle table
		void* ptr = m_handle_table[handle];
		m_handle_table[handle] = nullptr;

		// get the base pointer
		byte* p_aligned_mem = reinterpret_cast<byte*>(ptr);
		ptrdiff_t shift = *(p_aligned_mem - 1);
		if (shift == 0) shift = 256;
		byte* this_free_block = p_aligned_mem - shift - 24; // subtract 24 to get to base of allocation
		// follow the previous and next pointer
		size_t this_free_size = get_free_size(this_free_block);
		byte* prev_free_block = static_cast<byte*>(get_prev_free(&this_free_block));
		byte* next_free_block = static_cast<byte*>(get_next_free(&this_free_block));
		// insert the block into the free list
		set_next_free(&prev_free_block, this_free_block);
		set_prev_free(&next_free_block, this_free_block);

		// coalesce forwards first because forwards direction doesnt change the this_free_block pointer

		size_t next_free_size = get_free_size(next_free_block);
		// coalesce forwards (should only run 1 or 2 times)
		while (next_free_block && (this_free_block + this_free_size == next_free_block)) { // next free block is adjacent to us to coalesce
			set_free_size(this_free_block, this_free_size + next_free_size);
			set_next_free(&this_free_block, static_cast<byte*>(get_next_free(&next_free_block))); // set our next pointer to be the next of the block we absorbed
			this_free_size = get_free_size(this_free_block);
			next_free_block = static_cast<byte*>(get_next_free(&this_free_block)); // set the pointer to the next_free_block to be our next block (because we set out next block already)
			next_free_size = get_free_size(next_free_block);
			// no need to update this_free_block
		}
		// coalesce backwards (should only run 1 or 2 times)
		size_t prev_free_size = get_free_size(prev_free_block);
		while (prev_free_block && (prev_free_block + prev_free_size == this_free_block)) { // previous free block is adjacent to us so coalesce
			set_free_size(prev_free_block, prev_free_size + this_free_size); // set the size to be the combined size
			// prev pointer of prev block stays the same
			set_next_free(&prev_free_block, static_cast<byte*>(get_next_free(&this_free_block))); // set the next pointer of previous to our next
			this_free_block = prev_free_block; // set ourselves to be the previous
			prev_free_size = get_free_size(this_free_block); // grab our new previous block
			prev_free_block = static_cast<byte*>(get_prev_free(&this_free_block)); // update our pointer to the previous to be our previous
		}
		m_lock.unlock();
	}
private:
	Handle get_free_handle() {
		size_t index;
		for (index = 0; index != m_handle_table_size; ++index, ++m_last_handle) { // loop until the handle table entry is a null pointer
			if (m_handle_table[m_last_handle & m_handle_table_size] == nullptr) break;
		}
		if (index == m_handle_table_size) assert(false); // traversed entire handle table without finding a free handle;
		return m_last_handle; // return the free handle we found
	}
	void fill_free_block(byte* free_block, const FreeBlockInfo& _info) {
		// TODO: need to store everybyte individually
		//*free_block = _info.m_size;
		size_t size = _info.m_size;
		byte* prev = _info.m_prev_free;
		byte* next = _info.m_next_free;
		// need to pass the copies because for some godforsaken reason the _info struct get's set to null after set_prev_size
		set_free_size(free_block, size);
		set_prev_free(&free_block, prev);
		set_next_free(&free_block, next);
		size_t _size = get_free_size(free_block);
		byte* _prev = get_prev_free(&free_block);
		byte* _next = get_next_free(&free_block);
	}
	void set_prev_free(byte** free_block, byte* prev_free) {
		byte* temp = prev_free;
		free_block += 1;
		*free_block = temp;
		// if (!prev_free) {
		// 	*free_block = NULL;
		// 	*(free_block + 1) = NULL;
		// 	*(free_block + 2) = NULL;
		// 	*(free_block + 3) = NULL;
		// 	*(free_block + 4) = NULL;
		// 	*(free_block + 5) = NULL;
		// 	*(free_block + 6) = NULL;
		// 	*(free_block + 7) = NULL;
		// 	return;
		// }
		// *free_block = *prev_free;
		// *(free_block + 1) = *(prev_free + 1);
		// *(free_block + 2) = *(prev_free + 2);
		// *(free_block + 3) = *(prev_free + 3);
		// *(free_block + 4) = *(prev_free + 4);
		// *(free_block + 5) = *(prev_free + 5);
		// *(free_block + 6) = *(prev_free + 6);
		// *(free_block + 7) = *(prev_free + 7);
		// byte* result = free_block;
	}
	void set_next_free(byte** free_block, byte* next_free) { // need to set each individual byte
		byte* temp = next_free;
		free_block += 2;
		*free_block = temp;
		//// *free_block = reinterpret_cast<uintptr_t>(next_free);
		//if (!next_free) {
		//	*free_block = NULL;
		//	*(free_block + 1) = NULL;
		//	*(free_block + 2) = NULL;
		//	*(free_block + 3) = NULL;
		//	*(free_block + 4) = NULL;
		//	*(free_block + 5) = NULL;
		//	*(free_block + 6) = NULL;
		//	*(free_block + 7) = NULL;
		//	return;
		//}
		//*free_block = *next_free;
		//*(free_block + 1) = *(next_free + 1);
		//*(free_block + 2) = *(next_free + 2);
		//*(free_block + 3) = *(next_free + 3);
		//*(free_block + 4) = *(next_free + 4);
		//*(free_block + 5) = *(next_free + 5);
		//*(free_block + 6) = *(next_free + 6);
		//*(free_block + 7) = *(next_free + 7);
	}
	void set_free_size(byte* free_block, size_t size) {
		//size_t* free_block = reinterpret_cast<size_t*>(free_block);
		// free_block = reinterpret_cast<void*>(size);
		*free_block = size & 0xFF;
		*(free_block + 1) = (size >> 8) & 0xFF;
		*(free_block + 2) = (size >> 16) & 0xFF;
		*(free_block + 3) = (size >> 24) & 0xFF;
		*(free_block + 4) = (size >> 32) & 0xFF;
		*(free_block + 5) = (size >> 40) & 0xFF;
		*(free_block + 6) = (size >> 48) & 0xFF;
		*(free_block + 7) = (size >> 56) & 0xFF;

		size_t trailing = size - 8;
		*(free_block + trailing) = size & 0xFF;
		*(free_block + trailing + 1) = (size >> 8) & 0xFF;
		*(free_block + trailing + 2) = (size >> 16) & 0xFF;
		*(free_block + trailing + 3) = (size >> 24) & 0xFF;
		*(free_block + trailing + 4) = (size >> 32) & 0xFF;
		*(free_block + trailing + 5) = (size >> 40) & 0xFF;
		*(free_block + trailing + 6) = (size >> 48) & 0xFF;
		*(free_block + trailing + 7) = (size >> 56) & 0xFF;
	}
	byte* get_prev_free(byte** free_block) {
		//return reinterpret_cast<void*>(*(free_block + 8));
		free_block += 1;
		return *free_block;
		//return reinterpret_cast<byte*>(*free_block);
	}
	byte* get_next_free(byte** free_block) {
		//return reinterpret_cast<void*>(*(free_block + 16));
		free_block += 2;
		return *free_block;
		//return reinterpret_cast<byte*>(*free_block);
	}
	size_t get_free_size(byte* free_block) {
		if (!free_block) return 0; // return zero if the free_block is a nullptr
		size_t a = *free_block;
		size_t b = *(free_block + 1) << 8;
		size_t c = *(free_block + 2) << 16;
		size_t d = *(free_block + 3) << 24;
		size_t e = *(free_block + 4) << 32;
		size_t f = *(free_block + 5) << 40;
		size_t g = *(free_block + 6) << 48;
		size_t h = *(free_block + 7) << 56;
		//return reinterpret_cast<size_t>(free_block); // should be getting the first 8 bytes
		return (a + b + c + d + e + f + g + h);
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
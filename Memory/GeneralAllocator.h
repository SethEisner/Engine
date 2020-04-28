#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "memory.h"
#include <mutex>

/* TODO:
	fix search for free block large enough in allocate()
	implement coalescing in free()
	implement defragmenting
*/

class GeneralAllocator {
	typedef uint8_t byte;
	typedef int32_t Handle;
	struct FreeBlockInfo {
		int64_t m_size;		// the size of the free block, make negative if it's free
		// pointers to single bytes in memory
		uintptr_t* m_prev_free;  // stores the address of the first byte of the next free block
		uintptr_t* m_next_free;  // stores the size of this free block in bytes;
		//size_t m_size;		// the size of the free block (needed for reverse traversal) // doesnt 
		FreeBlockInfo(size_t _size, uintptr_t* _prev, uintptr_t* _next) : m_size(_size), m_prev_free(_prev), m_next_free(_next) {};
	};
public:
	GeneralAllocator() = delete;
	// malloc already aligns to 8 bytes so memory isnt awful
	GeneralAllocator(int64_t size) : m_begin(static_cast<uintptr_t*>(malloc(size))), m_last_handle(0) {
		assert(size > 0);
		assert(size % 32 == 0); // size must be even multiple of 32 because that is the minimum size of a free-chunk (may not make sense to force this requirement)
		assert(size % 512 == 0);
		m_current = reinterpret_cast<byte*>(m_begin);
		m_end = reinterpret_cast<uintptr_t*>(m_current + size); // cast to byte pointer so we can add to it
		for (size_t index = 0; index != m_handle_table_size; index++) {
			m_handle_table[index] = nullptr; // store nullpointers in the table
		}
		// make the allocated memory be one giant freeblock
		set_free_size(reinterpret_cast<uintptr_t*>(m_current), -size); // freeblocks have negative size
		set_prev_free(reinterpret_cast<uintptr_t*>(m_current), reinterpret_cast<uintptr_t>(nullptr));
		set_next_free(reinterpret_cast<uintptr_t*>(m_current), reinterpret_cast<uintptr_t>(nullptr));
		// size_t _size = get_free_size(reinterpret_cast<uintptr_t*>(m_current));
		// uintptr_t prev = get_prev_free(reinterpret_cast<uintptr_t*>(m_current));
		// uintptr_t next = get_next_free(reinterpret_cast<uintptr_t*>(m_current));
	}
	~GeneralAllocator() {
		delete m_begin;
	};
	
	Handle allocate(size_t size, size_t alignment) {
		std::lock_guard<std::mutex> lock(m_lock); // can assert and/or return without worry
		Handle handle;
		size_t sum = size + alignment;
		if (sum < 8) sum, alignment = 8; //  make alignment 8?
	
		int64_t total_bytes = 8 + 8 + 8 + sum; // total size the allocated block will take up
	
		 // manipulate free_block pointer to find a suitable free blocks
		uintptr_t* free_block = reinterpret_cast<uintptr_t*>(m_current);
		int64_t free_size = get_free_size(free_block);
		bool block_is_free = false;
		if (free_size < 0) {
			block_is_free = true;
			free_size = -free_size; // negative means the block is free
		}
		
		// free_size needs to be 32 bytes larger than what we need, unless it fills it up exactly (optimization to add later)
		uintptr_t* prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
		uintptr_t* next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
		// total_bytes + 32 > free_size
		// BROKEN!!!!
		if (!(free_size + 32 >= total_bytes)) { // current free block will not fit
			// search backwards for a freeblock
			while (prev_free_block && !(free_size + 32 >= total_bytes)) { // keep searching until prev_free is either a nullptr, or the the free_block is large enough
				prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(prev_free_block));
				free_size = -get_free_size(prev_free_block); // may be reinterpretting to wrong type
			}
			free_block = prev_free_block;
			// if prev_free_block is not a nullptr, then we found a block that can fit already and this while loop will be skipped
			if (!prev_free_block) {
				// search forwards for a freeblock
				while (next_free_block && !(free_size + 32 >= total_bytes)) {
					next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(next_free_block));
					free_size = -get_free_size(next_free_block);
				}
				free_block = next_free_block;
			}
			if (!free_block) return -1;
		}
		// from here on out we have found a block that will fit what we need
		// free_block holds the pointer to the free block we can store into
	
		free_size = -get_free_size(free_block); // probably unnecessary because freesize is set in a loop
		uintptr_t* _prev = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
		uintptr_t* _next = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
		//FreeBlockInfo remaining_free((free_size - total_bytes), reinterpret_cast<uintptr_t*>(get_prev_free(free_block)), reinterpret_cast<uintptr_t*>(get_next_free(free_block))); // get the info that will be stored in the leftover bytes
		set_free_size(free_block, total_bytes); // set the first and last 8 bytes to be the size of the allocation
		byte* user_mem = reinterpret_cast<byte*>(free_block) + 24;
		void* p_aligned = align(user_mem, alignment); // add 24 so we skip over the first 3 8 byte values
		// set the remaining free block
		uintptr_t* remaining_free_block = reinterpret_cast<uintptr_t*>(reinterpret_cast<byte*>(free_block) + total_bytes);
		set_free_size(remaining_free_block, -(free_size - total_bytes));
		set_prev_free(remaining_free_block, get_prev_free(free_block));
		set_next_free(remaining_free_block, get_next_free(free_block));
		m_current = reinterpret_cast<byte*>(remaining_free_block);
		handle = get_free_handle();
		m_handle_table[handle] = p_aligned;
	
		return handle;
	}
	void free(Handle handle) {
		std::lock_guard<std::mutex> lock(m_lock);
		// clear entry into handle table
		void* ptr = m_handle_table[handle];
		m_handle_table[handle] = nullptr;

		// get the base pointer
		byte* p_aligned_mem = reinterpret_cast<byte*>(ptr);
		ptrdiff_t shift = *(p_aligned_mem - 1);
		if (shift == 0) shift = 256;
		uintptr_t* this_free_block = reinterpret_cast<uintptr_t*>(p_aligned_mem - shift - sizeof(FreeBlockInfo));
		int64_t this_free_size = get_free_size(this_free_block);
		uintptr_t* prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block));
		uintptr_t* next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(this_free_block));
		
		uintptr_t* next_block = this_free_block + (this_free_size / 8);
		uintptr_t* prev_block = this_free_block;
		int64_t free_size;// = 0;
		if (next_block < m_end && !next_free_block ) { // next free block is a nullpointer so we need to go looking for an actual one
			free_size = get_free_size(next_block);
			// if free_size is negative, then next_free_block automatically has 
			while (next_block < m_end && free_size > 0) { // we are not out of bounds or the block we are looking at is in use
				next_block += free_size / 8; // fast division because it's a shift by 2
				free_size = get_free_size(next_block);
			}
			next_free_block = next_block; // next_free_block is either the next free block, or it is out of bounds
		}
		if (this_free_block > m_begin && !prev_free_block) { // need to make sure we can actually traverse backwards and that our prev_free_block is null so we actually need to find one
			do {
				free_size = *(this_free_block - 1) / 8;
				if (free_size < 0) break;
				prev_block -= free_size;
			} while (prev_block > m_begin);
			prev_free_block = prev_block;
		}
		// check that next_free and prev_free_block are inbounds before assigning them

		if (next_free_block > m_end) {
			next_free_block = nullptr;
		}
		else { // set the next_block's previous to me
			set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(this_free_block));
		}
		if (prev_free_block < m_begin) {
			prev_free_block = nullptr;
		}
		else { // set the prev_block's next to me
			set_next_free(prev_free_block, reinterpret_cast<uintptr_t>(this_free_block));
		}
		// set my next to the found next
		set_next_free(this_free_block, reinterpret_cast<uintptr_t>(next_free_block));
		// set my prev to the found prev
		set_prev_free(this_free_block, reinterpret_cast<uintptr_t>(prev_free_block));
		
		m_current = reinterpret_cast<byte*>(this_free_block);
		set_free_size(this_free_block, -this_free_size);
		

		// // insert the block into the free list
		// set_next_free(&prev_free_block, *this_free_block);
		// set_prev_free(&next_free_block, *this_free_block);
		// 
		// // coalesce forwards first because forwards direction doesnt change the this_free_block pointer
		// 
		// size_t next_free_size = get_free_size(next_free_block);
		// // coalesce forwards (should only run 1 or 2 times)
		// while (next_free_block && (*this_free_block + this_free_size == next_free_block)) { // next free block is adjacent to us to coalesce
		// 	set_free_size(*this_free_block, this_free_size + next_free_size);
		// 	set_next_free(this_free_block, static_cast<byte*>(get_next_free(&next_free_block))); // set our next pointer to be the next of the block we absorbed
		// 	this_free_size = get_free_size(*this_free_block);
		// 	next_free_block = static_cast<byte*>(get_next_free(this_free_block)); // set the pointer to the next_free_block to be our next block (because we set out next block already)
		// 	next_free_size = get_free_size(next_free_block);
		// 	// no need to update this_free_block
		// }
		// coalesce backwards (should only run 1 or 2 times)
		//size_t prev_free_size = get_free_size(prev_free_block);
		//while (prev_free_block && (prev_free_block + prev_free_size == this_free_block)) { // previous free block is adjacent to us so coalesce
		//	set_free_size(prev_free_block, prev_free_size + this_free_size); // set the size to be the combined size
		//	// prev pointer of prev block stays the same
		//	set_next_free(&prev_free_block, static_cast<byte*>(get_next_free(&this_free_block))); // set the next pointer of previous to our next
		//	this_free_block = prev_free_block; // set ourselves to be the previous
		//	prev_free_size = get_free_size(this_free_block); // grab our new previous block
		//	prev_free_block = static_cast<byte*>(get_prev_free(&this_free_block)); // update our pointer to the previous to be our previous
		//}
	}
private:
	Handle get_free_handle() {
		size_t index;
		for (index = 0; index != m_handle_table_size; ++index, ++m_last_handle) { // loop until the handle table entry is a null pointer
			if (m_handle_table[m_last_handle % m_handle_table_size] == nullptr) break;
		}
		if (index == m_handle_table_size) assert(false); // traversed entire handle table without finding a free handle;
		return m_last_handle; // return the free handle we found
	}
	
	void set_prev_free(uintptr_t* free_block, uintptr_t prev_free) {
		free_block += 1;
		*free_block = prev_free;
	}
	void set_next_free(uintptr_t* free_block, uintptr_t next_free) {
		free_block += 2;
		*free_block = next_free;
	}
	uintptr_t get_prev_free(uintptr_t* free_block) {
		free_block += 1;
		return *free_block;
	}
	uintptr_t get_next_free(uintptr_t* free_block) {
		free_block += 2;
		return *free_block;
	}
	
	void set_free_size(uintptr_t* free_block, int64_t size) {
		*free_block = size;
		size_t trailing = (size/8) - 1;
		auto addr = free_block + trailing;
		*(free_block + trailing) = size;
	}

	int64_t get_free_size(uintptr_t* free_block) {
		if (!free_block) return 0; // return zero if the free_block is a nullptr
		return static_cast<int64_t>(*free_block);
	}
	std::mutex m_lock;
	static const size_t m_handle_table_size = 128;
	void* m_handle_table[m_handle_table_size]; // be able to store 128 handles
	size_t m_last_handle; // sotores the index of the last handle, use it to begin iterating from to find a new handle;
	// these are byte* because they hold addresses to specific allocated bytes
	uintptr_t* m_begin;
	byte* m_current;
	uintptr_t* m_end;
};
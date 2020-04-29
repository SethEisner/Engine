#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "memory.h"
#include <mutex>

/* TODO:
	implement coalescing in free()
	implement defragmenting
*/

class GeneralAllocator {
	typedef uint8_t byte;
	typedef int32_t Handle;
	struct FreeBlockInfo {
		int64_t m_size;		// the size of the free block, make negative if it's free
		uintptr_t* m_prev_free;  // stores the address of the first byte of the next free block
		uintptr_t* m_next_free;  // stores the size of this free block in bytes;
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
		int64_t required_size;
		if (sum < 8) { // need to make sure we allocate enough space for the eventual freeblock
			alignment = 4; // set the alignment to half way inbetween the last byte
			required_size = 32; // make sure we alocate at least 32 bytes
			// added the last 8 bytes automatically for the alignment to occur without worry
		}
		//else if (sum < 24){ // before we do this, we need to make sure that the pointer we return has skipped any of the book keeping memory (set to tskip 24 bytes right now)
		//	required_size = 32;
		//}
		else { // may be too complicated if we overwrite those pointers because we're supposed to leave space for them if the size is small enough
			// sum includes the required extra space for the alignment so we dont need to worry about it and can just allocate what we need for everything
			required_size = 8 + 8 + 8 + sum;
			//requred_size = 8 + sum; (if the sum is 24 or greater than we can overwrite the pointers)
		}
	
		 // manipulate free_block pointer to find a suitable free blocks
		uintptr_t* free_block = reinterpret_cast<uintptr_t*>(m_current);
		int64_t free_size = -get_free_size(free_block);
		assert(free_size > 0); //m_current should always be set to a free block so we should always start out search from a free block
		// bool block_is_free = false;
		// if (free_size < 0) {
		// 	block_is_free = true;
		// 	free_size = -free_size; // negative means the block is free
		// }
		// free_size needs to be 32 bytes larger than what we need, unless it fills it up exactly (optimization to add later)
		uintptr_t* prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
		uintptr_t* next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));

		if (!(required_size == free_size || free_size >= required_size + 32)) { // the block we are looking at is too small so we need to find a free block big enough
			while (prev_free_block) {
				free_size = -get_free_size(prev_free_block);
				assert(free_size > 0); // make sure free blocks are only connected to other freeblocks
				if (required_size == free_size || free_size >= required_size + 32) { // this block will fit what we need
					free_block = prev_free_block;
					prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
					next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
					goto found_block;
				}
				prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(prev_free_block));
			}
			while(next_free_block){
				free_size = -get_free_size(next_free_block);
				assert(free_size > 0); // make sure free blocks are only connected to other freeblocks
				if (required_size == free_size || free_size >= required_size + 32) {
					free_block = next_free_block;
					prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
					next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
					goto found_block;
				}
				next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(next_free_block));
			}
			// no free block found so return a handle that can't exist
			return -1;
		}
	found_block: // we found a block in the loop and got all of it's pointers
		if (free_size == required_size) { // the freeblock exactly fits what we need, dont need to create a new freeblock at the end, but do need to update the free-block chain
			// update the pointers to remove this block from the freechain
			if (prev_free_block) {
				set_next_free(prev_free_block, reinterpret_cast<uintptr_t>(next_free_block)); // set the next of the previous to be our next
				m_current = reinterpret_cast<byte*>(prev_free_block);
				auto temp = get_next_free(prev_free_block);
			}
			if (next_free_block) {
				set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(prev_free_block)); // set the prev of our next to be our prev
				m_current = reinterpret_cast<byte*>(next_free_block);
				auto temp = get_prev_free(next_free_block);
			}
			if (!prev_free_block && !next_free_block) {
				// should only get here if the freeblock we used were the last bits of allocated memory
				m_current = nullptr;
			}
		}
		else if (free_size >= required_size + 32) {
			//assert(free_size >= required_size + 32); // should only get here if we found a block that will fit our allocation
			uintptr_t* remaining_free_block = reinterpret_cast<uintptr_t*>(reinterpret_cast<byte*>(free_block) + required_size);
			set_free_size(remaining_free_block, -(free_size - required_size)); // size is set to negative to indicate that it's free
			if (prev_free_block) { // if there is a freeblock we can reach before us, make its next pointer point to this new freeblock
				set_next_free(prev_free_block, reinterpret_cast<uintptr_t>(remaining_free_block));
			}
			if (next_free_block) { // if there is a freeblock we can reach ahead of us, make its prev pointer point to us
				set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(remaining_free_block));
			}
			set_prev_free(remaining_free_block, reinterpret_cast<uintptr_t>(prev_free_block));
			set_next_free(remaining_free_block, reinterpret_cast<uintptr_t>(next_free_block));

			auto temp0 = get_free_size(remaining_free_block);
			auto temp1 = get_prev_free(remaining_free_block);
			auto temp2 = get_next_free(remaining_free_block);
			m_current = reinterpret_cast<byte*>(remaining_free_block);
		}
		set_free_size(free_block, required_size); // set the first and last 8 bytes to be the size of the allocation
		set_next_free(free_block, reinterpret_cast<uintptr_t>(nullptr)); // want these to be null because we dont want to rely on the pointers in an allocated block
		set_prev_free(free_block, reinterpret_cast<uintptr_t>(nullptr));
		auto temp1 = get_next_free(free_block);
		auto temp2 = get_prev_free(free_block);
		byte* user_mem = reinterpret_cast<byte*>(free_block) + 24;
		void* p_aligned = align(user_mem, alignment); // add 24 so we skip over the first 3 8 byte values
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
		// dont want to read these pointers from an allocated block, because they hold an old view of the free list and will eventually cause a memory leak if used
		uintptr_t* prev_free_block = nullptr;
		uintptr_t* next_free_block = nullptr;
		
		uintptr_t* next_block = this_free_block + (this_free_size / 8);
		uintptr_t* prev_block = this_free_block;
		int64_t free_size;// = 0;
		if (next_block < m_end/* && !next_free_block */) { // next free block is a nullpointer so we need to go looking for an actual one
			free_size = get_free_size(next_block);
			// if free_size is negative, then next_free_block automatically has 
			while (next_block < m_end && free_size > 0) { // we are not out of bounds or the block we are looking at is in use
				next_block += free_size / 8; // fast division because it's a shift by 2
				free_size = get_free_size(next_block);
			}
			next_free_block = next_block; // next_free_block is either the next free block, or it is out of bounds
		}
		if (this_free_block > m_begin/* && !prev_free_block*/) { // need to make sure we can actually traverse backwards and that our prev_free_block is null so we actually need to find one
			do { // need to get past myself
				free_size = static_cast<int64_t>(*(this_free_block - 1)) / 8;
				if (free_size < 0) {
					prev_block += free_size; // freesize is negative so we add to subtract (needed to get the start of the prev block because we read the end)
					break;
				}
				prev_block -= free_size;
			} while (prev_block > m_begin);
			prev_free_block = prev_block;
		}
		// check that next_free and prev_free_block are inbounds before assigning them

		if (next_free_block >= m_end) {
			next_free_block = nullptr;
		}
		else { // set the next_block's previous to me
			set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(this_free_block));
		}
		if (prev_free_block <= m_begin) {
			prev_free_block = nullptr;
		}
		else { // set the prev_block's next to me
			set_next_free(prev_free_block, reinterpret_cast<uintptr_t>(this_free_block));
		}
		// set my next to the found next
		set_next_free(this_free_block, reinterpret_cast<uintptr_t>(next_free_block));
		// set my prev to the found prev
		set_prev_free(this_free_block, reinterpret_cast<uintptr_t>(prev_free_block));
		
		auto temp1 = get_next_free(this_free_block);
		auto temp2 = get_prev_free(this_free_block);
		m_current = reinterpret_cast<byte*>(this_free_block);
		set_free_size(this_free_block, -this_free_size);
		auto temp3 = get_free_size(this_free_block);

		// hanlde coalescing


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
		size_t trailing = (abs(size)/8) - 1;
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
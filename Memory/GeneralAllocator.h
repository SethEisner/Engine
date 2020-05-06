#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "memory.h"
#include <mutex>
/* TODO: */

// based off of dlmalloc: http://gee.cs.oswego.edu/dl/html/malloc.html

static const int bytes_for_size = 8;
static const int bytes_for_handle = 8;
static const int bytes_for_free_block = bytes_for_size + bytes_for_size + bytes_for_handle + 8; // 8 is minimum allocation size

// ERROR STATES
static constexpr bool GENERAL_ALLOCATOR_OUT_OF_MEMORY = false;

typedef size_t Handle;

class GeneralAllocator {
	typedef uint8_t byte;
public:
	GeneralAllocator() = delete;
	// malloc already aligns to 8 bytes so memory isnt awful
	// malloc already aligns to 8 bytes so memory isnt awful
	GeneralAllocator(int64_t size) : m_begin(static_cast<uintptr_t*>(malloc(size))), m_last_handle(0) {
		assert(size > 0);
		assert(size % bytes_for_free_block == 0); // size must be even multiple of 32 because that is the minimum size of a free-chunk (may not make sense to force this requirement)
		m_current = reinterpret_cast<byte*>(m_begin);
		m_end = reinterpret_cast<uintptr_t*>(m_current + size); // cast to byte pointer so we can add to it
		for (size_t index = 0; index != m_handle_table_size; index++) { // initialize handle table to be empty
			m_handle_table[index] = nullptr;
		}
		// make the allocated memory start as one giant freeblock
		set_free_size(reinterpret_cast<uintptr_t*>(m_current), -size); // freeblocks have negative size to tell if theyre in use
		set_prev_free(reinterpret_cast<uintptr_t*>(m_current), reinterpret_cast<uintptr_t>(nullptr));
		set_next_free(reinterpret_cast<uintptr_t*>(m_current), reinterpret_cast<uintptr_t>(nullptr));
	}
	~GeneralAllocator() {
		delete m_begin;
	};
	void* get_pointer(Handle h) { // no need to lock because this function is read only
		assert(m_handle_table[h] != nullptr);
		return m_handle_table[h];
	}
	Handle register_allocated_mem(void* allocated_block) {
		std::lock_guard<std::mutex> lock(m_lock);
		// maybe add a check that asserts if we try to get a handle for a block that already has one
		// get a handle we can use
		Handle handle = get_open_handle();
		m_handle_table[handle] = allocated_block;
		// get the start of the block we were handed so set the handle
		uintptr_t* this_block = get_block_pointer(handle);
		set_prev_free(this_block, static_cast<uintptr_t>(handle)); // store the handle for defragmenting purposes
		return handle;
	}
	void* allocate(size_t size, size_t alignment) {
		std::lock_guard<std::mutex> lock(m_lock); // can assert and/or return without worry
		size_t sum = size + alignment;
		if (sum < 8) { // need to make sure we allocate enough space for the eventual freeblock
			alignment = 4; // set the alignment to half way inbetween the last byte, will make the aligned pointer function use 8 bytes
			sum = 8;
		}
		else if (alignment < 8) { // if we are larger than 1 byte and our alignment is less than 8, we need to set our size to be a multiple of 8
			alignment = 8;
			sum = size - (size % 8) + 8 + alignment; // get the smallest multiple of 8 that is larger than us
		}
		int64_t required_size = bytes_for_size + bytes_for_handle + sum + bytes_for_size;
		assert(required_size % 8 == 0); //required size must be a multiple of 8
		// manipulate free_block pointer to find a suitable free block
		uintptr_t* free_block = reinterpret_cast<uintptr_t*>(m_current);
		int64_t free_size = -get_free_size(free_block);
		assert(free_size > 0); // m_current should always be set to a free block so we should always start out search from a free block
		uintptr_t* prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
		uintptr_t* next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));

		if (!(required_size == free_size || free_size >= required_size + bytes_for_free_block)) { // the block we are looking at is too small so we need to find a free block big enough
			while (prev_free_block) { // seach backwards
				free_size = -get_free_size(prev_free_block);
				assert(free_size > 0); // make sure free blocks are only connected to other freeblocks
				if (required_size == free_size || free_size >= required_size + bytes_for_free_block) { // this block will fit what we need
					free_block = prev_free_block;
					prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
					next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
					goto found_block;
				}
				prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(prev_free_block));
			}
			while (next_free_block) { // search forwards
				free_size = -get_free_size(next_free_block);
				assert(free_size > 0); // make sure free blocks are only connected to other freeblocks
				if (required_size == free_size || free_size >= required_size + bytes_for_free_block) {
					free_block = next_free_block;
					prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(free_block));
					next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(free_block));
					goto found_block;
				}
				next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(next_free_block));
			}
			// no free block large enough found so return a handle that can't exist
			assert(GENERAL_ALLOCATOR_OUT_OF_MEMORY);
			return nullptr;
		}
	found_block: // we found a block in the loop and got all of it's pointers
		if (free_size == required_size) { // the freeblock exactly fits what we need, dont need to create a new freeblock at the end, but do need to update the free-block chain
			// update the pointers to remove this block from the freechain
			if (prev_free_block) {
				set_next_free(prev_free_block, reinterpret_cast<uintptr_t>(next_free_block)); // set the next of the previous to be our next
				m_current = reinterpret_cast<byte*>(prev_free_block);
			}
			if (next_free_block) {
				set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(prev_free_block)); // set the prev of our next to be our prev
				m_current = reinterpret_cast<byte*>(next_free_block);
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
			m_current = reinterpret_cast<byte*>(remaining_free_block);
		}
		set_free_size(free_block, required_size); // set the first and last 8 bytes to be the size of the allocation
		byte* user_mem = reinterpret_cast<byte*>(free_block) + bytes_for_size + bytes_for_handle; // skip over the metadata to the start of userdata
		return align(user_mem, alignment); // return the aligned pointer
	}
	void free(Handle handle) {
		std::lock_guard<std::mutex> lock(m_lock);
		// get the base pointer -> this_free_block
		uintptr_t* this_free_block = get_block_pointer(handle);
		// clear entry into handle table
		m_handle_table[handle] = nullptr;
		int64_t this_free_size = get_free_size(this_free_block);
		// dont want to read these pointers from an allocated block, because they hold an old view of the free list and will eventually cause a memory leak if used
		uintptr_t* prev_free_block = nullptr;
		uintptr_t* next_free_block = nullptr;
		// update the freelist
		uintptr_t* next_block = this_free_block + (this_free_size / 8);
		uintptr_t* prev_block = this_free_block;
		int64_t free_size;// = 0;
		if (next_block < m_end) { // next free block is a nullpointer so we need to go looking for an actual one
			free_size = get_free_size(next_block);
			while (next_block < m_end && free_size > 0) { // we are not out of bounds or the block we are looking at is in use
				next_block += free_size / 8; // fast division because it's a shift by 2
				free_size = get_free_size(next_block);
			}
			next_free_block = next_block; // next_free_block is either the next free block, or it is out of bounds
		}
		if (this_free_block > m_begin) { // need to make sure we can actually traverse backwards and that our prev_free_block is null so we actually need to find one
			while (prev_block > m_begin) {
				free_size = get_adjacent_free_size(prev_block);
				if (free_size < 0) {
					prev_block += (free_size / 8); // freesize is negative so we add to subtract (needed to get the start of the prev block because we read the end)
					break;
				}
				prev_block -= (free_size / 8);
			}
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
		set_free_size(this_free_block, -this_free_size); // store the negative value because we are now a complete freeblock
		set_next_free(this_free_block, reinterpret_cast<uintptr_t>(next_free_block)); // set my next to the found next
		set_prev_free(this_free_block, reinterpret_cast<uintptr_t>(prev_free_block)); // set my prev to the found prev

		//	COALESCE the free blocks around us into one big block
		this_free_block = coalesce(this_free_block);
		// DEBUG
		// coalesce changes the metadata in this_free_block so we need to get that changed metadata for the asserts to work
		auto _next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(this_free_block)); // inorder for asserts below to work, not necessary for full speed version
		auto _prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block)); // inorder for asserts below to work, not necessary for full speed version
		// asserts to catch any corruption of the free list 
		if (_next_free_block) assert(this_free_block == reinterpret_cast<uintptr_t*>(get_prev_free(_next_free_block)));
		if (_prev_free_block) assert(this_free_block == reinterpret_cast<uintptr_t*>(get_next_free(_prev_free_block)));
		if (_next_free_block && _prev_free_block) assert(get_prev_free(_next_free_block) == get_next_free(_prev_free_block));
		// END DEBUG
		m_current = reinterpret_cast<byte*>(this_free_block);
	}
	void defragment() {
		std::lock_guard<std::mutex> lock(m_lock);
		// start at m_current
		if (!m_current) return; // cannot defragment if we are full
		// find the first free block
		uintptr_t* this_free_block = reinterpret_cast<uintptr_t*>(m_current);
		uintptr_t* prev_free_block;
		uintptr_t* next_free_block;
		while (prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block))) { // loop backwards until the previous block pointer is a nullptr (reached start of freechain)
			this_free_block = prev_free_block; // update this free_block to be the previous
		}
		// this_free_block now contains the first block in the free chain
		int64_t this_free_size = -get_free_size(this_free_block); // this_free_size must be positive here
		prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block)); // unnecessary because it's set in the loop
		next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(this_free_block));
		assert(prev_free_block == nullptr);

		uintptr_t* next_block = this_free_block + (this_free_size / 8);
		while (next_block < m_end) { // the next block is a valid region of memory
			size_t next_block_size = get_free_size(next_block);
			Handle next_block_handle = get_handle_of(next_block);
			assert(this_free_size > 0);
			assert(next_block_size > 0);
			assert(next_block_handle >= 0 && next_block_handle < m_handle_table_size); // the handle stored must be a valid entry in the handle table
			// use memcpy if we can, and memmove if we must. if next block is larger than the free block we must use memmove
			if (get_free_size(next_block) > this_free_size) {
				memmove(this_free_block, next_block, next_block_size); // next block is larger than the freeblock so we must use memmove
			}
			else {
				memcpy(this_free_block, next_block, next_block_size);
			}
			// update the handle entry in the handle table
			m_handle_table[next_block_handle] = reinterpret_cast<void*>(reinterpret_cast<byte*>(m_handle_table[next_block_handle]) - this_free_size);
			// create the new freeblock. next_block holds the pointer to what needs to become a free block
			this_free_block = next_block;
			set_free_size(this_free_block, -this_free_size);
			set_prev_free(this_free_block, reinterpret_cast<uintptr_t>(prev_free_block));
			set_next_free(this_free_block, reinterpret_cast<uintptr_t>(next_free_block));
			// coalesce this newly created freeblock
			this_free_block = coalesce(this_free_block);
			this_free_size = -get_free_size(this_free_block);  // need to update our copies of the info about the free block because of coalesce's side effects
			prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block));
			next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(this_free_block));
			// get the next_block to shift
			next_block = this_free_block + (this_free_size / 8);
		}
		m_current = reinterpret_cast<byte*>(this_free_block);
		assert(reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block)) == nullptr);
		assert(reinterpret_cast<uintptr_t*>(get_next_free(this_free_block)) == nullptr);
	}
private:
	// private functions must not grab a lock because they are called by functions that already hold the lock
	uintptr_t* get_block_pointer(Handle handle) {
		void* ptr = m_handle_table[handle];
		// get the base pointer -> this_free_block
		byte* p_aligned_mem = reinterpret_cast<byte*>(ptr);
		ptrdiff_t shift = *(p_aligned_mem - 1);
		if (shift == 0) shift = 256;
		return reinterpret_cast<uintptr_t*>(p_aligned_mem - shift - bytes_for_size - bytes_for_handle);
	}
	Handle get_open_handle() {
		size_t index;
		for (index = 0; index != m_handle_table_size; ++index, ++m_last_handle) { // loop until the handle table entry is a null pointer
			if (m_handle_table[m_last_handle % m_handle_table_size] == nullptr) break;
		}
		//if (index == m_handle_table_size) assert(false); 
		assert(index != m_handle_table_size); // traversed entire handle table without finding a free handle;
		return m_last_handle; // return the free handle we found
	}
	Handle get_handle_of(uintptr_t* allocated_block) {
		return static_cast<Handle>(get_prev_free(allocated_block));
	}
	uintptr_t* coalesce(uintptr_t* this_free_block) {
		int64_t this_free_size = -get_free_size(this_free_block); // free blocks have negative size so we need to negate it to get the actual size in bytes
		uintptr_t* next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(this_free_block));
		uintptr_t* prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block));
		if (next_free_block && next_free_block < m_end && (this_free_block + (this_free_size / 8) == next_free_block)) { // coalesce forwards
			int64_t next_free_size = -get_free_size(next_free_block);
			this_free_size += next_free_size;
			set_free_size(this_free_block, -this_free_size);
			next_free_block = reinterpret_cast<uintptr_t*>(get_next_free(next_free_block)); // store the next free block of the next free block
		}
		int64_t prev_free_size = -get_free_size(prev_free_block);
		if (prev_free_block && prev_free_block > m_begin && (this_free_block - (prev_free_size / 8) == prev_free_block)) { // coalesce backwards
			// block to our left is free because pointer_math was correct which would only happen if the size was negative
			set_next_free(prev_free_block, get_next_free(this_free_block)); // set the next of the previous to be our next
			this_free_size += prev_free_size;
			set_free_size(prev_free_block, -this_free_size); // set the size to be the negated total sum
			this_free_block = prev_free_block;
			prev_free_block = reinterpret_cast<uintptr_t*>(get_prev_free(this_free_block)); // store the prev free block of the prev_free_block
		}
		// we have coalesced everything we can. need to update our next pointer, and the prev pointer of the next
		// previous pointer stays the same because it was already being stored in the freeblock we coalesced
		set_next_free(this_free_block, reinterpret_cast<uintptr_t>(next_free_block));
		if (next_free_block) set_prev_free(next_free_block, reinterpret_cast<uintptr_t>(this_free_block));
		return this_free_block;
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
		size_t trailing = (abs(size) / 8) - 1;
		*(free_block + trailing) = size;
	}
	int64_t get_free_size(uintptr_t* free_block) {
		if (!free_block) return 0; // return zero if the free_block is a nullptr
		return static_cast<int64_t>(*free_block);
	}
	int64_t get_adjacent_free_size(uintptr_t* free_block) {
		return static_cast<int64_t>(*(free_block - 1));
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
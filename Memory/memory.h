#pragma once

// functions to align the pointer to the pre-allocated memory segment
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

static void* align(uint8_t* p_mem, size_t alignment) {
	uint8_t* p_aligned_mem = align_pointer(p_mem, alignment);
	// align the pointer to be in the second half of the allocated memory if it is already aligned. use the first half to store the alignment info for freeing
	if (p_aligned_mem == p_mem) p_aligned_mem += alignment;
	ptrdiff_t shift = p_aligned_mem - p_mem;
	assert(shift > 0 && shift <= 256); // shift cannot be greater than 256 because the minimum alignment space is 1 byte
	p_aligned_mem[-1] = static_cast<uint8_t>(shift & 0xFF);
	return p_aligned_mem;
}
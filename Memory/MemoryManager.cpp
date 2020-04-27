#include "MemoryManager.h"

MemoryManager* memory_manager = new MemoryManager();

MemoryManager::MemoryManager() :
	m_linear(new LinearAllocator(1024 * 1024)),
	m_stack(new StackAllocator(1024)),
	m_pool4(new PoolAllocator(4, 1024)),
	m_pool8(new PoolAllocator(8, 1024)),
	m_pool16(new PoolAllocator(16, 1024)),
	m_pool32(new PoolAllocator(32, 1024)),
	m_pool64(new PoolAllocator(64, 1024)),
	m_pool128(new PoolAllocator(128, 1024)),
	m_pool256(new PoolAllocator(256, 1024)) {};
MemoryManager::~MemoryManager() {
	delete m_pool256;
	delete m_pool128;
	delete m_pool64;
	delete m_pool32;
	delete m_pool16;
	delete m_pool8;
	delete m_pool4;
	delete m_stack;
	delete m_linear;
}
LinearAllocator* MemoryManager::get_linear_allocator() {
	return m_linear;
}
StackAllocator* MemoryManager::get_stack_allocator() {
	return m_stack;
}
PoolAllocator* MemoryManager::get_pool_allocator(size_t size) {
	// create different fucntion for each pool allocator if the switch case becomes a bottleneck (idk how it ever would be though)
	switch (size) {
	case 4:
		return m_pool4;
	case 8:
		return m_pool8;
	case 16:
		return m_pool16;
	case 32:
		return m_pool32;
	case 64:
		return m_pool64;
	case 128:
		return m_pool128;
	case 265:
		return m_pool256;
	default:
		assert(false);
		return nullptr;
	}
}

void* operator new (size_t bytes, size_t alignment, LinearAllocator* allocator) {
	return allocator->allocate(bytes, alignment);
}
void* operator new (size_t bytes, size_t alignment, StackAllocator* allocator) {
	return allocator->allocate(bytes, alignment);
}
void* operator new(size_t bytes, size_t alignment, PoolAllocator* _pool_allocator) {
	return _pool_allocator->allocate();
}
// allocate count number of contiguous objects aligned to the given alignment 
void* operator new (size_t bytes, size_t count, size_t alignment, LinearAllocator* allocator) {
	return allocator->allocate(bytes * count, alignment);
}
void* operator new (size_t bytes, size_t count, size_t alignment, StackAllocator* allocator) {
	return allocator->allocate(bytes * count, alignment);
}

void operator delete(void* ptr) noexcept {}
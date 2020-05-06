#pragma once
#include <atomic>
#include <assert.h>
#include "../Memory/MemoryManager.h"

template <typename T>
class Queue {
public:
	struct cell_t {
		std::atomic<size_t> m_cell_num;
		T m_data;
	};
	cell_t* const m_buffer;
	const size_t m_buffer_mask;
	std::atomic<size_t> m_pop_pos;
	std::atomic<size_t> m_push_pos;
	
public:
	//Queue(size_t size) : /*m_buffer(NEW_ARRAY(cell_t, size, memory_manager->get_linear_allocator())()),*/ m_buffer_mask(size - 1) {
	Queue(size_t size) : m_buffer(new cell_t[size]), m_buffer_mask(size - 1) {
		assert((size >= 2) && ((size & (m_buffer_mask)) == 0)); // provided size must be a power of two
		m_push_pos.store(0, std::memory_order_relaxed);
		m_pop_pos.store(0, std::memory_order_relaxed);
		for (size_t i = 0; i != size; ++i) {
			// operations can be in any order because initialization is single threaded.
			m_buffer[i].m_cell_num.store(i, std::memory_order_relaxed);
		}
	}
	~Queue() {
		// dont need to call the destructor on cell_t because it's all static
		free(m_buffer, memory_manager->get_linear_allocator());
	}
	Queue(Queue& q) = delete;
	void operator= (Queue& q) = delete;

	bool push(const T& data) {
		cell_t* cell; // create a pointer to where we want to store the data
		size_t push_pos = m_push_pos.load(std::memory_order_relaxed); // load the position we want to push to
		while (true) {
			cell = &m_buffer[push_pos & m_buffer_mask]; // make the pointer point position we can push to.
			size_t cell_num = cell->m_cell_num.load(std::memory_order_acquire); // get the sequence number of that cell we would like to push to
			intptr_t diff = static_cast<intptr_t>(cell_num) - static_cast<intptr_t>(push_pos);
			if (diff == 0) { // if the cell_num is the same as its pos then it's available
				// claim this cell as our own by incrementing the push position so we have free reign of this cell
				if (m_push_pos.compare_exchange_weak(push_pos, push_pos + 1, std::memory_order_relaxed)) {
					break; // weak because it must be able to fail if another thread beat us to this cell
				}
				else { // if a thread beat us to it, update the push_pos and try again
					push_pos = m_push_pos.load(std::memory_order_relaxed);
				}
			}
			else if (diff < 0) { // the queue is full
				return false;
			}
			else { // the cell was occupied, so load for this thread the new value of the push location and try again
				push_pos = m_push_pos.load(std::memory_order_relaxed);
			}
		}
		// increment the cell_num after setting the data so the cell can't be popped until we are fully done with it
		cell->m_data = data; // copy the data into the cell
		cell->m_cell_num.store(push_pos + 1, std::memory_order_release); // store the incremented position into the sequence number so that we know that we know if it's in use
		return true;
	}
	bool pop(T& data) {
		cell_t* cell;
		size_t pop_pos = m_pop_pos.load(std::memory_order_relaxed);
		while (true) {
			cell = &m_buffer[pop_pos & m_buffer_mask];
			size_t cell_num = cell->m_cell_num.load(std::memory_order_acquire);
			intptr_t diff = static_cast<intptr_t>(cell_num - 1) - static_cast<intptr_t>(pop_pos); // need to sub 1 because the stored cell_num of a used cell is one higher than it's position
			if (diff == 0) {
				if (m_pop_pos.compare_exchange_weak(pop_pos, pop_pos + 1, std::memory_order_relaxed)) {
					break;
				}
				else { // if a thread beat us ot it, update the pop_pos and try again
					pop_pos = m_pop_pos.load(std::memory_order_relaxed);
				}
			}
			else if (diff < 0) { // queue empty
				return false;
			}
			else {
				pop_pos = m_pop_pos.load(std::memory_order_relaxed);
			}
		}
		data = cell->m_data; // copy the data out of the cell
		cell->m_cell_num.store(pop_pos + m_buffer_mask + 1, std::memory_order_release);
		return true;
	}
};
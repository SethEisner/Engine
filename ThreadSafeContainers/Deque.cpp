#include "Deque.h"
//#include "../JobSystem/JobSystem.h"
//struct Job;
void Deque::push(Job* job) {
	// size_t b = m_bottom.fetch_add(1);
	// m_jobs[b & MASK] = job;
	long b = m_bottom;
	m_jobs[b & MASK] = job;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	m_bottom = b + 1;
}
Job* Deque::pop() {
	long b = m_bottom - 1;
	_InterlockedExchange(&m_bottom, b);
	long t = m_top;
	if (t <= b) {
		Job* job = m_jobs[b & MASK];
		if (t != b) {
			return job;
		}
		if (_InterlockedCompareExchange(&m_top, t + 1, t) != t) {
			job = nullptr;
		}
		m_bottom = t + 1;
		return job;
	}
	else {
		m_bottom = t;
		return nullptr;
	}

	// size_t b;// = m_bottom.fetch_sub(1) - 1; // get the old value of 1 and subtract it 
	// size_t t;// = m_top.load();
	// Job* job = nullptr;
	// while (true) {
	// 	b = m_bottom.load(); 
	// 	t = m_top.load();
	// 	//b = m_bottom.load() - 1;
	// 	if (t != b) { // the deque is not empty as of our snapshot of it
	// 		if (m_top.compare_exchange_weak(t, t + 1)) { // we successfully incremented t
	// 			b = m_bottom.fetch_sub(1) - 1;
	// 			job = m_jobs[b & MASK];
	// 			m_bottom.fetch_add(1);
	// 			return job;
	// 		}
	// 	}
	// 	break;
	// }
	// return job;
}
Job* Deque::steal() {
	long t = m_top;// std::memory_order_acquire); // ensure we read t before b
	std::atomic_thread_fence(std::memory_order_seq_cst);
	long b = m_bottom;// std::memory_order_relaxed);
	if (t < b) {
		Job* job = m_jobs[t & MASK];
		if (_InterlockedCompareExchange(&m_top, t + 1, t) != t) {
			return nullptr;
		}
		return job;
	}
	else {
		return nullptr;
	}
	// size_t b;
	// size_t t;
	// Job* job = nullptr;
	// while (true) {
	// 	b = m_bottom.load();
	// 	t = m_top.load();
	// 	if (t < b) {
	// 		if (m_top.compare_exchange_weak(t, t + 1)) {
	// 			job = m_jobs[t & MASK];
	// 			return job;
	// 		}
	// 	}
	// 	break;
	// }
	// return job;
}
/*
void Deque::push(T* job) {
	size_t b = m_bottom.fetch_add(1); // returns the old value
	// we are only thread that changes b because other threads only call steal which does not change b
	//*(m_jobs + (b & MASK)) = job;
	m_jobs[b & MASK] = job;
}
Job* Deque::steal() {
	// size_t t = m_top.load();
	// size_t b;
	// if (t < (b = m_bottom.load())) {
	// 	Job* job = m_jobs[t & MASK];
	// 	
	// }
	size_t t;
	size_t b;
	Job* job;
	while (true) { // keep trying until the queue is empty or we successfully get 
		t = m_top.load();
		b = m_bottom.load();
		if (t < b) {
			if (m_top.compare_exchange_weak(t, t + 1)) { // increment t starting from our expected value to ensure another thread didnt beat us to stealing a job
				break;
				job = m_jobs[t & MASK];
			}
		}
		else { // the queue is empty according to this thread's view of the memory
			return nullptr;
		}
	}
	return job;
}
Job* Deque::pop() {
	size_t b = m_bottom.fetch_sub(1);
	size_t t = m_top.load();
	Job* job;
	while (true) {
		if (t <= b) { // check that there is atleast one item is the queue
			job = m_jobs[b & MASK];
			if (t != b) { // there is more than one item in the queue so we can freely return the job
				return job;
			}
			if (m_top.compare_exchange_weak(t, t + 1)) {
				break;
			}
		}
		else {
			m_bottom.store(t);
			return nullptr;
		}
	}
	m_bottom.store(m_top.load() + 1);
}
*/
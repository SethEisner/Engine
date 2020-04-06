#pragma once

#include <atomic>
//#include "../JobSystem/JobSystem.h"

struct Job;

class Deque {
public:
	void push(Job*);
	Job* steal();
	Job* pop();
	
	Deque(size_t size) : m_jobs(new Job*[size]), MASK(size - 1u) {
	//Deque(size_t size) : m_jobs((Job**)malloc(sizeof(Job*)*size)), MASK(size - 1u) {
		m_top = 0;// .store(0);
		m_bottom = 0;// .store(0);
		for (size_t i = 0; i != size; ++i) {
			//m_jobs[i] = (Job*)malloc(sizeof(Job*));
			m_jobs[i] = (Job*)calloc(1, sizeof(Job*));
		}
		
	}
	~Deque() {
		// for (size_t i = 0; i != MASK + 1; ++i) {
		// 	delete m_jobs[i];
		// 	//	free(m_jobs[i]);
		// }
		//free(m_jobs);
		delete[] m_jobs;
	}
private:
	Job** m_jobs; // array of jobs pointers
	const unsigned long MASK;
	volatile long m_top;
	volatile long m_bottom;
	//std::atomic<size_t> m_top;
	//std::atomic<size_t> m_bottom;
};
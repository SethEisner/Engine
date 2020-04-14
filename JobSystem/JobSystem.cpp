#pragma once
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>
#include "JobSystem.h"
#include <string>
#include <windows.h>
//#include "../ThreadSafeContainers/Queue.h"
//#include "../ThreadSafeContainers/Deque.h"
// typedef void (*JobFunction) (Job*, const void*);


size_t g_lehmer64_state = 1;

uint64_t lehmer64() {
	g_lehmer64_state *= 0xda942042e4dd58b5;
	return g_lehmer64_state >> 64;
}


namespace JobSystem {
	void JobSystem::startup(uint8_t worker_count) {
		shutdown_flag = false;
		g_worker_count = worker_count + 1;
		m_workers = new Worker[g_worker_count];
		for (uint8_t i = 1; i != g_worker_count; ++i) {
			m_workers[i].m_thread = new std::thread(start_thread, i);
		}
	}
	void shutdown() {
		shutdown_flag = true;
		for (int i = 1; i != g_worker_count; ++i) {
			m_workers[i].m_thread->join();
		}
		delete[] m_workers;
	}
	void start_thread(uint8_t i) {
		id = i;
		SetThreadAffinityMask(GetCurrentThread(), 1u<<static_cast<uint64_t>(i));
		while (!shutdown_flag) {
 			Job* job = get_job();
			if (!is_empty_job(job)) execute_job(job);
		}
	}
	Job* create_job(JobFunction func) {
		Job* job = allocate_job();
		job->m_function = func;
		job->m_parent = nullptr;
		job->m_unfinished_jobs.store(1);
		return job;
	}
	Job* create_job(JobFunction func, const void* data) {
		Job* job = allocate_job();
		job->m_function = func;
		job->m_parent = nullptr;
		job->m_unfinished_jobs.store(1);
		memcpy(job->m_data, data, 44);
		return job;
	}
	Job* create_job_as_child(JobFunction func, Job* parent) {
		std::atomic_fetch_add(&parent->m_unfinished_jobs, 1); //increment the unfinished jobs count of the parent if we supply one
		Job* job = allocate_job();
		job->m_function = func;
		job->m_parent = parent;
		job->m_unfinished_jobs.store(1);
		return job;
	}
	Job* create_job_as_child(JobFunction func, Job* parent, const void* data) {
		std::atomic_fetch_add(&parent->m_unfinished_jobs, 1); //increment the unfinished jobs count of the parent if we supply one
		Job* job = allocate_job();
		job->m_function = func;
		job->m_parent = parent;
		job->m_unfinished_jobs.store(1);
		memcpy(job->m_data, data, 44);
		return job;
	}
	void finish(Job* job) {
		job->m_unfinished_jobs.fetch_sub(1);
		if (job->m_unfinished_jobs.load() == 0 && job->m_parent) finish(job->m_parent);
	}
	inline Job* allocate_job() {
		return &g_job_buffer[g_allocated_jobs++ & G_MAX_JOB_MASK];
	}
	inline Queue<Job*>* get_worker_thread_queue() {
		return m_workers[id].m_queue;
	}
	inline Queue<Job*>* get_worker_thread_queue(int i) {
		return m_workers[i].m_queue;
	}
	void queue_job(Job* job) {
		while (!get_worker_thread_queue()->push(job)) { // run jobs until there's space in the queue 
			Job* next = get_job();
			if (next) execute_job(next);
		}
	}
	inline bool job_completed(const Job* job) {
		return (job->m_unfinished_jobs.load() == 0);
	}
	void execute_job(Job* job) {
		(job->m_function)(job, job->m_data);
		finish(job);
	}
	inline bool is_empty_job(Job* job) {
		return (job == nullptr || job->m_unfinished_jobs.load() == 0);
	}
	Job* get_job() {
		Queue<Job*>* queue = get_worker_thread_queue();
		Job* job = nullptr;
		if (!queue->pop(job)) { // unsuccessfully popped a job
			uint32_t index = lehmer64() % g_worker_count;
			queue = get_worker_thread_queue(index);
			if (!queue->pop(job)) {
				std::this_thread::yield();
			}
		}
		return job;
	}
	void wait(const Job* job) {
		while (!job_completed(job)) { // do work while we wait
			Job* next = get_job();
			if (!is_empty_job(next)) execute_job(next);
		}
	}
}

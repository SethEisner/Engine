#pragma once
#include <atomic>
#include <mutex>
#include <memory>
/*
 push and pop to/from the bottom
 pop from the top


*/
template <class T>
class ThreadSafeBoundedDeque
{
	const int m_size;
	std::atomic<int> m_front;
	std::atomic<int> m_back;
	std::atomic<int> m_item_count;
	T* m_contents;

public:
	explicit ThreadSafeBoundedDeque(int size) : m_size(size), m_front(1), m_back(0), m_item_count(0) {
		m_contents = new T[size];
	}

	~ThreadSafeBoundedDeque() {
		delete[] m_contents;
	}

	// when pushing, change the index first so we don't overwrite. when popping, change index after to have it point to the new end.
	void push_back(T entry) {
		m_back++;
		m_back = m_back % m_size;
		m_contents[m_back] = entry;
	}

	T pop_back() {
		T res = m_contents[m_back--];
		if (m_back < 0) m_back = m_size - 1;
		m_item_count--;
		return res;
	}
	
	void push_front(T entry) {
		m_front--;
		if (m_front < 0) m_front = m_size - 1;
		m_contents[m_front] = entry;
		m_item_count++;
	}

	T pop_front() {
		T res = m_contents[m_front++];
		m_front = m_front % m_size;
		m_item_count--;
		return res;
	}
	
	T peek_front() {
		return m_contents[m_front];
	}
	T peek_back() {
		return m_contents[m_back];
	}


};


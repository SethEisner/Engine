#include "Timer.h"
#include <windows.h>

Timer::Timer() : m_seconds_per_count(0.0), m_delta_time(-1.0), m_base_time(0), m_paused_time(0), m_prev_time(0), m_curr_time(0), m_stopped(0) {
	int64_t counts_per_sec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&counts_per_sec));
	m_seconds_per_count = 1.0 / static_cast<double>(counts_per_sec);
}
float Timer::total_time() const {
	if (m_stopped) {
		return static_cast<float>(((m_stop_time - m_paused_time) - m_base_time) * m_seconds_per_count);
	}
	else{
		return static_cast<float>(((m_curr_time - m_paused_time) - m_base_time) * m_seconds_per_count);
	}
}
float Timer::delta_time() const {
	return static_cast<float>(m_delta_time);
}
void Timer::reset() {
	int64_t curr_time;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curr_time));
	m_base_time = curr_time;
	m_prev_time = curr_time;
	m_stop_time = 0;
	m_stopped = false;
}
void Timer::start() {
	int64_t start_time;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&start_time));
	if (m_stopped) {
		m_paused_time += (start_time - m_stop_time);
		m_prev_time = start_time;
		m_stop_time = 0;
		m_stopped = false;
	}
}
void Timer::stop() {
	if (!m_stopped) {
		int64_t curr_time;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curr_time));
		m_stop_time = curr_time;
		m_stopped = true;
	}
}
void Timer::tick() {
	if (m_stopped) {
		m_delta_time = 0.0;
		return;
	}
	int64_t curr_time;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curr_time));
	m_curr_time = curr_time;
	m_delta_time = (m_curr_time - m_prev_time) * m_seconds_per_count;
	m_prev_time = m_curr_time;
	if (m_delta_time < 0.0) m_delta_time = 0.0f;
}
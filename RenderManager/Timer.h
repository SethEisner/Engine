#pragma once
#include <stdint.h>
class Timer {
public:
    Timer();
    float total_time() const;
    float delta_time() const;
    void reset();
    void start();
    void stop();
    void tick();
private:
    double m_seconds_per_count; // hardware specific
    double m_delta_time;
    // stores hardware counts that can be converted to seconds
    // make int64_t so arithmetic is well defined
    int64_t m_base_time;
    int64_t m_paused_time;
    int64_t m_stop_time;
    int64_t m_prev_time;
    int64_t m_curr_time;
    bool m_stopped;
};
/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include <chrono>
#include <string>
#include <time.h>

using timespec_t = struct timespec;

class PersistRTC
{
public:
    PersistRTC();
    ~PersistRTC();

    [[nodiscard]] static timespec_t get_initial_timespec();

private:
    static timespec_t m_initial_timespec;
    std::chrono::steady_clock::time_point m_start_time;
};

void reset_rtc();
void forward_rtc();

void log_note(const std::string & kind, const std::string & message);

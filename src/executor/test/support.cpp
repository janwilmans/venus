/*
 * Copyright (c) 2025 Jan Wilmans
 */

// https://randombitsofuselessinformation.blogspot.com/2018/06/its-about-time-monotonic-time.html

#include "support.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <time.h>

#include <fmt/core.h>

timespec_t PersistRTC::m_initial_timespec = timespec_t();

PersistRTC::PersistRTC()
{
    clock_gettime(CLOCK_REALTIME, &m_initial_timespec);
    m_start_time = std::chrono::steady_clock::now();
}

PersistRTC::~PersistRTC()
{
    auto elased_seconds = std::chrono::duration_cast<std::chrono::seconds>(m_start_time - std::chrono::steady_clock::now());
    auto ts = PersistRTC::get_initial_timespec();
    ts.tv_sec += elased_seconds.count();
    clock_settime(CLOCK_REALTIME, &ts);
}

[[nodiscard]] timespec_t PersistRTC::get_initial_timespec()
{
    return PersistRTC::m_initial_timespec;
}

void reset_rtc()
{
    auto ts = PersistRTC::get_initial_timespec();
    if (clock_settime(CLOCK_REALTIME, &ts) != 0)
    {
        FAIL() << "This test needs root privileges to change the realtime clock";
    }
}

void forward_rtc()
{
    auto ts = PersistRTC::get_initial_timespec();
    ts.tv_sec += 3600; // ahead one hour
    clock_settime(CLOCK_REALTIME, &ts);
}


void log_note(const std::string & kind, const std::string & message)
{
    std::string text = fmt::format("\u001b[32m[{0:^10}] \u001b[33m{1}\u001b[0m\n", kind, message);
    fmt::print(stderr, text);
}

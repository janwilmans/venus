/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include <gtest/gtest.h>

#include "support.hpp"

// Note: warning! this test need to run as root to change your realtime clock
// As such, this is not a pure unittest and can change your realtime clock setting
// A best effort is made to restore it to the original setting at the end using class PersistRTC.

TEST(chrono, get_set_rtc)
{
    struct timespec ts;
    auto read_result = clock_gettime(CLOCK_REALTIME, &ts); // can we read the RTC clock
    if (read_result != 0)
    {
        perror("clock_gettime");
    }
    ASSERT_EQ(read_result, 0);

    auto write_result = clock_settime(CLOCK_REALTIME, &ts); // can we write the RTC clock
    if (write_result != 0)
    {
        perror("clock_settime");
    }
    ASSERT_EQ(write_result, 0);
}

int main(int argc, char ** argv)
{
    PersistRTC persistRTC; // restores the RTC to the original time when the test ends.

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

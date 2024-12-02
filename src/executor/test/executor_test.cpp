/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include <gtest/gtest.h>

#include <fmt/core.h>

TEST(test1, test1)
{
    ASSERT_TRUE(true);
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
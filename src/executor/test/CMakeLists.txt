add_executable(executor_test
  executor_test.cpp
)

target_link_libraries(executor_test
PRIVATE
  fmt::fmt
  #GTest::gmock
  GTest::gtest
  GTest::gtest_main
)
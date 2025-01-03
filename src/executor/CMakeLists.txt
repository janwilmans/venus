add_library(venus_executor_library
  src/executor.cpp
  src/scheduled_calls.cpp
  include/executor/synchronized_queue.hpp
)

target_include_directories(venus_executor_library PUBLIC
    include
)

add_library(venus::executor ALIAS venus_executor_library)

add_executable(executor_test
  test/executor_test.cpp
)

target_link_libraries(executor_test
PRIVATE
  fmt::fmt
  GTest::gtest
  GTest::gtest_main
  venus::executor
)

add_executable(chrono_test
  test/chrono_main_test.cpp
  test/support.cpp
  test/chono_rtc_test.cpp
  test/chono_timezone_test.cpp
)

target_link_libraries(chrono_test
PRIVATE
  fmt::fmt
  GTest::gtest
  GTest::gtest_main
)

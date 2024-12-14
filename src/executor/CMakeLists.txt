add_library(venus_executor_library
  src/executor.cpp
  include/executor/synchronized_queue.hpp
)

target_include_directories(venus_executor_library PUBLIC
    include
)

add_library(venus::executor ALIAS venus_executor_library)

add_executable(executor_test
  test/chono_test.cpp
  test/executor_test.cpp
)

target_link_libraries(executor_test
PRIVATE
  fmt::fmt
  GTest::gtest
  GTest::gtest_main
  venus::executor
)
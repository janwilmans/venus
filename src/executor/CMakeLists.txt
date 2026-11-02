add_library(venus_executor_library
  src/executor.cpp
  src/scheduled_calls.cpp
  include/executor/synchronized_queue.hpp
)

target_include_directories(venus_executor_library PUBLIC
    include
)

target_compile_features(venus_executor_library PUBLIC cxx_std_14)
target_link_libraries(venus_executor_library PUBLIC Threads::Threads)

add_library(venus::executor ALIAS venus_executor_library)

if (NOT VENUS_BUILD_TESTS)
  return()
endif()

add_executable(executor_test
  test/executor_test.cpp
  test/synchronized_queue_test.cpp
)

target_link_libraries(executor_test
PRIVATE
  GTest::gmock
  GTest::gtest
  GTest::gtest_main
  venus::executor
)

add_test(NAME executor_test COMMAND executor_test)
# a deadlock should fail the test, not hang the build
set_tests_properties(executor_test PROPERTIES TIMEOUT 120)

# exploratory tests of std::chrono behaviour when the realtime clock is changed,
# POSIX only, and not registered with ctest because they modify the system clock (requires root)
if (NOT WIN32)
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
endif()

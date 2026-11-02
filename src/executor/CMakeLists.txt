# header-only: no sources to compile, consumers only need the include directory, C++14 and threads
add_library(venus_executor_library INTERFACE)

target_include_directories(venus_executor_library INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/venus>
)

target_compile_features(venus_executor_library INTERFACE cxx_std_14)
target_link_libraries(venus_executor_library INTERFACE Threads::Threads)

set_target_properties(venus_executor_library PROPERTIES EXPORT_NAME executor)
add_library(venus::executor ALIAS venus_executor_library)

if (NOT VENUS_BUILD_TESTS)
  return()
endif()

add_executable(executor_test
  test/executor_test.cpp
  test/synchronized_queue_test.cpp
  test/task_test.cpp
  test/pool_executor_test.cpp
  test/dispatcher_test.cpp
  test/headers_test.cpp
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

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

# the single header include/venus.hpp is generated from the separate headers and committed, so it is available
# without a build (for example to include it from a URL in Compiler Explorer); never generated in a consumer's copy
if (VENUS_SINGLE_HEADER)
  file(GLOB_RECURSE venus_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/executor/*.hpp)
  set(venus_single_header ${CMAKE_CURRENT_SOURCE_DIR}/include/venus.hpp)
  add_custom_command(
    OUTPUT ${venus_single_header}
    COMMAND ${CMAKE_COMMAND} -DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include -DOUTPUT=${venus_single_header}
            -P ${PROJECT_SOURCE_DIR}/cmake/single_header.cmake
    DEPENDS ${venus_headers} ${PROJECT_SOURCE_DIR}/cmake/single_header.cmake
    COMMENT "Generating the single header venus.hpp"
  )
  add_custom_target(venus_single_header ALL DEPENDS ${venus_single_header})
endif()

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

# googletest's TEST() uses [[maybe_unused]] when the compiler supports it, Clang warns about that in C++14 mode
# (the venus headers themselves are checked without this exception, by the examples and tools/compiler_explorer.py)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(executor_test PRIVATE -Wno-c++17-attribute-extensions)
endif()

add_test(NAME executor_test COMMAND executor_test)

# uses only the single header, in a separate executable because it defines the same classes as the separate headers
add_executable(single_header_test test/single_header_test.cpp)
target_link_libraries(single_header_test PRIVATE venus::executor)
if (VENUS_SINGLE_HEADER)
  add_dependencies(single_header_test venus_single_header)
endif()
add_test(NAME single_header_test COMMAND single_header_test)
set_tests_properties(single_header_test PROPERTIES TIMEOUT 60)
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

  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(chrono_test PRIVATE -Wno-c++17-attribute-extensions)
  endif()
endif()

# A project that uses venus, to check the three ways of consuming it:
#   -DVENUS_CONSUME_MODE=add_subdirectory  (default)
#   -DVENUS_CONSUME_MODE=fetchcontent      -DVENUS_GIT_TAG=<commit>
#   -DVENUS_CONSUME_MODE=find_package      -DCMAKE_PREFIX_PATH=<venus install prefix>
cmake_minimum_required(VERSION 3.18)
project(venus_consumer CXX)

set(VENUS_CONSUME_MODE "add_subdirectory" CACHE STRING "add_subdirectory, fetchcontent or find_package")
set(VENUS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "venus source directory (a git repository for fetchcontent)")

if (VENUS_CONSUME_MODE STREQUAL "add_subdirectory")
  add_subdirectory(${VENUS_SOURCE_DIR} venus)
elseif (VENUS_CONSUME_MODE STREQUAL "fetchcontent")
  include(FetchContent)
  FetchContent_Declare(venus
    GIT_REPOSITORY ${VENUS_SOURCE_DIR}
    GIT_TAG ${VENUS_GIT_TAG}
  )
  FetchContent_MakeAvailable(venus)
elseif (VENUS_CONSUME_MODE STREQUAL "find_package")
  find_package(venus 0.1 REQUIRED CONFIG)
else()
  message(FATAL_ERROR "unknown VENUS_CONSUME_MODE '${VENUS_CONSUME_MODE}'")
endif()

if (TARGET GTest::gtest OR TARGET fmt::fmt)
  message(FATAL_ERROR "venus fetched its test dependencies while used as a dependency")
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE venus::executor)

enable_testing()
add_test(NAME consumer COMMAND consumer)

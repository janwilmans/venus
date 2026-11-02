cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(venus CXX)

# defaults are ON when venus is the top-level project, OFF when consumed through add_subdirectory/FetchContent
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(VENUS_IS_TOP_LEVEL ON)
else()
  set(VENUS_IS_TOP_LEVEL OFF)
endif()

option(VENUS_BUILD_TESTS "Build the venus tests (fetches googletest)" ${VENUS_IS_TOP_LEVEL})
option(VENUS_WARNINGS "Compile venus with extra warnings (and -Werror outside CI)" ${VENUS_IS_TOP_LEVEL})
set(VENUS_SANITIZE "" CACHE STRING "Sanitizers to enable, for example 'address,undefined' or 'thread'")

if (VENUS_IS_TOP_LEVEL)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_package(Threads REQUIRED)

if (VENUS_BUILD_TESTS)
  enable_testing()
  include(CMakeLists.googletest.cmake)
  if (NOT WIN32)
    # fmt is only used by the POSIX-only chrono exploration tests
    include(CMakeLists.fmt.cmake)
  endif()
endif()

if (VENUS_WARNINGS)
  include(CMakeLists.warnings.cmake)
endif()

if (VENUS_SANITIZE)
  if (MSVC)
    add_compile_options(/fsanitize=${VENUS_SANITIZE})
  else()
    add_compile_options(-fsanitize=${VENUS_SANITIZE} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${VENUS_SANITIZE})
  endif()
endif()

add_subdirectory(executor)

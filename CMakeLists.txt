cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(venus VERSION 0.1.0 LANGUAGES CXX)

# defaults are ON when venus is the top-level project, OFF when consumed through add_subdirectory/FetchContent
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(VENUS_IS_TOP_LEVEL ON)
else()
  set(VENUS_IS_TOP_LEVEL OFF)
endif()

option(VENUS_BUILD_TESTS "Build the venus tests (fetches googletest)" ${VENUS_IS_TOP_LEVEL})
option(VENUS_WARNINGS "Compile venus with extra warnings (and -Werror outside CI)" ${VENUS_IS_TOP_LEVEL})
option(VENUS_SINGLE_HEADER "Generate the single header src/executor/include/venus.hpp during the build" ${VENUS_IS_TOP_LEVEL})
option(VENUS_INSTALL "Generate the install target, for use with find_package(venus)" ${VENUS_IS_TOP_LEVEL})
set(VENUS_SANITIZE "" CACHE STRING "Sanitizers to enable, for example 'address,undefined' or 'thread'")

if (VENUS_IS_TOP_LEVEL)
  # C++14 by default, can be overridden with -DCMAKE_CXX_STANDARD=17 to test newer standards
  if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
  endif()
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_package(Threads REQUIRED)

if (VENUS_BUILD_TESTS)
  enable_testing()
  include(cmake/googletest.cmake)
  if (NOT WIN32)
    # fmt is only used by the POSIX-only chrono exploration tests
    include(cmake/fmt.cmake)
  endif()
endif()

if (VENUS_WARNINGS)
  include(cmake/warnings.cmake)
endif()

if (VENUS_SANITIZE)
  if (MSVC)
    add_compile_options(/fsanitize=${VENUS_SANITIZE})
  else()
    add_compile_options(-fsanitize=${VENUS_SANITIZE} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${VENUS_SANITIZE})
  endif()
endif()

# defines CMAKE_INSTALL_INCLUDEDIR, used by the install interface of the library target
include(GNUInstallDirs)

add_subdirectory(src/executor)

if (VENUS_BUILD_TESTS)
  add_subdirectory(examples)
endif()

if (VENUS_INSTALL)
  include(CMakePackageConfigHelpers)

  # headers are installed in <prefix>/include/venus/executor/, so '#include "executor/executor.hpp"' keeps working
  # without adding a generic 'executor' directory to <prefix>/include
  install(TARGETS venus_executor_library EXPORT venus-targets)
  install(DIRECTORY src/executor/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/venus)
  install(EXPORT venus-targets NAMESPACE venus:: DESTINATION ${CMAKE_INSTALL_DATADIR}/venus)

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/venus-config.cmake [=[
include(CMakeFindDependencyMacro)
find_dependency(Threads)
include("${CMAKE_CURRENT_LIST_DIR}/venus-targets.cmake")
]=])
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/venus-config-version.cmake
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/venus-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/venus-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/venus
  )
endif()

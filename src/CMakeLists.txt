project(venus)

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

enable_testing()
include(CTest)

#
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CMakeLists.fmt.cmake)
include(CMakeLists.googletest.cmake)

find_package(Threads REQUIRED)

include(CMakeLists.warnings.cmake)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

#add_compile_options(-fsanitize=thread -g -O1)
#add_link_options(-fsanitize=thread)

add_subdirectory(executor)

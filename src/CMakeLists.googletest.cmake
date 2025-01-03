if (NOT BUILD_IN_TREE)

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_VERSION LESS 5)
      set(OLDER_GCC 1)
    endif()
  endif()

  include(FetchContent)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  if (OLDER_GCC)

    message(STATUS "Building ${PROJECT_NAME} as standalone project, fetch GoogleTest release-1.8.1")
    FetchContent_Declare(googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.8.1
    )
    FetchContent_MakeAvailable(googletest)

    # older googletest did not have these aliases
    add_library(GTest::gtest ALIAS gtest)
    add_library(GTest::gtest_main ALIAS gtest_main)
    add_library(GTest::gmock ALIAS gmock)
    add_library(GTest::gmock_main ALIAS gmock_main)
  else()

    message(STATUS "Building ${PROJECT_NAME} as standalone project, fetch GoogleTest v1.15.2")
    FetchContent_Declare(googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)
  endif()
endif()

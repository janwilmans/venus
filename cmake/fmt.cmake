if (NOT BUILD_IN_TREE)

    message(STATUS "Building ${PROJECT_NAME} as standalone project, fetch FMT")

    include(FetchContent)

    message(STATUS "Fetch content for FMT 11.1.1")
    FetchContent_Declare(fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG 11.1.1
    )
    FetchContent_MakeAvailable(fmt)

target_compile_options(fmt PUBLIC -Wno-sign-conversion -Wno-shadow )

# workaround for {fmt} on gcc >= 13
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(fmt PUBLIC -Wno-logical-op)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
      target_compile_options(fmt PUBLIC -Wno-dangling-reference)
    endif()
else()
  target_compile_options(fmt PUBLIC -Wno-switch-enum)

endif()

endif()

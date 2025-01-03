message("  CMAKE_CXX_COMPILER_ID:      '${CMAKE_CXX_COMPILER_ID}'")
message("  CMAKE_CXX_COMPILER:         '${CMAKE_CXX_COMPILER}'")
message("  CMAKE_CXX_FRONTEND_VARIANT: '${CMAKE_CXX_FRONTEND_VARIANT}'")

if (CMAKE_CXX_COMPILER MATCHES ".*c..-analyzer")
  message("clang-analyser reports itself as a GNU compiler but with its own version, this will mess up the compiler detection below")
  message(FATAL_ERROR "you are building for clang-analyser (scan-build) but did not add -DBUILD_WARNINGS_AS_ERRORS=OFF")
endif()

# extra warnings from https://github.com/lefticus/cpp_starter_project/blob/master/cmake/CompilerWarnings.cmake
# -Wdouble-promotion # not sure when it is a problem or what to do instead?

add_compile_options(
  -Wall
  -Wextra
  -pedantic
  -Wcast-align
  -Wformat=2
  -Wmissing-include-dirs
  -Wswitch-enum
  -Wunused
  -Wunused-variable
  -Wunused-parameter
  -Wvla
  -Wsign-conversion
  -Wconversion
  -Wno-attributes
  $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
)

# No -Werror on the CI build, because we let the compiler report _all_ warnings,
# and the validation/tr_regression.py script will fail the build if NEW warnings are introduced
if (NOT DEFINED ENV{CI_SERVER})
  message("Adding options to locally compile _with_ -WError and colored messages")
  add_compile_options(-Werror)
  add_compile_options(-fdiagnostics-color=auto)

  # We allow these warnings locally, because they are very inconvenient to get as errors
  # during development, where variables and parameters may be in flux.
  add_compile_options(
    -Wno-error=unused
    -Wno-error=unused-variable
    -Wno-error=unused-parameter
  )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message("Adding extra GCC only compilation options")
    add_compile_options(
      -Wframe-larger-than=1000000
      -Wlogical-op
    )

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9)
      message("Adding GCC > 9 warnings")
      add_compile_options(
        -Wnull-dereference
        -Wmisleading-indentation
        -Wduplicated-branches
        -Wduplicated-cond
        -Wformat-overflow=2
        -Wshadow

        # Warnings that are not allowed in C compilation units
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=suggest-override>
        $<$<COMPILE_LANGUAGE:CXX>:-Wvolatile>
      )
    endif()

    # Some system headers (and fmt) produce warnings we cannot solve
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
      message("Adding modern GCC exceptions")
      add_compile_options(
        -Wno-array-bounds
        -Wno-stringop-overflow
        -Wno-dangling-reference
        -Wno-c++17-attribute-extensions
      )
    endif()


endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
      -Wshadow
      -Wno-c++17-attribute-extensions
      )
endif()

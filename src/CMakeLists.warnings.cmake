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
  -Wmisleading-indentation
  -Wmissing-include-dirs
  -Wnull-dereference
  -Wswitch-enum
  -Wunused
  -Wunused-variable
  -Wunused-parameter
  -Wvla
  -Wshadow
  -Wsign-conversion
  -Wconversion
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
      -Wduplicated-cond
      -Wframe-larger-than=1000000
      -Wlogical-op
      -Wduplicated-branches
      -Wformat-overflow=2
      # Warnings that are not allowed in C compilation units
      $<$<COMPILE_LANGUAGE:CXX>:-Wvolatile>
      $<$<COMPILE_LANGUAGE:CXX>:-Werror=suggest-override>
    )

    # We know that system headers (and fmt) produce bogus warnings with array-bounds;
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
      message("Adding GCC-13 exceptions")
      add_compile_options(
        -Wno-array-bounds
        -Wno-stringop-overflow
      )
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message("Adding Clang suppressions")
  add_compile_options(
      -Wcomma
     # warnings that are on, but not completely solved
      -Wno-error=cast-align
      -Wno-error=deprecated-enum-enum-conversion
      -Wno-error=deprecated-declarations
      -Wno-error=gnu-zero-variadic-macro-arguments
      -Wno-error=format-nonliteral
      # this very noisy and these warnings are coming from third-party headers
      -Wno-gnu-zero-variadic-macro-arguments
      -Wno-gnu-line-marker
  )

  add_link_options(-no-pie)
endif()

if (SQRREPORT)
  message("Adding extra SQR warnings")
  add_compile_options(
      -Wno-error
      -Weverything
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-documentation
      -Wno-shadow-field-in-constructor
  )
endif()

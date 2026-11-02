/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

// VENUS_NODISCARD is a C++17 attribute, in C++14 mode compilers warn about it (GCC: -Wattributes,
// Clang: -Wc++17-attribute-extensions), these warnings would show up in the builds of everyone that uses venus.
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define VENUS_NODISCARD [[nodiscard]]
#else
#define VENUS_NODISCARD
#endif

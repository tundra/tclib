//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _STDC
#define _STDC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Convert the compiler-defined macros into simpler ones that express the
// differences we're interested in.
#ifdef _MSC_VER
#  define IS_MSVC 1
#else
#  define IS_GCC 1
#endif

#ifdef __MACH__
#  define IS_MACH
#endif

// Include custom headers for each toolchain.
#ifdef IS_MSVC
#  include "stdc-msvc.h"
#else // !IS_MSVC
#  include "stdc-posix.h"
#endif

// Define some expression macros for small pieces of platform-dependent code.
#ifdef IS_MSVC
#  define IF_MSVC(T, E) T
#  define IF_GCC(T, E) E
#else
#  define IF_MSVC(T, E) E
#  define IF_GCC(T, E) T
#endif

#ifndef PRIi64
#  define PRIi64 "li"
#endif

#ifndef PRIx64
#  define PRIx64 "lx"
#endif

#ifdef IS_MACH
#  define IF_MACH(T, E) T
#else
#  define IF_MACH(T, E) E
#endif

// Define some expression macros for wordsize dependent code. The IS_..._BIT
// macros should be set in some platform dependent way above.
#ifdef IS_32_BIT
#  define IF_32_BIT(T, F) T
#  define IF_64_BIT(T, F) F
#  define WORD_SIZE 4
#else
#  define IF_32_BIT(T, F) F
#  define IF_64_BIT(T, F) T
#  define WORD_SIZE 8
#endif

// Includes of C headers from C++ files should be surrounded by these macros to
// ensure that they're linked appropriately.
#ifdef IS_GCC
#  if defined(__cplusplus)
#    define BEGIN_C_INCLUDES extern "C" {
#    define END_C_INCLUDES }
#  else
#    define BEGIN_C_INCLUDES
#    define END_C_INCLUDES
#  endif
#else
   // On windows everything gets compiled as C++ so there's no need to handle
   // C includes differently.
#  define BEGIN_C_INCLUDES
#  define END_C_INCLUDES
#endif

// Ensures that the compiler knows that the expression is used but doesn't cause
// it to be executed. The 'if (false)' ensures that the code is not run, the
// 'do while (false)' ensures that the macro doesn't leave a potential dangling
// else ambiguity.
#define USE(E) do { if (false) { E; } } while (false)

// Shorthand for bytes.
typedef unsigned char byte_t;

// Byte-size memory address (so addition increments by one byte at a time).
typedef byte_t *address_t;

// Integer datatype large enough to allow address arithmetic.
typedef size_t address_arith_t;

// Evaluates the arguments and joins them together as strings.
#define JOIN3(A, B, C) A##_##B##_##C
#define JOIN2(A, B) A##_##B

#endif // _STDC

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
#define IS_MSVC 1
#else
#define IS_GCC 1
#endif

// Include custom headers for each toolchain.
#ifdef IS_MSVC
#include "stdc-msvc.h"
#else // !IS_MSVC
#include "stdc-posix.h"
#endif

// Define some expression macros for small pieces of platform-dependent code.
#ifdef IS_MSVC
#define IF_MSVC(T, E) T
#define IF_GCC(T, E) E
#else
#define IF_MSVC(T, E) E
#define IF_GCC(T, E) T
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

#endif // _STDC

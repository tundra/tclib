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

// Convenience macros.
#define ONLY_MSVC(E) IF_MSVC(E, )
#define UNLESS_MSVC(E) IF_MSVC(, E)
#define kIsMsvc IF_MSVC(true, false)
#define ONLY_GCC(E) IF_GCC(E, )
#define UNLESS_GCC(E) IF_GCC(, E)
#define kIsGcc IF_GCC(true, false)

// The debug mode macro is set depending on how code is generated, whether it is
// highly optimized and debug symbols are retained, so you should very rarely,
// ideally never, need to depend on it. It is orthogonal to whether checks or
// indeed expensive checks are enabled.
#ifdef DEBUG_CODEGEN
#  define IS_DEBUG_CODEGEN
#endif

#ifdef IS_DEBUG_CODEGEN
#  define IF_DEBUG_CODEGEN(T, E) T
#else
#  define IF_DEBUG_CODEGEN(T, E) E
#endif


#define ONLY_DEBUG_CODEGEN(E) IF_DEBUG_CODEGEN(E, )
#define UNLESS_DEBUG_CODEGEN(E) IF_DEBUG_CODEGEN(, E)
#define kIsDebugCodegen IF_DEBUG_CODEGEN(true, false)

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

#ifdef ENABLE_CHECKS
#  define IF_CHECKS(T, F) T
#else
#  define IF_CHECKS(T, F) F
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

#define ONLY_32_BIT(E) IF_32_BIT(E, )
#define UNLESS_32_BIT(E) IF_32_BIT(, E)
#define kIs32Bit IF_32_BIT(true, false)
#define ONLY_64_BIT(E) IF_64_BIT(E, )
#define UNLESS_64_BIT(E) IF_64_BIT(, E)
#define kIs64Bit IF_64_BIT(true, false)

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

#ifdef FAIL_ON_DEVUTILS
#  define IF_ALLOW_DEVUTILS(T, F) F
#else
#  define ALLOW_DEVUTILS
#  define IF_ALLOW_DEVUTILS(T, F) T
#endif

#define ONLY_ALLOW_DEVUTILS(E) IF_ALLOW_DEVUTILS(E, )
#define UNLESS_ALLOW_DEVUTILS(E) IF_ALLOW_DEVUTILS(, E)
#define kAllowDevutils IF_ALLOW_DEVUTILS(true, false)

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
#define JOIN5(A, B, C, D, E) A##_##B##_##C##_##D##_##E
#define JOIN4(A, B, C, D) A##_##B##_##C##_##D
#define JOIN3(A, B, C) A##_##B##_##C
#define JOIN2(A, B) A##_##B

// Macro argument that indicates true.
#define X(T, F) T

// Macro argument that indicates false.
#define _(T, F) F

#endif // _STDC

//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Surely there's a better way to not make the stl headers blow up? But this
// does seem to work.
#define _HAS_EXCEPTIONS 0

// Rename the built-in types to the names expected in the code base. These
// should be compatible with how those types are defined in stdint.h in case
// this file gets included along with that.
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#if defined(_M_X64) || defined(__amd64__)
#  define IS_64_BIT 1
#else
#  define IS_32_BIT 1
#endif

// Versions of vc before 2013 don't have va_copy but this appears to work for
// them.
#if _MSC_VER < 1800
#  define va_copy(d,s) ((d) = (s))
#endif

// See stdc-posix.h for why this is lower case.
#define always_inline __forceinline

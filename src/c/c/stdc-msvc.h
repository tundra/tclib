//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Surely there's a better way to not make the stl headers blow up? But this
// does seem to work.
#define _HAS_EXCEPTIONS 0

// Rename the built-in types to the names expected in the code base.
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

// See http://blogs.msdn.com/b/oldnewthing/archive/2006/09/06/742710.aspx.
#ifdef _M_IX86
#  define IS_64_BIT 1
#else
#  define IS_32_BIT 1
#endif

// Windows doesn't have va_copy but this appears to work.
#define va_copy(d,s) ((d) = (s))

// See stdc-posix.h for why this is lower case.
#define always_inline __forceinline

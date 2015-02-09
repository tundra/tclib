//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __SIZEOF_POINTER__
#  if __SIZEOF_POINTER__ == 4
#    define IS_32_BIT
#  elif __SIZEOF_POINTER__ == 8
#    define IS_64_BIT
#  else
#    error "Unexpected pointer size."
#  endif
#else
#  error "Can't determine the pointer size."
#endif

// You might reasonably want this to be ALWAYS_INLINE but that just gets soooo
// shouty and it looks bad mixed with other lower-case modifiers.
#ifdef IS_GCC
#  define always_inline __attribute__((always_inline)) inline
#else
#  define always_inline inline
#endif

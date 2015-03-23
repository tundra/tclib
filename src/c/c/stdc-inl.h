//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Platform-specific inline functions and macros.

#ifndef _STDC_INL
#define _STDC_INL

#include <stdarg.h>

#ifdef IS_MSVC
#include "stdc-msvc-inl.h"
#endif


// Passing va_lists by reference works differently depending on platform. On
// windows passing a pointer to the va_list works, on linux (the ones I've tried
// at least) you can't get a pointer and need to pass it by value.
#if defined(IS_MSVC) || defined(IS_32_BIT)
typedef va_list *va_list_ref_t;
#  define VA_LIST_REF(argp) (&(argp))
#  define VA_LIST_DEREF(argp_ref) (*(argp_ref))
#else
typedef va_list va_list_ref_t;
#  define VA_LIST_REF(argp) (argp)
#  define VA_LIST_DEREF(argp_ref) (argp_ref)
#endif

#endif // _STDC_INL

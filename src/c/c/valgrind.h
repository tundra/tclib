//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_C_VALGRIND_H
#define _TCLIB_C_VALGRIND_H

#include "stdc.h"

#ifdef IS_GCC
#  include "../../third_party/c/valgrind/valgrind.h"
#else
#  define VALGRIND_DISCARD_TRANSLATIONS(addr, size)
#endif

#endif // _TCLIB_C_VALGRIND_H

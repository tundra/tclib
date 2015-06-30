//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/atomic.h"
END_C_INCLUDES

int32_t atomic_int32_get(atomic_int32_t *value) {
  return value->value;
}

atomic_int32_t atomic_int32_new(int32_t value) {
  atomic_int32_t result = {value};
  return result;
}

#ifdef IS_GCC
#  include "atomic-posix.c"
#endif

#ifdef IS_MSVC
#  include "atomic-msvc.c"
#endif

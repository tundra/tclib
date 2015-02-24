//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_ALLOC_HH
#define _TCLIB_UTILS_ALLOC_HH

#include "c/stdc.h"
#include "c/stdnew.hh"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
END_C_INCLUDES

namespace tclib {

// A marker that can be passed to placement new to indicate that the allocation
// should be done using the default allocator from the allocator framework.
typedef enum {
  kDefaultAlloc = 0
} default_alloc_marker_t;

// Delete a concrete instance of T, that is, an instance of T and not a subtype
// or at least one where the subtype does not add additional state beyond what
// is in T.
template <typename T>
void default_delete_concrete(T *ptr) {
  ptr->~T();
  allocator_default_free(new_memory_block(ptr, sizeof(T)));
}

} // namespace tclib

// Allocator that uses the default allocator from the C alloc framework. Note
// that this discards information about the amount allocated which we need on
// deallocation so you need to keep track of that manually somehow. But we want
// to keep track of that in any case since we want to exercise explicit control
// over how much memory gets allocated from the system.
inline void *operator new(size_t size, tclib::default_alloc_marker_t) {
  return allocator_default_malloc(size).memory;
}

#endif // _TCLIB_UTILS_ALLOC_HH

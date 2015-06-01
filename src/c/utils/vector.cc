//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

#include "c/stdvector.hh"

BEGIN_C_INCLUDES
#include "utils/vector.h"
END_C_INCLUDES

void voidp_vector_init(voidp_vector_t *vect) {
  vect->delegate_ = new std::vector<void*>();
}

void voidp_vector_dispose(voidp_vector_t *vect) {
  delete static_cast<std::vector<void*>*>(vect->delegate_);
  vect->delegate_ = NULL;
}

// Push an element at the end of the given vector.
void voidp_vector_push_back(voidp_vector_t *vect, void *elm) {
  static_cast<std::vector<void*>*>(vect->delegate_)->push_back(elm);
}

// Returns the number of elements in the given vector.
size_t voidp_vector_size(voidp_vector_t *vect) {
  return static_cast<std::vector<void*>*>(vect->delegate_)->size();
}

// Returns the index'th element of the given vector.
void *voidp_vector_get(voidp_vector_t *vect, size_t index) {
  return static_cast<std::vector<void*>*>(vect->delegate_)->at(index);
}

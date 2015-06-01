//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_C_VECTOR
#define _TCLIB_C_VECTOR

// A C port of the standard C++ vector type with void* elements.
typedef struct {
  void *delegate_;
} voidp_vector_t;

// Initialize a new void* vector.
void voidp_vector_init(voidp_vector_t *vect);

// Release any resources held by the given void* vector.
void voidp_vector_dispose(voidp_vector_t *vect);

// Push an element at the end of the given vector.
void voidp_vector_push_back(voidp_vector_t *vect, void *elm);

// Returns the number of elements in the given vector.
size_t voidp_vector_size(voidp_vector_t *vect);

// Returns the index'th element of the given vector.
void *voidp_vector_get(voidp_vector_t *vect, size_t index);

#endif // _TCLIB_C_VECTOR

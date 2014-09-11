//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_ALLOC_H
#define _TCLIB_ALLOC_H

#include "stdc.h"

// A block of memory as returned from an allocator. Bundling the length with the
// memory allows us to check how much memory is live at any given time.
typedef struct {
  // The actual memory.
  void *memory;
  // The number of bytes of memory (minus any overhead added by the allocator).
  size_t size;
} memory_block_t;

// Returns true iff the given block is empty, say because allocation failed.
bool memory_block_is_empty(memory_block_t block);

// Resets the given block to the empty state.
memory_block_t memory_block_empty();

// Creates a new memory block with the given contents. Be sure to note that this
// doesn't allocate anything, just bundles previously allocated memory into a
// struct.
memory_block_t new_memory_block(void *memory, size_t size);

// An allocator encapsulates a source of memory from the system.
typedef struct {
  // Function to call to do allocation.
  memory_block_t (*malloc)(void *data, size_t size);
  // Function to call to dispose memory. Note: the memory to deallocate is
  // the second arguments.
  void (*free)(void *data, memory_block_t memory);
  // Extra data that can be used by the alloc/dealloc functions.
  void *data;
} allocator_t;

// Initializes the given allocator to be the system allocator, using malloc and
// free.
void init_system_allocator(allocator_t *alloc);

// Allocates a block of memory using the given allocator.
memory_block_t allocator_malloc(allocator_t *alloc, size_t size);

// Allocates the specified amount of memory using the default allocator.
memory_block_t allocator_default_malloc(size_t size);

// Frees the given block of memory using the default allocator.
void allocator_default_free(memory_block_t block);

// Frees a block of memory using the given allocator.
void allocator_free(allocator_t *alloc, memory_block_t memory);

// Returns the current default allocator. If none has been explicitly set this
// will be the system allocator.
allocator_t *allocator_get_default();

// Sets the default allocator, returning the previous value.
allocator_t *allocator_set_default(allocator_t *value);

#endif // _TCLIB_ALLOC_H

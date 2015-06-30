//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_ALLOC_H
#define _TCLIB_ALLOC_H

#include "c/stdc.h"

// A block of memory as returned from an allocator. Bundling the length with the
// memory allows us to check how much memory is live at any given time.
typedef struct {
  // The actual memory.
  void *start;
  // The number of bytes of memory (minus any overhead added by the allocator).
  size_t size;
} blob_t;

// Returns true iff the given block is empty, say because allocation failed.
bool blob_is_empty(blob_t block);

// Resets the given block to the empty state.
blob_t blob_empty();

// Creates a new memory block with the given contents. Be sure to note that this
// doesn't allocate anything, just bundles previously allocated memory into a
// struct.
blob_t blob_new(void *start, size_t size);

// Fills this memory block's data with the given value.
void blob_fill(blob_t block, byte_t value);

// Write the contents of the source blob into the destination.
void blob_copy_to(blob_t src, blob_t dest);

// Given a struct by value, fills the memory occupied by the struct with zeroes.
#define struct_zero_fill(V) blob_fill(blob_new(&(V), sizeof(V)), 0)

// An allocator encapsulates a source of memory from the system.
typedef struct allocator_t {
  // Function to call to do allocation.
  blob_t (*malloc)(struct allocator_t *self, size_t size);
  // Function to call to dispose memory. Note: the memory to deallocate is
  // the second arguments.
  void (*free)(struct allocator_t *self, blob_t memory);
} allocator_t;

// Returns an allocator that uses system malloc/free.
allocator_t allocator_system();

// Allocates a block of memory using the given allocator.
blob_t allocator_malloc(allocator_t *alloc, size_t size);

// Allocates the specified amount of memory using the default allocator.
blob_t allocator_default_malloc(size_t size);

// Allocates a struct of the given type using the current default allocator. If
// allocation fails NULL is returned. The result must be deallocated using
// allocator_default_free_struct with the exact same type.
#define allocator_default_malloc_struct(T) ((T*) allocator_default_malloc(sizeof(T)).start)

// Frees the given block of memory using the default allocator.
void allocator_default_free(blob_t block);

// Frees a struct of the given type allocated with allocator_default_malloc.
#define allocator_default_free_struct(T, v) allocator_default_free(blob_new((v), sizeof(T)))

// Frees a block of memory using the given allocator.
void allocator_free(allocator_t *alloc, blob_t memory);

// Returns the current default allocator. If none has been explicitly set this
// will be the system allocator.
allocator_t *allocator_get_default();

// Sets the default allocator, returning the previous value.
allocator_t *allocator_set_default(allocator_t *value);

// An allocator that keeps track of the total amount allocated and limits how
// much allocation it will allow.
typedef struct {
  allocator_t header;
  // The default allocator this one is replacing.
  allocator_t *outer;
  // How much memory to allow in total.
  size_t limit;
  // The total amount of live memory.
  size_t live_memory;
  // Total number of blocks allocated.
  size_t live_blocks;
  // Has this allocator issued any warnings?
  bool has_warned;
} limited_allocator_t;

// Initializes the given allocator with the given limit and installs it as the
// default.
void limited_allocator_install(limited_allocator_t *alloc, size_t limit);

// Uninstalls the given allocator (which must be the current default) and
// restores the previous allocator, whatever it was. Returns true iff no memory
// was leaked.
bool limited_allocator_uninstall(limited_allocator_t *alloc);

// How many buckets do we divide allocations into by fingerprint?
#define kAllocFingerprintBuckets 257

// A fingerprinting allocator computes a fingerprint for each allocation and
// matches up the fingerprint of allocations and frees, reporting errors if they
// don't match. Similar to the limited allocator but may narrow down the site
// of incorrect allocations/frees.
typedef struct {
  allocator_t header;
  // The default allocator this one is replacing.
  allocator_t *outer;
  // How many blocks of memory have been allocated for a given fingerprint?
  size_t blocks[kAllocFingerprintBuckets];
  // How many bytes of memory have been allocated for a given fingerprint?
  size_t bytes[kAllocFingerprintBuckets];
  // Has this allocator issued any warnings?
  bool has_warned;
} fingerprinting_allocator_t;

// Initializes the given allocator and installs it as the default.
void fingerprinting_allocator_install(fingerprinting_allocator_t *alloc);

// Uninstalls the given allocator, which must be the current default, and
// restores the previous one.
bool fingerprinting_allocator_uninstall(fingerprinting_allocator_t *alloc);

#endif // _TCLIB_ALLOC_H

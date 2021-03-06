//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_ALLOC_H
#define _TCLIB_ALLOC_H

#include "c/stdc.h"

#include "sync/atomic.h"
#include "utils/blob.h"
#include "utils/check.h"

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

// Allocates an array of structs of the given type and length. If allocation
// fails NULL is returned. The result must be deallocated using
// allocator_default_free_structs with the exact same type and length.
#define allocator_default_malloc_structs(T, N) ((T*) allocator_default_malloc((N) * sizeof(T)).start)

// Frees the given block of memory using the default allocator.
void allocator_default_free(blob_t block);

// Frees a struct of the given type allocated with allocator_default_malloc_struct.
#define allocator_default_free_struct(T, v) allocator_default_free(blob_new((v), sizeof(T)))

// Frees an arra of structs of the given type and length allocated with
// allocator_default_malloc_structs.
#define allocator_default_free_structs(T, N, v) allocator_default_free(blob_new((v), (N) * sizeof(T)))

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
  atomic_int64_t live_memory;
  // Total number of blocks allocated.
  atomic_int64_t live_blocks;
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

// A fingerprinting allocator computes a fingerprint for each allocation and
// matches up the fingerprint of allocations and frees, reporting errors if they
// don't match. Similar to the limited allocator but may narrow down the site
// of incorrect allocations/frees.
typedef struct {
  allocator_t header;
  // The default allocator this one is replacing.
  allocator_t *outer;
  // How many blocks of memory have been allocated for a given fingerprint?
  atomic_int64_t *blocks;
  // How many bytes of memory have been allocated for a given fingerprint?
  atomic_int64_t *bytes;
  // Has this allocator issued any warnings?
  bool has_warned;
} fingerprinting_allocator_t;

// Initializes the given allocator and installs it as the default.
void fingerprinting_allocator_install(fingerprinting_allocator_t *alloc);

// Uninstalls the given allocator, which must be the current default, and
// restores the previous one.
bool fingerprinting_allocator_uninstall(fingerprinting_allocator_t *alloc);

#endif // _TCLIB_ALLOC_H

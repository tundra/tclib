//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/alloc.h"
#include "utils/log.h"
#include "sync/mutex.h"

static const uint8_t kMallocHeapMarker = 0xB0;
static const uint8_t kMallocFreedMarker = 0xC0;

// Throws away the data argument and just calls malloc.
static blob_t system_malloc_trampoline(allocator_t *self, size_t size) {
  void *chunk = malloc(size);
  if (chunk == NULL) {
    return blob_empty();
  } else {
    blob_t result = blob_new(chunk, size);
    blob_fill(result, kMallocHeapMarker);
    return result;
  }
}

// Throws away the data argument and just calls free.
static void system_free_trampoline(allocator_t *self, blob_t memory) {
  if (!blob_is_empty(memory))
    blob_fill(memory, kMallocFreedMarker);
  free(memory.start);
}

allocator_t allocator_system() {
  allocator_t result;
  struct_zero_fill(result);
  result.malloc = system_malloc_trampoline;
  result.free = system_free_trampoline;
  return result;
}

blob_t allocator_malloc(allocator_t *alloc, size_t size) {
  return (alloc->malloc)(alloc, size);
}

void allocator_free(allocator_t *alloc, blob_t memory) {
  (alloc->free)(alloc, memory);
}

blob_t allocator_default_malloc(size_t size) {
  return allocator_malloc(allocator_get_default(), size);
}

void allocator_default_free(blob_t block) {
  allocator_free(allocator_get_default(), block);
}

allocator_t kSystemAllocator;
allocator_t *allocator_default = NULL;

allocator_t *allocator_get_default() {
  if (allocator_default == NULL) {
    kSystemAllocator = allocator_system();
    allocator_default = &kSystemAllocator;
  }
  return allocator_default;
}

allocator_t *allocator_set_default(allocator_t *value) {
  allocator_t *previous = allocator_get_default();
  allocator_default = value;
  return previous;
}

static void limited_allocator_free(allocator_t *raw_self, blob_t memory) {
  if (blob_is_empty(memory))
    return;
  limited_allocator_t *data = (limited_allocator_t*) raw_self;
  size_t live_memory = (size_t) atomic_int64_get(&data->live_memory);
  if (memory.size > live_memory) {
    data->has_warned = true;
    FATAL("Unbalanced free of %ib with %ib live", memory.size, live_memory);
  }
  atomic_int64_subtract(&data->live_memory, memory.size);
  atomic_int64_decrement(&data->live_blocks);
  allocator_free(data->outer, memory);
}

static blob_t limited_allocator_malloc(allocator_t *raw_self, size_t size) {
  if (size == 0)
    return blob_empty();
  limited_allocator_t *data = (limited_allocator_t*) raw_self;
  size_t live_memory = (size_t) atomic_int64_get(&data->live_memory);
  if (live_memory + size > data->limit) {
    data->has_warned = true;
    WARN("Tried to allocate more than %i of system memory. At %i, requested %i.",
        data->limit, live_memory, size);
    return blob_empty();
  } else {
    atomic_int64_add(&data->live_memory, size);
    atomic_int64_increment(&data->live_blocks);
    return allocator_malloc(data->outer, size);
  }
}

void limited_allocator_install(limited_allocator_t *alloc, size_t limit) {
  struct_zero_fill(*alloc);
  alloc->header.malloc = limited_allocator_malloc;
  alloc->header.free = limited_allocator_free;
  alloc->live_memory = atomic_int64_new(0);
  alloc->live_blocks = atomic_int64_new(0);
  alloc->limit = limit;
  alloc->has_warned = false;
  alloc->outer = allocator_set_default(&alloc->header);
}

bool limited_allocator_uninstall(limited_allocator_t *alloc) {
  CHECK_PTREQ("not current allocator", &alloc->header, allocator_get_default());
  allocator_set_default(alloc->outer);
  size_t live_memory = (size_t) atomic_int64_get(&alloc->live_memory);
  size_t live_blocks = (size_t) atomic_int64_get(&alloc->live_blocks);
  bool had_leaks = (live_memory > 0) || (live_blocks > 0);
  if (had_leaks) {
    WARN("Disposing with %ib of live memory in %i blocks", live_memory,
        live_blocks);
  }
  return !had_leaks && !alloc->has_warned;
}

// How many buckets do we divide allocations into by fingerprint?
#define kAllocFingerprintBuckets 65521

static size_t calc_fingerprint(blob_t blob) {
  address_arith_t addr = (address_arith_t) blob.start;
  uint64_t v64 = ((uint64_t) addr) * blob.size;
  uint32_t v32 = (uint32_t) ((v64 & 0xFFFFFFFF) ^ (v64 >> 32));
  uint32_t raw = (v32 * 2654435761U) % kAllocFingerprintBuckets;
  return (size_t) raw;
}

static blob_t fingerprinting_allocator_malloc(allocator_t *raw_self, size_t size) {
  fingerprinting_allocator_t *self = (fingerprinting_allocator_t*) raw_self;
  blob_t result = allocator_malloc(self->outer, size);
  if (blob_is_empty(result))
    return result;
  size_t fprint = calc_fingerprint(result);
  atomic_int64_increment(&self->blocks[fprint]);
  atomic_int64_add(&self->bytes[fprint], size);
  return result;
}

static void fingerprinting_allocator_free(allocator_t *raw_self, blob_t memory) {
  fingerprinting_allocator_t *self = (fingerprinting_allocator_t*) raw_self;
  if (blob_is_empty(memory))
    return;
  size_t fprint = calc_fingerprint(memory);
  size_t blocks = (size_t) atomic_int64_get(&self->blocks[fprint]);
  size_t bytes = (size_t) atomic_int64_get(&self->bytes[fprint]);
  if (blocks == 0 || bytes < memory.size) {
    self->has_warned = true;
    WARN("Unbalanced free of %ib with fingerprint %04X", memory.size, fprint);
  }
  atomic_int64_decrement(&self->blocks[fprint]);
  atomic_int64_subtract(&self->bytes[fprint], memory.size);
  allocator_free(self->outer, memory);
}

void fingerprinting_allocator_install(fingerprinting_allocator_t *alloc) {
  struct_zero_fill(*alloc);
  alloc->header.malloc = fingerprinting_allocator_malloc;
  alloc->header.free = fingerprinting_allocator_free;
  alloc->has_warned = false;
  alloc->outer = allocator_set_default(&alloc->header);
  blob_t blocks = allocator_malloc(alloc->outer, kAllocFingerprintBuckets * sizeof(atomic_int64_t));
  blob_fill(blocks, 0);
  blob_t bytes = allocator_malloc(alloc->outer, kAllocFingerprintBuckets * sizeof(atomic_int64_t));
  blob_fill(bytes, 0);
  alloc->blocks = (atomic_int64_t*) blocks.start;
  alloc->bytes = (atomic_int64_t*) bytes.start;
}

bool fingerprinting_allocator_uninstall(fingerprinting_allocator_t *alloc) {
  bool had_leaks = false;
  for (size_t i = 0; i < kAllocFingerprintBuckets; i++) {
    size_t blocks = (size_t) atomic_int64_get(&alloc->blocks[i]);
    size_t bytes = (size_t) atomic_int64_get(&alloc->bytes[i]);
    if (blocks > 0 || bytes > 0) {
      had_leaks = true;
      WARN("Disposing with %ib of live memory in %i allocations with fingerprint %04X",
          bytes, blocks, i);
    }
  }
  blob_t blocks = blob_new(alloc->blocks, kAllocFingerprintBuckets * sizeof(atomic_int64_t));
  blob_t bytes = blob_new(alloc->bytes, kAllocFingerprintBuckets * sizeof(atomic_int64_t));
  allocator_free(alloc->outer, blocks);
  allocator_free(alloc->outer, bytes);
  return allocator_set_default(alloc->outer) && !had_leaks && !alloc->has_warned;
}

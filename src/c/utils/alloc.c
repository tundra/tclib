//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/alloc.h"
#include "utils/check.h"
#include "utils/log.h"

static const uint8_t kMallocHeapMarker = 0xB0;

memory_block_t memory_block_empty() {
  return new_memory_block(NULL, 0);
}

memory_block_t new_memory_block(void *memory, size_t size) {
  memory_block_t result;
  result.memory = memory;
  result.size = size;
  return result;
}

bool memory_block_is_empty(memory_block_t block) {
  return block.memory == NULL;
}

// Throws away the data argument and just calls malloc.
static memory_block_t system_malloc_trampoline(void *data, size_t size) {
  CHECK_PTREQ("invalid system allocator", NULL, data);
  void *result = malloc(size);
  if (result == NULL) {
    return memory_block_empty();
  } else {
    memset(result, kMallocHeapMarker, size);
    return new_memory_block(result, size);
  }
}

// Throws away the data argument and just calls free.
static void system_free_trampoline(void *data, memory_block_t memory) {
  CHECK_PTREQ("invalid system allocator", NULL, data);
  free(memory.memory);
}

void init_system_allocator(allocator_t *alloc) {
  alloc->malloc = system_malloc_trampoline;
  alloc->free = system_free_trampoline;
  alloc->data = NULL;
}

memory_block_t allocator_malloc(allocator_t *alloc, size_t size) {
  return (alloc->malloc)(alloc->data, size);
}

void allocator_free(allocator_t *alloc, memory_block_t memory) {
  (alloc->free)(alloc->data, memory);
}

memory_block_t allocator_default_malloc(size_t size) {
  return allocator_malloc(allocator_get_default(), size);
}

void allocator_default_free(memory_block_t block) {
  allocator_free(allocator_get_default(), block);
}

allocator_t kSystemAllocator;
allocator_t *allocator_default = NULL;

allocator_t *allocator_get_default() {
  if (allocator_default == NULL) {
    init_system_allocator(&kSystemAllocator);
    allocator_default = &kSystemAllocator;
  }
  return allocator_default;
}

allocator_t *allocator_set_default(allocator_t *value) {
  allocator_t *previous = allocator_get_default();
  allocator_default = value;
  return previous;
}

static void limited_allocator_free(void *raw_data, memory_block_t memory) {
  if (memory_block_is_empty(memory))
    return;
  limited_allocator_t *data = (limited_allocator_t*) raw_data;
  if (memory.size > data->live_memory)
    FATAL("Unbalanced free of %ib", memory.size);
  data->live_memory -= memory.size;
  data->live_blocks--;
  allocator_free(data->outer, memory);
}

static memory_block_t limited_allocator_malloc(void *raw_data, size_t size) {
  if (size == 0)
    return memory_block_empty();
  limited_allocator_t *data = (limited_allocator_t*) raw_data;
  if (data->live_memory + size > data->limit) {
    WARN("Tried to allocate more than %i of system memory. At %i, requested %i.",
        data->limit, data->live_memory, size);
    return memory_block_empty();
  }
  memory_block_t result = allocator_malloc(data->outer, size);
  if (!memory_block_is_empty(result)) {
    data->live_memory += result.size;
    data->live_blocks++;
  }
  return result;
}

void limited_allocator_install(limited_allocator_t *alloc, size_t limit) {
  alloc->live_memory = 0;
  alloc->live_blocks = 0;
  alloc->limit = limit;
  alloc->self.data = alloc;
  alloc->self.free = limited_allocator_free;
  alloc->self.malloc = limited_allocator_malloc;
  alloc->outer = allocator_set_default(&alloc->self);
}

bool limited_allocator_uninstall(limited_allocator_t *alloc) {
  CHECK_PTREQ("not current allocator", &alloc->self, allocator_get_default());
  allocator_set_default(alloc->outer);
  bool had_leaks = (alloc->live_memory > 0) || (alloc->live_blocks > 0);
  if (had_leaks) {
    WARN("Disposing with %ib of live memory in %i blocks", alloc->live_memory,
        alloc->live_blocks);
  }
  return !had_leaks;
}

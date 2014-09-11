#include "utils/alloc.h"
#include "utils/check.h"

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

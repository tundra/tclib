//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
#include "utils/clock.h"
#include "utils/eventseq.h"
END_C_INCLUDES

void event_sequence_init(event_sequence_t *seq, size_t length) {
  struct_zero_fill(*seq);
  seq->next_index = atomic_int32_new(0);
  seq->length = length;
  seq->entries = allocator_default_malloc_structs(event_sequence_entry_t, length);
}

void event_sequence_dispose(event_sequence_t *seq) {
  allocator_default_free_structs(event_sequence_entry_t, seq->length, seq->entries);
}

void event_sequence_record(event_sequence_t *seq, const char *tag) {
  native_time_t raw_timestamp = real_time_clock_time_since_epoch_utc(real_time_clock_system());
  event_sequence_entry_t entry;
  entry.tag = tag;
  entry.timestamp = native_time_to_millis(raw_timestamp);
  entry.thread = native_thread_get_current_id();
  int32_t raw_index = atomic_int32_increment(&seq->next_index) - 1;
  size_t index = (size_t) (raw_index % seq->length);
  seq->entries[index] = entry;
}
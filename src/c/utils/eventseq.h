//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_EVENTSEQ_H
#define _TCLIB_EVENTSEQ_H

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/atomic.h"
#include "sync/thread.h"
END_C_INCLUDES

// An individual event in an event sequence.
typedef struct {
  // User provided tag.
  const char *tag;
  // The time this event occurred.
  uint64_t timestamp;
  // Thread that recorded the event.
  native_thread_id_t thread;
} event_sequence_entry_t;

// An event sequence is a lightweight thread safe circular buffer that threads
// can write events into. This can be useful when debugging threading issues
// because the threads can leave a trace of events in the sequence which can
// be inspected in the debugger.
typedef struct {
  atomic_int32_t next_index;
  event_sequence_entry_t *entries;
  size_t length;
} event_sequence_t;

// Initialize the event sequence with the given length.
void event_sequence_init(event_sequence_t *seq, size_t length);

// Release the sequence's resources.
void event_sequence_dispose(event_sequence_t *seq);

// Record an event with the given tag in the sequence.
void event_sequence_record(event_sequence_t *seq, const char *tag);

#endif // _TCLIB_EVENTSEQ_H

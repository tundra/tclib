//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/eventseq.h"
END_C_INCLUDES

TEST(eventseq, simple) {
  event_sequence_t seq;
  event_sequence_init(&seq, 256);
  ASSERT_TRUE(event_sequence_is_empty(&seq));
  for (size_t i = 0; i < 1024; i++) {
    event_sequence_record(&seq, "open", NULL);
    ASSERT_FALSE(event_sequence_is_empty(&seq));
    event_sequence_record(&seq, "close", NULL);
  }
  event_sequence_dispose(&seq);
}

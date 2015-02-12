//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "sync/semaphore.h"
END_C_INCLUDES

TEST(semaphore_c, default_initial) {
  native_semaphore_t sema;
  native_semaphore_construct(&sema);
  ASSERT_TRUE(native_semaphore_initialize(&sema));
  ASSERT_TRUE(native_semaphore_acquire(&sema, duration_unlimited()));
  ASSERT_FALSE(native_semaphore_try_acquire(&sema));
  ASSERT_TRUE(native_semaphore_release(&sema));
  ASSERT_TRUE(native_semaphore_acquire(&sema, duration_unlimited()));
  ASSERT_FALSE(native_semaphore_try_acquire(&sema));
  ASSERT_TRUE(native_semaphore_release(&sema));
  ASSERT_TRUE(native_semaphore_release(&sema));
  ASSERT_TRUE(native_semaphore_try_acquire(&sema));
  ASSERT_TRUE(native_semaphore_try_acquire(&sema));
  ASSERT_FALSE(native_semaphore_try_acquire(&sema));
  native_semaphore_dispose(&sema);
}

TEST(semaphore_c, explicit_initial) {
  native_semaphore_t sema;
  native_semaphore_construct_with_count(&sema, 256);
  ASSERT_TRUE(native_semaphore_initialize(&sema));
  for (size_t i = 0; i < 256; i++)
    ASSERT_TRUE(native_semaphore_acquire(&sema, duration_unlimited()));
  ASSERT_FALSE(native_semaphore_try_acquire(&sema));
  native_semaphore_dispose(&sema);
}

TEST(semaphore_c, timed_wait) {
  native_semaphore_t sema;
  native_semaphore_construct_with_count(&sema, 0);
  ASSERT_TRUE(native_semaphore_initialize(&sema));
  ASSERT_FALSE(native_semaphore_acquire(&sema, duration_millis(100)));
  native_semaphore_dispose(&sema);
}

//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "sync/mutex.h"
#include "sync/thread.h"
#include "utils/callback.h"
END_C_INCLUDES

static opaque_t fail_to_unlock(opaque_t opaque_m0, opaque_t opaque_m1) {
  native_mutex_t *m0 = (native_mutex_t*) o2p(opaque_m0);
  native_mutex_t *m1 = (native_mutex_t*) o2p(opaque_m1);
  ASSERT_FALSE(native_mutex_try_lock(m0));
  ASSERT_FALSE(native_mutex_unlock(m0));
  ASSERT_FALSE(native_mutex_try_lock(m1));
  ASSERT_FALSE(native_mutex_unlock(m1));
  return opaque_null();
}

static opaque_t do_lock(opaque_t opaque_m0, opaque_t opaque_m1) {
  native_mutex_t *m0 = (native_mutex_t*) o2p(opaque_m0);
  native_mutex_t *m1 = (native_mutex_t*) o2p(opaque_m1);
  ASSERT_TRUE(native_mutex_lock(m0));
  ASSERT_TRUE(native_mutex_lock(m1));
  ASSERT_TRUE(native_mutex_unlock(m0));
  ASSERT_TRUE(native_mutex_unlock(m1));
  return opaque_null();
}

TEST(mutex_c, simple) {
  // Some of the failures cause log messages to be issued, which we don't want
  // to see.
  log_o *noisy_log = silence_global_log();

  native_mutex_t *m0 = new_native_mutex();
  ASSERT_TRUE(native_mutex_initialize(m0));
  native_mutex_t *m1 = new_native_mutex();
  ASSERT_TRUE(native_mutex_initialize(m1));
  ASSERT_FALSE(native_mutex_unlock(m0));
  ASSERT_TRUE(native_mutex_lock(m0));
  ASSERT_FALSE(native_mutex_unlock(m1));
  ASSERT_TRUE(native_mutex_lock(m1));

  // Try and fail to lock/unlock from a different thread.
  nullary_callback_t *failer_callback = new_nullary_callback_2(fail_to_unlock,
      p2o(m0), p2o(m1));
  native_thread_t *failer = new_native_thread(failer_callback);
  ASSERT_TRUE(native_thread_start(failer));
  native_thread_join(failer);
  dispose_native_thread(failer);
  callback_dispose(failer_callback);

  // Locking recursively works.
  ASSERT_TRUE(native_mutex_lock(m1));
  ASSERT_TRUE(native_mutex_lock(m1));
  ASSERT_TRUE(native_mutex_unlock(m1));
  ASSERT_TRUE(native_mutex_unlock(m1));
  ASSERT_TRUE(native_mutex_unlock(m1));
  ASSERT_FALSE(native_mutex_unlock(m1));

  // Unlock completely and let a different thread try locking.
  ASSERT_TRUE(native_mutex_unlock(m0));
  nullary_callback_t *succeeder_callback = new_nullary_callback_2(do_lock,
      p2o(m0), p2o(m1));
  native_thread_t *succeeder = new_native_thread(succeeder_callback);
  ASSERT_TRUE(native_thread_start(succeeder));
  native_thread_join(succeeder);
  dispose_native_thread(succeeder);
  callback_dispose(succeeder_callback);

  native_mutex_dispose(m0);
  native_mutex_dispose(m1);
  set_global_log(noisy_log);
}

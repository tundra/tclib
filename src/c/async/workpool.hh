//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_WORKPOOL_HH
#define _TCLIB_WORKPOOL_HH

#include "c/stdc.h"
#include "sync/thread.hh"
#include "sync/semaphore.hh"

BEGIN_C_INCLUDES
#include "async/promise.h"
END_C_INCLUDES

namespace tclib {

class Task;

typedef enum {
  // The given task is required to be run before the workpool is allowed to shut
  // down.
  tfRequired = 0x00,

  // It is acceptable if the given task is not executed before the workpool is
  // shut down.
  tfDaemon = 0x01
} task_flag_t;

class Workpool {
public:
  typedef callback_t<opaque_t()> task_thunk_t;

  Workpool();

  // Prepares this workpool for running. The worker thread(s) won't be started
  // but after this you can add tasks.
  bool initialize();

  // Starts the worker thread(s) running.
  bool start();

  // Adds a task to the set this workpool should run. By default the task will
  // keep the workpool running until the task has been executed but the flags
  // can use to control that. Thread safe.
  bool add_task(task_thunk_t task, int32_t flags);

  // Runs this workpool until it has no more tasks. If the flag is true then
  // we execute daemon tasks, otherwise those are skipped.
  bool join(bool skip_daemons = true);

  // Instructs the workpool whether to skip daemons or not. Usually you'd let
  // this happen automatically from join but it's exposed separately for
  // testing.
  bool set_skip_daemons(bool value);

private:
  // Entry-point for worker threads.
  opaque_t run_worker();

  // Adds the given task to the list run by this workpool.
  bool offer_task(Task *task);

  // Waits for the next task to become available, then takes it and stores it in
  // the given out parameter. When shutting down this may return NULL, otherwise
  // it is guaranteed not to.
  bool poll_task(Task **task_out);

  // The next task to perform. Guarded by guard_;
  Task *next_task_;

  // The last task to perform; new tasks are added after this one. Guarded by
  // guard_.
  Task *last_task_;

  // Is this workpool shutting down?
  bool is_shutting_down_;

  // Do we execute or skip daemon tasks?
  bool skip_daemons_;

  // Worker thread.
  NativeThread *worker_;

  // How many actions are left to perform? An action can either be performing a
  // task or shutting down. When shutting down this should stay nonzero or, if
  // it becomes zero, should be released to allow all workers to terminate.
  NativeSemaphore action_count_;

  // Guards all state that is not otherwise synchronized.
  NativeMutex guard_;
};

} // namespace tclib

#endif // _TCLIB_WORKPOOL_HH

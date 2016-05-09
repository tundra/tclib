//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/trybool.h"
END_C_INCLUDES

#include "async/workpool.hh"
#include "utils/alloc.hh"

using namespace tclib;

namespace tclib {
class Task {
public:
  Task(Workpool::task_thunk_t thunk, int32_t flags);
  bool is_daemon();
private:
  friend class Workpool;
  Workpool::task_thunk_t thunk_;
  Task *successor_;
  int32_t flags_;
};
} // namespace tclib

Task::Task(Workpool::task_thunk_t thunk, int32_t flags)
  : thunk_(thunk)
  , successor_(NULL)
  , flags_(flags) { }

bool Task::is_daemon() {
  return (flags_ & tfDaemon) != 0;
}

Workpool::Workpool()
  : next_task_(NULL)
  , last_task_(NULL)
  , is_shutting_down_(false)
  , skip_daemons_(false)
  , worker_(NULL)
  , action_count_(0) { }

opaque_t Workpool::run_worker() {
  while (true) {
    Task *task = NULL;
    fat_bool_t polled = poll_task(&task);
    if (!polled)
      return f2o(polled);
    if (task == NULL)
      // There are no more tasks left so we can simply return.
      return f2o(F_TRUE);
    if (!(skip_daemons_ && task->is_daemon()))
      task->thunk_();
    default_delete_concrete(task);
  }
}

fat_bool_t Workpool::initialize() {
  F_TRY(action_count_.initialize());
  F_TRY(guard_.initialize());
  return F_TRUE;
}

fat_bool_t Workpool::start() {
  worker_ = new NativeThread(new_callback(&Workpool::run_worker, this));
  return worker_->start();
}

void Workpool::set_skip_daemons(bool skip_daemons) {
  skip_daemons_ = skip_daemons;
}

fat_bool_t Workpool::join(bool skip_daemons) {
  if (worker_ == NULL)
    return F_TRUE;
  F_TRY(guard_.lock());
    is_shutting_down_ = true;
    set_skip_daemons(skip_daemons);
  F_TRY(guard_.unlock());
  F_TRY(action_count_.release());
  opaque_t value = o0();
  F_TRY(worker_->join(&value));
  delete worker_;
  worker_ = NULL;
  return o2f(value);
}

fat_bool_t Workpool::add_task(task_thunk_t callback, int32_t flags) {
  Task *task = new (kDefaultAlloc) Task(callback, flags);
  if (task == NULL) {
    WARN("Failed to allocate task");
    return F_FALSE;
  }
  return offer_task(task);
}

fat_bool_t Workpool::offer_task(Task *task) {
  F_TRY(guard_.lock());
    if (next_task_ == NULL) {
      next_task_ = last_task_ = task;
    } else {
      Task *old_last_task = last_task_;
      last_task_ = old_last_task->successor_ = task;
    }
  F_TRY(guard_.unlock());
  F_TRY(action_count_.release());
  return F_TRUE;
}

fat_bool_t Workpool::poll_task(Task **task_out) {
  F_TRY(action_count_.acquire());
  F_TRY(guard_.lock());
    if (is_shutting_down_)
      // If we're shutting down we have to leave the action count nonzero such
      // that other workers also get the message.
      F_TRY(action_count_.release());
    Task *task = next_task_;
    if (task != NULL) {
      Task *new_next = task->successor_;
      next_task_ = new_next;
      if (new_next == NULL)
        last_task_ = NULL;
    }
  F_TRY(guard_.unlock());
  *task_out = task;
  return F_TRUE;
}

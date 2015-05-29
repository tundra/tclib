//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

#include "iop.hh"
using namespace tclib;

struct iop_group_state_t {
public:
  iop_group_state_t();
  ~iop_group_state_t();

  OVERLAPPED *overlapped() { return &overlapped_; }

  handle_t event() { return event_; }

  void recycle();

  bool has_been_scheduled_;

private:
  handle_t event_;
  OVERLAPPED overlapped_;
};

iop_group_state_t::iop_group_state_t()
  : has_been_scheduled_(false)
  , event_(INVALID_HANDLE_VALUE) {
  event_ = CreateEvent(
      NULL,  // lpEventAttributes
      false, // bManualReset
      false, // bInitialState
      NULL); // lpName
  ZeroMemory(&overlapped_, sizeof(overlapped_));
  overlapped_.hEvent = event_;
}

iop_group_state_t::~iop_group_state_t() {
  CloseHandle(event_);
}

void iop_group_state_t::recycle() {
  has_been_scheduled_ = false;
}

iop_group_state_t *Iop::get_or_create_group_state() {
  if (peek_group_state() == NULL)
    header()->group_state_ = new iop_group_state_t();
  return peek_group_state();
}

void Iop::platform_recycle() {
  iop_group_state_t *group_state = peek_group_state();
  if (group_state != NULL)
    group_state->recycle();
}

bool Iop::finish_nonblocking() {
  iop_group_state_t *group_state = peek_group_state();
  CHECK_TRUE("finishing non-scheduled", group_state->has_been_scheduled_);
  handle_t handle = stream()->to_raw_handle();
  OVERLAPPED *overlapped = group_state->overlapped();
  dword_t bytes_read = 0;
  bool result = GetOverlappedResult(
      handle,      // hFile
      overlapped,  // lpOverlapped
      &bytes_read, // lpNumberOfBytesTransferred
      false);      // bWait
  bool at_eof = !result && (bytes_read == 0);
  read_iop_deliver(as_read(), bytes_read, at_eof);
  return true;
}

Iop::ensure_scheduled_outcome_t Iop::ensure_scheduled() {
  CHECK_FALSE("scheduling complete iop", is_complete());
  iop_group_state_t *group_state = get_or_create_group_state();
  if (group_state->has_been_scheduled_)
    // This iop is already scheduled so don't do it again.
    return eoScheduled;
  handle_t handle = stream()->to_raw_handle();
  if (handle == AbstractStream::kNullNakedFileHandle) {
    WARN("Multiplexing invalid stream");
    mark_complete(false);
    return eoFailedImmediately;
  }
  // The op is not complete and it has not already been scheduled. Schedule it.
  dword_t bytes_read = 0;
  OVERLAPPED *overlapped = group_state->overlapped();
  read_iop_t *read_iop = as_read();
  bool result = ReadFile(
      handle,                                     // hFile
      read_iop->dest_,                            // lpBuffer
      static_cast<dword_t>(read_iop->dest_size_), // nNumberOfBytesToRead
      &bytes_read,                                // lpNumberOfBytesRead
      overlapped);                                // lpOverlapped
  if (result) {
    // Read may succeed immediately in which case we're done. If the read does
    // succeed it will have read something, if it can't read the call will fail
    // or block. That's the assumption.
    CHECK_TRUE("successful empty read", bytes_read > 0);
    read_iop_deliver(read_iop, bytes_read, false);
    mark_complete(true);
    return eoCompletedImmediately;
  } else if (GetLastError() == ERROR_IO_PENDING) {
    // The read call succeeded in scheduling the read but we have to wait for
    // it to complete.
    group_state->has_been_scheduled_ = true;
    return eoScheduled;
  } else {
    read_iop_deliver(read_iop, bytes_read, true);
    mark_complete(true);
    // Scheduling went wrong for some reason; bail.
    return eoCompletedImmediately;
  }
}

bool IopGroup::wait_for_next(size_t *index_out) {
  // Okay, so this is a little bit involved. It works like this.
  //
  // In the current set of ops there will be some that have already completed
  // and some that have been started but not yet completed. We first scan
  // through and start any ones that haven't been started. It may happen that
  // starting one causes it to complete immediately, in which case we're done.
  // At that point we may have started a number of the others but won't ever
  // have gotten to the point of waiting for them and that's fine.
  //
  // If we get past this point none of the ops will have completed immediately
  // and any incomplete ones will have been started.

  size_t count = ops_.size();
  CHECK_TRUE("multiplexing too much", count <= MAXIMUM_WAIT_OBJECTS);
  std::vector<handle_t> events;
  for (size_t i = 0; i < count; i++) {
    Iop *iop = ops_[i];
    if (iop->is_complete())
      continue;
    CHECK_TRUE("unsupported iop", iop->is_read());
    Iop::ensure_scheduled_outcome_t outcome = iop->ensure_scheduled();
    if (outcome == Iop::eoCompletedImmediately) {
      *index_out = i;
      return true;
    } else if (outcome == Iop::eoFailedImmediately) {
      return false;
    } else {
      events.push_back(iop->peek_group_state()->event());
    }
  }
  dword_t result = WaitForMultipleObjects(
      static_cast<dword_t>(events.size()), // nCount
      events.data(),                       // lpHandles
      false,                               // bWaitAll
      INFINITE);                           // dwMilliseconds
  // Thanks for the brilliant insight MSVC that WAIT_OBJECT_0 is 0 but maybe
  // I'd like to use a symbolic constant rather than inline that fact.
  dword_t lemme_use_wait_object_0 = WAIT_OBJECT_0;
  if (!(lemme_use_wait_object_0 <= result && result < (WAIT_OBJECT_0 + count)))
    return false;
  size_t index = result - WAIT_OBJECT_0;
  handle_t event = events[index];
  for (size_t i = 0; i < count; i++) {
    Iop *iop = ops_[i];
    if (iop->is_complete())
      continue;
    if (event == iop->peek_group_state()->event()) {
      *index_out = i;
      bool result = iop->finish_nonblocking();
      iop->mark_complete(result);
      return true;
    }
  }
  return false;
}

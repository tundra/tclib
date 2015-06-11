//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

#include "iop.hh"
using namespace tclib;

struct iop_group_state_t {
public:
  iop_group_state_t();
  ~iop_group_state_t();

  // Resets the group state to make it ready to reuse.
  void recycle();

  OVERLAPPED *overlapped() { return &overlapped_; }
  handle_t event() { return event_; }
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
    group_state_ = new iop_group_state_t();
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
  dword_t bytes_xferred = 0;
  bool result = GetOverlappedResult(
      handle,         // hFile
      overlapped,     // lpOverlapped
      &bytes_xferred, // lpNumberOfBytesTransferred
      false);         // bWait
  if (is_read()) {
    bool at_eof = !result && (bytes_xferred == 0);
    read_iop_state_deliver(as_read(), bytes_xferred, at_eof);
  } else {
    write_iop_state_deliver(as_write(), bytes_xferred);
  }
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

  return is_read()
      ? schedule_read(handle, as_read())
      : schedule_write(handle, as_write());
}

Iop::ensure_scheduled_outcome_t Iop::schedule_read(handle_t handle,
    read_iop_state_t *read_iop) {
  iop_group_state_t *group_state = peek_group_state();
  dword_t bytes_read = 0;
  OVERLAPPED *overlapped = group_state->overlapped();
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
    read_iop_state_deliver(read_iop, bytes_read, false);
    mark_complete(true);
    return eoCompletedImmediately;
  } else if (GetLastError() == ERROR_IO_PENDING) {
    // The read call succeeded in scheduling the read but we have to wait for
    // it to complete.
    group_state->has_been_scheduled_ = true;
    return eoScheduled;
  } else {
    read_iop_state_deliver(read_iop, bytes_read, true);
    mark_complete(true);
    // Scheduling went wrong for some reason; bail.
    return eoCompletedImmediately;
  }
}

Iop::ensure_scheduled_outcome_t Iop::schedule_write(handle_t handle,
    write_iop_state_t *write_iop) {
  iop_group_state_t *group_state = peek_group_state();
  dword_t bytes_written = 0;
  OVERLAPPED *overlapped = group_state->overlapped();
  bool result = WriteFile(
      handle,                                     // hFile
      write_iop->src_,                            // lpBuffer
      static_cast<dword_t>(write_iop->src_size_), // nNumberOfBytesToWrite
      &bytes_written,                             // lpNumberOfBytesWritten
      overlapped);                                // lpOverlapped
  if (result) {
    write_iop_state_deliver(write_iop, bytes_written);
    mark_complete(true);
    return eoCompletedImmediately;
  } else if (GetLastError() == ERROR_IO_PENDING) {
    // The read call succeeded in scheduling the read but we have to wait for
    // it to complete.
    group_state->has_been_scheduled_ = true;
    return eoScheduled;
  } else {
    write_iop_state_deliver(write_iop, bytes_written);
    mark_complete(true);
    // Scheduling went wrong for some reason; bail.
    return eoCompletedImmediately;
  }
  return eoFailedImmediately;
}

bool IopGroup::wait_for_next(Duration timeout, opaque_t *extra_out) {
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

  std::vector<handle_t> events;
  for (size_t i = 0; i < ops()->size(); i++) {
    Iop *iop = ops()->at(i);
    if (iop == NULL)
      continue;
    Iop::ensure_scheduled_outcome_t outcome = iop->ensure_scheduled();
    if (outcome == Iop::eoCompletedImmediately) {
      *extra_out = iop->extra();
      return true;
    } else if (outcome == Iop::eoFailedImmediately) {
      return false;
    } else {
      events.push_back(iop->peek_group_state()->event());
    }
  }
  size_t count = events.size();
  CHECK_TRUE("multiplexing too much", count <= MAXIMUM_WAIT_OBJECTS);
  dword_t result = WaitForMultipleObjects(
      static_cast<dword_t>(events.size()), // nCount
      events.data(),                       // lpHandles
      false,                               // bWaitAll
      timeout.to_winapi_millis());         // dwMilliseconds
  // Thanks for the brilliant insight MSVC that WAIT_OBJECT_0 is 0 but maybe
  // I'd like to use a symbolic constant rather than inline that fact.
  dword_t lemme_use_wait_object_0 = WAIT_OBJECT_0;
  if (!(lemme_use_wait_object_0 <= result && result < (WAIT_OBJECT_0 + count)))
    return false;
  size_t index = result - WAIT_OBJECT_0;
  handle_t event = events[index];
  for (size_t i = 0; i < ops()->size(); i++) {
    Iop *iop = ops()->at(i);
    if (iop == NULL)
      continue;
    if (event == iop->peek_group_state()->event()) {
      *extra_out = iop->extra();
      bool result = iop->finish_nonblocking();
      iop->mark_complete(result);
      return true;
    }
  }
  UNREACHABLE("event matched no iops");
  return false;
}

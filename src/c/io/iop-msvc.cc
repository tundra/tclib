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
  void recycle();

  bool has_been_scheduled_;
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
    iop()->group_state_ = new iop_group_state_t();
  return peek_group_state();
}

void Iop::platform_recycle() {
  iop_group_state_t *group_state = peek_group_state();
  if (group_state != NULL)
    group_state->recycle();
}

bool Iop::finish_nonblocking() {
  handle_t handle = stream()->to_raw_handle();
  iop_group_state_t *group_state = peek_group_state();
  OVERLAPPED *overlapped = group_state->overlapped();
  dword_t read = 0;
  bool result = GetOverlappedResult(
      handle,     // hFile
      overlapped, // lpOverlapped
      &read,      // lpNumberOfBytesTransferred
      false);     // bWait
  as_read()->read_out_ = read;
  if (!result && read == 0)
    as_read()->at_eof_ = true;
  return true;
}

Iop::ensure_scheduled_outcome_t Iop::ensure_scheduled() {
  CHECK_FALSE("scheduling complete iop", is_complete());
  iop_group_state_t *group_state = get_or_create_group_state();
  if (group_state->has_been_scheduled_) {
    // This iop is already scheduled so don't do it again.
    return eoScheduled;
  }
  handle_t handle = stream()->to_raw_handle();
  if (handle == AbstractStream::kNullNakedFileHandle) {
    WARN("Multiplexing invalid stream");
    mark_complete(false);
    return eoFailedImmediately;
  }
  dword_t read = 0;
  OVERLAPPED *overlapped = group_state->overlapped();
  read_iop_t *read_iop = as_read();
  bool result = ReadFile(
      handle,                                     // hFile
      read_iop->dest_,                            // lpBuffer
      static_cast<dword_t>(read_iop->dest_size_), // nNumberOfBytesToRead
      &read,                                      // lpNumberOfBytesRead
      overlapped);                                // lpOverlapped
  if (result) {
    // The read call succeeded immediately, we didn't even have to wait.
    read_iop->read_out_ = read;
    mark_complete(true);
    return eoCompletedImmediately;
  } else {
    int err = GetLastError();
    if (err == ERROR_IO_PENDING) {
      // The read call succeeded in scheduling the read but we have to wait for
      // it to complete.
      group_state->has_been_scheduled_ = true;
      return eoScheduled;
    } else {
      read_iop->at_eof_ = true;
      mark_complete(true);
      // Scheduling went wrong for some reason; bail.
      return eoCompletedImmediately;
    }
  }
}

bool IopGroup::wait_for_next(size_t *index_out) {
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
      events.push_back(iop->peek_group_state()->event_);
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
    if (iop->peek_group_state()->event_ == event) {
      *index_out = i;
      bool result = iop->finish_nonblocking();
      iop->mark_complete(result);
      return true;
    }
  }
  return false;
}

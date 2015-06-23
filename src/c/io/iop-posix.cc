//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <sys/select.h>
#include <errno.h>

#include "sync/thread.hh"

#include "iop.hh"
using namespace tclib;

// Dummy state.
struct iop_group_state_t { };

void Iop::platform_recycle() {
  // nothing to do
}

bool Iop::finish_nonblocking() {
  return is_read()
      ? as_in()->read_sync(as_read())
      : as_out()->write_sync(as_write());
}

bool IopGroup::wait_for_next(Duration timeout, Iop **iop_out) {
  // On posix waiting for the next iop is done by selecting for the next fd that
  // can be accessed without blocking and then executing the op that accesses
  // that one. This means that there is no actual parallelism in performing the
  // ops, only in waiting until one can be performed.
  fd_set reads;
  FD_ZERO(&reads);
  fd_set writes;
  FD_ZERO(&writes);
  int high_fd_mark = 0;
  for (size_t i = 0; i < ops()->size(); i++) {
    Iop *iop = ops()->at(i);
    if (iop == NULL)
      continue;
    CHECK_FALSE("complete iop member", iop->is_complete());
    AbstractStream *stream = iop->stream();
    naked_file_handle_t fd = stream->to_raw_handle();
    if (fd == AbstractStream::kNullNakedFileHandle) {
      WARN("Multiplexing over invalid in stream");
      return false;
    }
    if (fd > high_fd_mark)
      high_fd_mark = fd;
    fd_set *set = iop->is_read() ? &reads : &writes;
    FD_SET(fd, set);
  }
  struct timeval timeout_data;
  struct timeval *timeout_ptr;
  if (!timeout.is_unlimited()) {
    timeout_data = timeout.to_timeval();
    timeout_ptr = &timeout_data;
  } else {
    timeout_ptr = NULL;
  }
  if (select(high_fd_mark + 1, &reads, &writes, NULL, timeout_ptr) == -1) {
    WARN("Call to select failed: %s", strerror(errno));
    return false;
  }
  // Scan through the out fd_set to identify the stream that became available.
  for (size_t i = 0; i < ops()->size(); i++) {
    Iop *iop = ops()->at(i);
    if (iop == NULL)
      continue;
    AbstractStream *stream = iop->stream();
    naked_file_handle_t fd = stream->to_raw_handle();
    fd_set *set = iop->is_read() ? &reads : &writes;
    if (FD_ISSET(fd, set)) {
      *iop_out = iop;
      bool result = iop->finish_nonblocking();
      iop->mark_complete(result);
      return true;
    }
  }
  // For some reason no streams were set in the output -- that's an error.
  return false;
}

//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_IOP_HH
#define _TCLIB_IOP_HH

#include "c/stdc.h"

#include "c/stdvector.hh"
#include "utils/duration.hh"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "utils/duration.h"
#include "io/iop.h"
END_C_INCLUDES

namespace tclib {

class AbstractStream;
class InStream;
class Iop;
class NativeProcess;
class OutStream;

// A set of iops that are executed asynchronously, in parallel. The discipline
// around iop groups is somewhat involved because it abstracts over two quite
// different apis on posix and windows. But if you observe the discipline you
// will get the same behavior independent of platforms. The discipline is as
// follows.
//
//   1. Once an iop has been added to a group you must wait for it to complete
//      at some point. If you wait for fewer iops than you've added the behavior
//      is unspecified because waiting for one may or may not start them all,
//      and then if you don't wait for one of the others they may or may not
//      happen depending on the implementation. The only way to get the same
//      behavior on all platforms is to always wait for all iops.
//
//   2. Once an iop has completed it is ignored in the next wait call. So if you
//      add 3 iops, the first wait call waits for all 3, the next skips the one
//      the completed in the first call and waits for the 2 remaining, the third
//      call to wait skips the two iops that completed in the first call and
//      only waits for the last remaining.
//
//   3. Once an iop has completed, successfully or otherwise, it may be reused
//      using the recycle method. This adds it back to the set wait will wait
//      for. Typically if you wait on multiple reads, for instance, you want all
//      the reads to stay active so as soon as one has succeeded you would
//      recycle it to reactivate it unless it reaches EOF.
class IopGroup : public iop_group_t {
public:
  IopGroup();
  ~IopGroup();

  // Add an iop to this group. See the class comment for how to use this
  // correctly. Only incomplete iops may be scheduled, also once an iop has been
  // scheduled it may not be completed synchronously. As soon as an iop has
  // completed the group will no longer use it so it is safe to dispose it. If
  // you recycle an iop that has been scheduled once it will be reinserted in
  // the group it was initially in, so you can't use an iop with multiple
  // groups.
  void schedule(Iop *iop);

  // Wait for the next iop to complete, storing the the iop in the given out
  // parameter, and returns true. If waiting fails false is returned. Note that
  // wait will return true even if the iop fails; result of the op will be
  // stored in the op itself. The return value only indicates whether we
  // successfully waited for an op to complete, not whether it completed
  // successfully.
  //
  // See the class comment for details on the discipline you need to use when
  // calling this.
  bool wait_for_next(Duration timeout, Iop **iop_out);

  // Returns the number of iops in this group that haven't been completed yet.
  size_t pending_count() { return pending_count_; }

  // Returns true iff this group has pending operations left.
  bool has_pending() { return pending_count() > 0; }

private:
  friend class Iop;

  // Update the internal state after the given iop has been marked as complete.
  void on_member_complete(Iop *iop);

  // If the load factor of the ops array is too low run through and compact the
  // values, overwriting null entries.
  void maybe_compact_ops();

  std::vector<Iop*> *ops() { return static_cast<std::vector<Iop*>*>(ops_.delegate_); }
};

// Abstract I/O operation. There are iop subtypes for convenience but they can
// safely be cast to the base Iop type, all the information is stored there.
class Iop : public iop_t {
public:
  // Has this iop completed successfully. That is, this will return false either
  // if the op is not yet complete or it is complete but failed.
  bool has_succeeded() { return has_succeeded_; }

  // Has this op completed, successfully or otherwise?
  bool is_complete() { return is_complete_; }

  // Is this a read operation?
  bool is_read() { return type_ == ioRead; }

  bool is_write() { return type_ == ioWrite; }

  // The extra data that was provided at initialization.
  opaque_t extra() { return extra_; }

protected:
  friend class IopGroup;
  Iop(iop_type_t type, opaque_t extra);
  ~Iop();

  // Marks this iop as having completed.
  void mark_complete(bool has_succeeded);

  // Reset the state in preparation for another use of this iop.
  void recycle();

  // Hook platforms can use to do platform-specific recycling.
  void platform_recycle();

  // Returns this iop's group state, creating it if necessary.
  iop_group_state_t *get_or_create_group_state();

  // Returns this iop's group state if it has one, otherwise NULL.
  iop_group_state_t *peek_group_state() { return group_state_; }

  // Returns this op viewed as a write op.
  write_iop_state_t *as_write();

  // Returns this op viewed as a read op.
  read_iop_state_t *as_read();

  // Returns the out stream for this op, which must be a write.
  OutStream *as_out();

  // Returns the in stream for this op, which must be a read.
  InStream *as_in();

  // Returns the stream used by this op, independent of type.
  AbstractStream *stream();

  // What happened when we ensured this iop was scheduled?
  typedef enum {
    // The op is currently scheduled to be performed.
    eoScheduled,
    eoFailedImmediately,
    eoCompletedImmediately
  } ensure_scheduled_outcome_t;

  // Schedules this iop if it hasn't been already. Not used on all platforms.
  ensure_scheduled_outcome_t ensure_scheduled();

  // Schedules a read. Not used on all platforms.
  ensure_scheduled_outcome_t schedule_read(handle_t handle,
      read_iop_state_t *read_iop);

  // Schedules a write. Not used on all platforms.
  ensure_scheduled_outcome_t schedule_write(handle_t handle,
      write_iop_state_t *write_iop);

  // Perform this iop which has already been scheduled by an iop group.
  bool finish_nonblocking();
};

// A write operation.
class WriteIop : public Iop {
public:
  WriteIop(OutStream *out, const void *src, size_t src_size, opaque_t data = o0());

  // Perform this write operation synchronously.
  bool execute();

  // Reuse this iop struct to perform another write of the given data to the
  // same output stream. This op must have completed before this can be
  // called, otherwise if a pending write is still ongoing this can corrupt the
  // pending state.
  void recycle(const void *src, size_t src_size);

  // Reuse this iop struct to perform another write of the same data to the
  // same output stream. This op must have completed before this can be
  // called, otherwise if a pending write is still ongoing this can corrupt the
  // pending state.
  void recycle();

  // Returns the number of bytes written.
  size_t bytes_written() { return as_write()->bytes_written; }

  static inline WriteIop *cast(write_iop_t *c_iop) {
    return static_cast<WriteIop*>(&c_iop->iop);
  }
};

// A read operation.
class ReadIop : public Iop {
public:
  ReadIop(InStream *in, void *dest, size_t dest_size, opaque_t data = o0());

  // Perform this read operation synchronously.
  bool execute();

  // Reuse this iop struct to perform another read into the given buffer from
  // the same input stream. This op must have completed before this can be
  // called, otherwise if a pending read is still ongoing this can corrupt the
  // pending state.
  void recycle(void *dest, size_t dest_size);

  // Reuse this iop struct to perform another read into the same buffer as was
  // used previously. See the other recycle for details.
  void recycle();

  // Returns the number of bytes read.
  size_t bytes_read() { return as_read()->bytes_read_; }

  // Did this read hit the end of the input?
  bool at_eof() { return as_read()->at_eof_; }

  // Returns the given iop viewed as the class type.
  static inline ReadIop *cast(read_iop_t *c_iop) {
    return static_cast<ReadIop*>(&c_iop->iop);
  }
};

// Iop that waits for a process to complete. The process must already have been
// started. Returns true iff waiting succeeded.
class ProcessWaitIop : public Iop {
public:
  ProcessWaitIop(NativeProcess *process, opaque_t extra);

  // Perform this wait operation synchronously.
  bool execute();
};

} // tclib

#endif // _TCLIB_IOP_HH

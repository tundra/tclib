//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_IOP_HH
#define _TCLIB_IOP_HH

#include "c/stdc.h"

#include "c/stdvector.hh"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "utils/duration.h"
#include "io/iop.h"
END_C_INCLUDES

namespace tclib {

class AbstractStream;
class InStream;
class Iop;
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
class IopGroup {
public:
  IopGroup();
  ~IopGroup();

  // Add an iop to this group. See the class comment for how to use this
  // correctly.
  void schedule(Iop *iop);

  // Wait for the next iop to complete, storing the index of the iop in the
  // given out parameter, and returns true. If waiting fails false is returned.
  // Note that wait will return true even if the iop fails; result of the op
  // will be stored in the op itself. The return value only indicates whether
  // we successfully waited for an op to complete, not whether it completed
  // successfully.
  //
  // See the class comment for details on the discipline you need to use when
  // calling this.
  bool wait_for_next(size_t *index_out);

private:
  std::vector<Iop*> ops_;
};

// Abstract I/O operation. There are iop subtypes for convenience but they can
// safely be cast to the base Iop type, all the information is stored there.
class Iop {
public:
  // Has this iop completed successfully. That is, this will return false either
  // if the op is not yet complete or it is complete but failed.
  bool has_succeeded() { return header()->has_succeeded_; }

  // Has this op completed, successfully or otherwise?
  bool is_complete() { return header()->is_complete_; }

  // Is this a read operation?
  bool is_read() { return header()->type_ == ioRead; }

protected:
  friend class IopGroup;
  Iop(iop_type_t type);
  ~Iop();

  // Marks this iop as having completed.
  void mark_complete(bool has_succeeded);

  // Reset the state in preparation for another use of this iop.
  void recycle();

  // Hook platforms can use to do platform-specific recycling.
  void platform_recycle();

  // Returns this iop viewed as a C iop. This only works because of the way
  // the iop structs are arranged so be careful with changing those.
  iop_header_t *header() { return reinterpret_cast<iop_header_t*>(this); }

  // Returns this iop's group state, creating it if necessary.
  iop_group_state_t *get_or_create_group_state();

  // Returns this iop's group state if it has one, otherwise NULL.
  iop_group_state_t *peek_group_state() { return header()->group_state_; }

  // Returns this op viewed as a write op.
  write_iop_t *as_write();

  // Returns this op viewed as a read op.
  read_iop_t *as_read();

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

  // Perform this iop which has already been scheduled by an iop group.
  bool finish_nonblocking();
};

// A write operation.
class WriteIop : public Iop, public write_iop_t {
public:
  WriteIop(OutStream *out, const void *src, size_t src_size);

  // Perform this write operation synchronously.
  bool execute();

  // Reuse this iop struct to perform another write of the given data to the
  // same output stream. This op must have completed before this can be
  // called, otherwise if a pending write is still ongoing this can corrupt the
  // pending state.
  void recycle(const void *src, size_t src_size);

  // Returns the number of bytes written.
  size_t bytes_written() { return as_write()->bytes_written_; }
};

// A read operation.
class ReadIop : public Iop, public read_iop_t {
public:
  ReadIop(InStream *in, void *dest, size_t dest_size);

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
  size_t bytes_read() { return bytes_read_; }

  // Did this read hit the end of the input?
  bool at_eof() { return at_eof_; }
};

} // tclib

#endif // _TCLIB_IOP_HH

//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_IOP_H
#define _TCLIB_IOP_H

#include "c/stdc.h"
#include "io/stream.h"
#include "utils/opaque.h"
#include "utils/vector.h"
#include "sync/process.h"

// State used when issuing iops as a group. The actual contents of this is
// platform dependent.
typedef struct iop_group_state_t iop_group_state_t;
typedef struct iop_t iop_t;

// Marker indicating the type of an iop.
typedef enum {
  ioRead,
  ioWrite,
  ioProcessWait
} iop_type_t;

// A group of iops that can be executed in parallel. See the class comment on
// IopGroup for details.
typedef struct {
  size_t pending_count_;
  // This vector holds pointers to the individual iops. When an iop completes
  // its entry in the vector gets NULL'ed out so the group will ignore it but
  // without requiring the vector to be compacted immediately, which would be
  // expensive. New ops are added at the end, including ops that get recycled,
  // so for each op that gets executed and recycled the ops vector grows. To
  // limit this we compact the vector when the load factor becomes too low.
  voidp_vector_t ops_;
} iop_group_t;

// Initialize an iop group.
void iop_group_initialize(iop_group_t *group);

// Releate the resources held by the given iop group.
void iop_group_dispose(iop_group_t *group);

// Returns the number of iops in this group that haven't been completed yet.
size_t iop_group_pending_count(iop_group_t *group);

// Wait for the next iop to complete, storing the iop in the given out
// parameter, and returns true. If waiting fails false is returned. Note that
// wait will return true even if the iop fails; result of the op will be stored
// in the op itself. The return value only indicates whether we successfully
// waited for an op to complete, not whether it completed successfully.
//
// See the comment on IopGroup for details on the discipline you need to use
// when calling this.
bool iop_group_wait_for_next(iop_group_t *group, duration_t timeout,
    iop_t **iop_out);

// Data associated with a read operation.
typedef struct {
  in_stream_t *in_;
  void *dest_;
  size_t dest_size_;
  size_t bytes_read_;
  bool at_eof_;
} read_iop_state_t;

// Data associated with a write operation.
typedef struct {
  out_stream_t *out;
  const void *src;
  size_t src_size;
  size_t bytes_written;
} write_iop_state_t;

// Data associated with a process-wait operation.
typedef struct {
  native_process_t *process;
  bool has_terminated;
  int exit_code;
} process_wait_iop_state_t;

// A full read or write. To simplify the interface, that is, to avoid having
// two paths for everything, an iop_t can hold both a read and write and can
// be used without knowing which type it is in some circumstances. If you know
// statically which kind of operation you're dealing with you can use read_iop_t
// and write_iop_t instead.
struct iop_t {
  iop_type_t type_;
  bool is_complete_;
  bool has_succeeded_;
  iop_group_t *group_;
  size_t group_index_;
  iop_group_state_t *group_state_;
  opaque_t extra_;
  union {
    read_iop_state_t as_read;
    write_iop_state_t as_write;
    process_wait_iop_state_t as_process_wait;
  } state;
};

// Add an iop to this group. See the comment on IopGroup for how to use this
// correctly. Only incomplete iops may be scheduled, also once an iop has been
// scheduled it may not be completed synchronously.
void iop_group_schedule(iop_group_t *group, iop_t *iop);

// Initializes a generic iop to be a read.
void iop_init_read(iop_t *iop, in_stream_t *in, void *dest, size_t dest_size,
    opaque_t extra);

// Initializes a generic iop to be a write.
void iop_init_write(iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size, opaque_t extra);

// Dispose of the given initialized iop.
void iop_dispose(iop_t *iop);

// Has this iop completed successfully? That is, this will return false either
// if the op is not yet complete or it is complete but failed.
bool iop_has_succeeded(iop_t *iop);

// The extra data that was provided at initialization.
opaque_t iop_extra(iop_t *iop);

// Reuse this iop struct to perform another operation using the same data as
// was used last time the op was performed.
void iop_recycle_same_state(iop_t *iop);

// A read iop.
typedef struct {
  iop_t iop;
} read_iop_t;

// Initialize a read operation.
void read_iop_init(read_iop_t *iop, in_stream_t *in, void *dest, size_t dest_size,
    opaque_t extra);

// Deliver the result of a read to a read iop.
void read_iop_state_deliver(read_iop_state_t *iop, size_t bytes_read, bool at_eof);

// Dispose the given initialized read.
void read_iop_dispose(read_iop_t *iop);

// Reuse this iop struct to perform another read into the given buffer from
// the same input stream. This op must have completed before this can be
// called, otherwise if a pending read is still ongoing this can corrupt the
// pending state.
void read_iop_recycle(read_iop_t *iop, void *dest, size_t dest_size);

// Reuse this iop struct to perform another read into the same buffer as was
// used previously. See the other recycle for details.
void read_iop_recycle_same_state(read_iop_t *iop);

// Did this read hit the end of the input?
bool read_iop_at_eof(read_iop_t *iop);

// Returns the number of bytes read.
size_t read_iop_bytes_read(read_iop_t *iop);

// Returns the extra value that was provided when this iop was initialized.
opaque_t read_iop_extra(read_iop_t *iop);

// Has this iop completed successfully? That is, this will return false either
// if the op is not yet complete or it is complete but failed.
bool read_iop_has_succeeded(read_iop_t *iop);

// Perform this read operation synchronously.
bool read_iop_execute(read_iop_t *iop);

// Does the same as iop_group_schedule but takes a read directly.
void iop_group_schedule_read(iop_group_t *group, read_iop_t *iop);

// Return the given read iop viewed as a generic iop.
static inline iop_t *read_iop_upcast(read_iop_t *read_iop) { return &read_iop->iop; }

// A write iop.
typedef struct {
  iop_t iop;
} write_iop_t;

// Initialize a write operation.
void write_iop_init(write_iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size, opaque_t extra);

// Deliver the result of a write to a write iop.
void write_iop_state_deliver(write_iop_state_t *iop, size_t bytes_written);

// Dispose the given initialized write.
void write_iop_dispose(write_iop_t *iop);

// Perform this write operation synchronously.
bool write_iop_execute(write_iop_t *iop);

// Reuse this iop struct to perform another write of the same data to the
// same output stream. This op must have completed before this can be
// called, otherwise if a pending write is still ongoing this can corrupt the
// pending state.
void write_iop_recycle_same_state(write_iop_t *iop);

// Returns the number of bytes written.
size_t write_iop_bytes_written(write_iop_t *iop);

// Returns the extra value that was provided when this iop was initialized.
opaque_t write_iop_extra(write_iop_t *iop);

// Return the given write iop viewed as a generic iop.
static inline iop_t *write_iop_upcast(write_iop_t* write_iop) { return &write_iop->iop; }

// Add a read to this group. See the comment on IopGroup for how to use this
// correctly. Only incomplete iops may be scheduled, also once an iop has been
// scheduled it may not be completed synchronously.
void iop_group_schedule_write(iop_group_t *group, write_iop_t *iop);

#endif // _TCLIB_IOP_H

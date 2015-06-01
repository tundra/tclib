//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_IOP_H
#define _TCLIB_IOP_H

#include "c/stdc.h"
#include "io/stream.h"
#include "utils/vector.h"

// State used when issuing iops as a group. The actual contents of this is
// platform dependent.
typedef struct iop_group_state_t iop_group_state_t;

// Marker indicating the type of an iop.
typedef enum {
  ioRead, ioWrite
} iop_type_t;

// A group of iops that can be executed in parallel. See the class comment on
// IopGroup for details.
typedef struct {
  size_t pending_count_;
  voidp_vector_t ops_;
} iop_group_t;

// Initialize an iop group.
void iop_group_initialize(iop_group_t *group);

// Releate the resources held by the given iop group.
void iop_group_dispose(iop_group_t *group);

// Returns the number of iops in this group that haven't been completed yet.
size_t iop_group_pending_count(iop_group_t *group);

// Wait for the next iop to complete, storing the index of the iop in the
// given out parameter, and returns true. If waiting fails false is returned.
// Note that wait will return true even if the iop fails; result of the op
// will be stored in the op itself. The return value only indicates whether
// we successfully waited for an op to complete, not whether it completed
// successfully.
//
// See the comment on IopGroup for details on the discipline you need to use
// when calling this.
bool iop_group_wait_for_next(iop_group_t *group, size_t *index_out);

// State shared between read and write iops.
typedef struct {
  iop_type_t type_;
  bool is_complete_;
  bool has_succeeded_;
  iop_group_t *group_;
  iop_group_state_t *group_state_;
} iop_header_t;

// Data associated with a read operation.
typedef struct {
  iop_header_t header_;
  in_stream_t *in_;
  void *dest_;
  size_t dest_size_;
  size_t bytes_read_;
  bool at_eof_;
} read_iop_t;

// Initialize a read operation.
void read_iop_init(read_iop_t *iop, in_stream_t *in, void *dest, size_t dest_size);

// Deliver the result of a read to a read iop.
void read_iop_deliver(read_iop_t *iop, size_t bytes_read, bool at_eof);

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

// Has this iop completed successfully. That is, this will return false either
// if the op is not yet complete or it is complete but failed.
bool read_iop_has_succeeded(read_iop_t *iop);

// Perform this read operation synchronously.
bool read_iop_execute(read_iop_t *iop);

// Add a read to this group. See the comment on IopGroup for how to use this
// correctly. Only incomplete iops may be scheduled, also once an iop has been
// scheduled it may not be completed synchronously.
void iop_group_schedule_read(iop_group_t *group, read_iop_t *iop);

// Data associated with a write operation.
typedef struct {
  iop_header_t header_;
  out_stream_t *out_;
  const void *src_;
  size_t src_size_;
  size_t bytes_written_;
} write_iop_t;

// Initialize a write operation.
void write_iop_init(write_iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size);

// Deliver the result of a write to a write iop.
void write_iop_deliver(write_iop_t *iop, size_t bytes_written);

// Dispose the given initialized write.
void write_iop_dispose(write_iop_t *iop);

// Perform this write operation synchronously.
bool write_iop_execute(write_iop_t *iop);

// Returns the number of bytes written.
size_t write_iop_bytes_written(write_iop_t *iop);

// Add a read to this group. See the comment on IopGroup for how to use this
// correctly. Only incomplete iops may be scheduled, also once an iop has been
// scheduled it may not be completed synchronously.
void iop_group_schedule_write(iop_group_t *group, write_iop_t *iop);

#endif // _TCLIB_IOP_H

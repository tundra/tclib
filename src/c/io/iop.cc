//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "io/iop.hh"

#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

using namespace tclib;

void read_iop_state_deliver(read_iop_state_t *iop, size_t bytes_read, bool at_eof) {
  iop->bytes_read_ = bytes_read;
  iop->at_eof_ = at_eof;
}

void write_iop_state_deliver(write_iop_state_t *iop, size_t bytes_written) {
  iop->bytes_written_ = bytes_written;
}

Iop::Iop(iop_type_t type, opaque_t extra) {
  type_ = type;
  group_state_ = NULL;
  group_ = NULL;
  group_index_ = 0;
  extra_ = extra;
  recycle();
}

void Iop::recycle() {
  is_complete_ = false;
  has_succeeded_ = false;
  if (group_ != NULL)
    static_cast<IopGroup*>(group_)->schedule(this);
  platform_recycle();
}

void iop_init_read(iop_t *iop, in_stream_t *in, void *dest, size_t dest_size,
    opaque_t extra) {
  new (static_cast<ReadIop*>(iop)) ReadIop(static_cast<InStream*>(in), dest,
      dest_size, extra);
}

void read_iop_init(read_iop_t *iop, in_stream_t *in, void *dest, size_t dest_size,
    opaque_t extra) {
  iop_init_read(read_iop_upcast(iop), in, dest, dest_size, extra);
}

void iop_dispose(iop_t *iop) {
  if (iop->type_ == ioRead) {
    static_cast<ReadIop*>(iop)->~ReadIop();
  } else {
    static_cast<WriteIop*>(iop)->~WriteIop();
  }
}

bool iop_has_succeeded(iop_t *iop) {
  return iop->has_succeeded_;
}

opaque_t iop_extra(iop_t *iop) {
  return iop->extra_;
}

void iop_recycle_same_state(iop_t *iop) {
  if (iop->type_ == ioRead) {
    read_iop_recycle_same_state((read_iop_t*) iop);
  } else {
    write_iop_recycle_same_state((write_iop_t*) iop);
  }
}

void read_iop_dispose(read_iop_t *iop) {
  iop_dispose(read_iop_upcast(iop));
}

void read_iop_recycle(read_iop_t *iop, void *dest, size_t dest_size) {
  return ReadIop::cast(iop)->recycle(dest, dest_size);
}

void read_iop_recycle_same_state(read_iop_t *iop) {
  return ReadIop::cast(iop)->recycle();
}

bool read_iop_at_eof(read_iop_t *iop) {
  return ReadIop::cast(iop)->at_eof();
}

opaque_t read_iop_extra(read_iop_t *iop) {
  return ReadIop::cast(iop)->extra();
}

size_t read_iop_bytes_read(read_iop_t *iop) {
  return ReadIop::cast(iop)->bytes_read();
}

bool read_iop_has_succeeded(read_iop_t *iop) {
  return iop_has_succeeded(read_iop_upcast(iop));
}

bool read_iop_execute(read_iop_t *iop) {
  return ReadIop::cast(iop)->execute();
}

void iop_init_write(iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size, opaque_t extra) {
  new (static_cast<WriteIop*>(iop)) WriteIop(static_cast<OutStream*>(out), src,
      src_size, extra);
}

void write_iop_init(write_iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size, opaque_t extra) {
  iop_init_write(write_iop_upcast(iop), out, src, src_size, extra);
}

void write_iop_dispose(write_iop_t *iop) {
  iop_dispose(write_iop_upcast(iop));
}

bool write_iop_execute(write_iop_t *iop) {
  return WriteIop::cast(iop)->execute();
}

void write_iop_recycle_same_state(write_iop_t *iop) {
  return WriteIop::cast(iop)->recycle();
}

size_t write_iop_bytes_written(write_iop_t *iop) {
  return WriteIop::cast(iop)->bytes_written();
}

void iop_group_initialize(iop_group_t *raw_group) {
  IopGroup *group = static_cast<IopGroup*>(raw_group);
  new (group) IopGroup();
}

void iop_group_dispose(iop_group_t *group) {
  static_cast<IopGroup*>(group)->~IopGroup();
}

void iop_group_schedule_read(iop_group_t *group, read_iop_t *iop) {
  iop_group_schedule(group, read_iop_upcast(iop));
}

void iop_group_schedule_write(iop_group_t *group, write_iop_t *iop) {
  iop_group_schedule(group, write_iop_upcast(iop));
}

void iop_group_schedule(iop_group_t *group, iop_t *iop) {
  static_cast<IopGroup*>(group)->schedule(static_cast<Iop*>(iop));
}

size_t iop_group_pending_count(iop_group_t *group) {
  return static_cast<IopGroup*>(group)->pending_count();
}

bool iop_group_wait_for_next(iop_group_t *group, duration_t timeout,
    iop_t **iop_out) {
  Iop *cpp_iop_out = NULL;
  bool result = static_cast<IopGroup*>(group)->wait_for_next(timeout, &cpp_iop_out);
  *iop_out = cpp_iop_out;
  return result;
}

IopGroup::IopGroup() {
  pending_count_ = 0;
  voidp_vector_init(&ops_);
}

IopGroup::~IopGroup() {
  CHECK_EQ("disposing active group", 0, pending_count());
  voidp_vector_dispose(&ops_);
}

void IopGroup::schedule(Iop *iop) {
  CHECK_FALSE("scheduling complete iop", iop->is_complete());
  CHECK_TRUE("invalid reschedule", iop->group_ == NULL || iop->group_ == this);
  iop->group_index_ = ops()->size();
  iop->group_ = this;
  ops()->push_back(iop);
  pending_count_++;
}

void IopGroup::maybe_compact_ops() {
  if (pending_count_ == 0 && !ops()->empty()) {
    // If we reach zero pending compacting is trivial.
    ops()->clear();
    return;
  }
  // Check the load factor.
  double factor = static_cast<double>(pending_count_) / static_cast<double>(ops()->size());
  if (factor > 0.3)
    return;
  // Load factor is too low, compact.
  size_t pending_seen = 0;
  for (size_t i = 0; i < ops()->size(); i++) {
    Iop *op = ops()->at(i);
    if (op != NULL) {
      CHECK_EQ("unaligned op", i, op->group_index_);
      op->group_index_ = pending_seen;
      // Note that this may overwrite an op with itself, which is fine.
      ops()->at(pending_seen) = op;
      pending_seen++;
    }
  }
  CHECK_EQ("pending unaligned", pending_count_, pending_seen);
  ops()->resize(pending_count_);
}

void IopGroup::on_member_complete(Iop *iop) {
  CHECK_TRUE("inconsistent iop complete", this == iop->group_);
  size_t index = iop->group_index_;
  CHECK_TRUE("invalid index", ops()->at(index) == iop);
  ops()->at(index) = NULL;
  iop->group_index_ = 0;
  pending_count_--;
  maybe_compact_ops();
}

write_iop_state_t *Iop::as_write() {
  CHECK_FALSE("read as write", is_read());
  return &state.as_write;
}

read_iop_state_t *Iop::as_read() {
  CHECK_TRUE("write as read", is_read());
  return &state.as_read;
}

InStream *Iop::as_in() {
  return static_cast<InStream*>(as_read()->in_);
}

OutStream *Iop::as_out() {
  return static_cast<OutStream*>(as_write()->out_);
}

AbstractStream *Iop::stream() {
  return is_read() ? static_cast<AbstractStream*>(as_in()) : as_out();
}

void Iop::mark_complete(bool has_succeeded) {
  CHECK_FALSE("already complete", is_complete_);
  is_complete_ = true;
  has_succeeded_ = has_succeeded;
  if (group_ != NULL)
    static_cast<IopGroup*>(group_)->on_member_complete(this);
}

ReadIop::ReadIop(InStream *in, void *dest, size_t dest_size, opaque_t extra)
  : Iop(ioRead, extra) {
  state.as_read.in_ = in;
  recycle(dest, dest_size);
}

bool ReadIop::execute() {
  CHECK_FALSE("re-execution", is_complete());
  CHECK_TRUE("executing group iop", group_ == NULL);
  bool result = as_in()->read_sync(&state.as_read);
  mark_complete(result);
  return result;
}

void ReadIop::recycle(void *dest, size_t dest_size) {
  Iop::recycle();
  state.as_read.dest_ = dest;
  state.as_read.dest_size_ = dest_size;
  state.as_read.at_eof_ = false;
  state.as_read.bytes_read_ = 0;
}

void ReadIop::recycle() {
  recycle(as_read()->dest_, as_read()->dest_size_);
}

WriteIop::WriteIop(OutStream *out, const void *src, size_t src_size, opaque_t extra)
  : Iop(ioWrite, extra) {
  state.as_write.out_ = out;
  recycle(src, src_size);
}

bool WriteIop::execute() {
  CHECK_FALSE("re-execution", is_complete());
  CHECK_TRUE("executing group iop", group_ == NULL);
  bool result = as_out()->write_sync(&state.as_write);
  mark_complete(result);
  return result;
}

void WriteIop::recycle(const void *src, size_t src_size) {
  Iop::recycle();
  state.as_write.src_ = src;
  state.as_write.src_size_ = src_size;
  state.as_write.bytes_written_ = 0;
}

void WriteIop::recycle() {
  recycle(state.as_write.src_, state.as_write.src_size_);
}

#ifdef IS_GCC
#include "iop-posix.cc"
#endif

#ifdef IS_MSVC
#include "iop-msvc.cc"
#endif

Iop::~Iop() {
  CHECK_TRUE("incomplete iop", is_complete_);
  delete peek_group_state();
}


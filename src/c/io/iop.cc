//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "io/iop.hh"

#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

using namespace tclib;

void read_iop_deliver(read_iop_t *iop, size_t bytes_read, bool at_eof) {
  iop->bytes_read_ = bytes_read;
  iop->at_eof_ = at_eof;
}

void write_iop_deliver(write_iop_t *iop, size_t bytes_written) {
  iop->bytes_written_ = bytes_written;
}

Iop::Iop(iop_type_t type) {
  header()->type_ = type;
  header()->group_state_ = NULL;
  header()->group_ = NULL;
  recycle();
}

void Iop::recycle() {
  header()->is_complete_ = false;
  header()->has_succeeded_ = false;
  iop_group_t *group = header()->group_;
  if (group != NULL)
    group->pending_count_++;
  platform_recycle();
}

void read_iop_init(read_iop_t *iop, in_stream_t *in, void *dest, size_t dest_size) {
  new (static_cast<ReadIop*>(iop)) ReadIop(static_cast<InStream*>(in), dest, dest_size);
}

void read_iop_dispose(read_iop_t *iop) {
  static_cast<ReadIop*>(iop)->~ReadIop();
}

void read_iop_recycle(read_iop_t *iop, void *dest, size_t dest_size) {
  return static_cast<ReadIop*>(iop)->recycle(dest, dest_size);
}

void read_iop_recycle_same_state(read_iop_t *iop) {
  return static_cast<ReadIop*>(iop)->recycle();
}

bool read_iop_at_eof(read_iop_t *iop) {
  return static_cast<ReadIop*>(iop)->at_eof();
}

size_t read_iop_bytes_read(read_iop_t *iop) {
  return static_cast<ReadIop*>(iop)->bytes_read();
}

bool read_iop_has_succeeded(read_iop_t *iop) {
  return static_cast<ReadIop*>(iop)->has_succeeded();
}

bool read_iop_execute(read_iop_t *iop) {
  return static_cast<ReadIop*>(iop)->execute();
}

void write_iop_init(write_iop_t *iop, out_stream_t *out, const void *src,
    size_t src_size) {
  new (static_cast<WriteIop*>(iop)) WriteIop(static_cast<OutStream*>(out), src,
      src_size);
}

void write_iop_dispose(write_iop_t *iop) {
  static_cast<WriteIop*>(iop)->~WriteIop();
}

bool write_iop_execute(write_iop_t *iop) {
  return static_cast<WriteIop*>(iop)->execute();
}

size_t write_iop_bytes_written(write_iop_t *iop) {
  return static_cast<WriteIop*>(iop)->bytes_written();
}

void iop_group_initialize(iop_group_t *raw_group) {
  IopGroup *group = static_cast<IopGroup*>(raw_group);
  new (group) IopGroup();
}

void iop_group_dispose(iop_group_t *group) {
  static_cast<IopGroup*>(group)->~IopGroup();
}

void iop_group_schedule(iop_group_t *group, void *iop) {
  static_cast<IopGroup*>(group)->schedule(static_cast<Iop*>(iop));
}

size_t iop_group_pending_count(iop_group_t *group) {
  return static_cast<IopGroup*>(group)->pending_count();
}

bool iop_group_wait_for_next(iop_group_t *group, size_t *index_out) {
  return static_cast<IopGroup*>(group)->wait_for_next(index_out);
}

IopGroup::IopGroup() {
  pending_count_ = 0;
  voidp_vector_init(&ops_);
}

IopGroup::~IopGroup() {
  CHECK_EQ("disposing active group", 0, pending_count_);
  voidp_vector_dispose(&ops_);
}

void IopGroup::schedule(Iop *iop) {
  CHECK_FALSE("scheduling complete iop", iop->is_complete());
  CHECK_TRUE("rescheduling iop", iop->header()->group_ == NULL);
  ops()->push_back(iop);
  iop->header()->group_ = this;
  pending_count_++;
}

write_iop_t *Iop::as_write() {
  CHECK_FALSE("read as write", is_read());
  return reinterpret_cast<write_iop_t*>(this);
}

read_iop_t *Iop::as_read() {
  CHECK_TRUE("write as read", is_read());
  return reinterpret_cast<read_iop_t*>(this);
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
  CHECK_FALSE("already complete", header()->is_complete_);
  header()->is_complete_ = true;
  header()->has_succeeded_ = has_succeeded;
  iop_group_t *group = header()->group_;
  if (group != NULL)
    group->pending_count_--;
}

ReadIop::ReadIop(InStream *in, void *dest, size_t dest_size)
  : Iop(ioRead) {
  in_ = in;
  recycle(dest, dest_size);
}

bool ReadIop::execute() {
  CHECK_FALSE("re-execution", is_complete());
  CHECK_TRUE("executing group iop", header()->group_ == NULL);
  bool result = as_in()->read_sync(this);
  mark_complete(result);
  return result;
}

void ReadIop::recycle(void *dest, size_t dest_size) {
  Iop::recycle();
  dest_ = dest;
  dest_size_ = dest_size;
  at_eof_ = false;
  bytes_read_ = 0;
}

void ReadIop::recycle() {
  recycle(as_read()->dest_, as_read()->dest_size_);
}

WriteIop::WriteIop(OutStream *out, const void *src, size_t src_size)
  : Iop(ioWrite) {
  out_ = out;
  recycle(src, src_size);
}

bool WriteIop::execute() {
  CHECK_FALSE("re-execution", is_complete());
  CHECK_TRUE("executing group iop", header()->group_ == NULL);
  bool result = as_out()->write_sync(this);
  mark_complete(result);
  return result;
}

void WriteIop::recycle(const void *src, size_t src_size) {
  Iop::recycle();
  src_ = src;
  src_size_ = src_size;
  bytes_written_ = 0;
}

void WriteIop::recycle() {
  recycle(src_, src_size_);
}

#ifdef IS_GCC
#include "iop-posix.cc"
#endif

#ifdef IS_MSVC
#include "iop-msvc.cc"
#endif

Iop::~Iop() {
  CHECK_TRUE("incomplete iop", header()->is_complete_);
  delete peek_group_state();
}


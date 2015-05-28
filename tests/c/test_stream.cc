//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "io/iop.hh"
#include "io/stream.hh"

using namespace tclib;

TEST(stream, byte_in_stream_cpp) {
  byte_t data[12] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  ByteInStream in(data, 12);
  byte_t buf[4] = {0, 0, 0, 0};
  ReadIop iop(&in, buf, 1);
  ASSERT_FALSE(iop.at_eof());
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(1, iop.read_size());
  ASSERT_EQ(11, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(iop.at_eof());
  iop.recycle(buf, 2);
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(2, iop.read_size());
  ASSERT_EQ(10, buf[0]);
  ASSERT_EQ(9, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(iop.at_eof());
  iop.recycle(buf, 3);
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(3, iop.read_size());
  ASSERT_EQ(8, buf[0]);
  ASSERT_EQ(7, buf[1]);
  ASSERT_EQ(6, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(iop.at_eof());
  iop.recycle(buf, 4);
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(4, iop.read_size());
  ASSERT_EQ(5, buf[0]);
  ASSERT_EQ(4, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_FALSE(in.at_eof());
  iop.recycle();
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(2, iop.read_size());
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_TRUE(iop.at_eof());
  iop.recycle();
  ASSERT_TRUE(iop.exec_sync());
  ASSERT_EQ(0, iop.read_size());
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
}

TEST(stream, byte_in_stream_c) {
  byte_t data[12] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  in_stream_t *in = byte_in_stream_open(data, 12);
  byte_t buf[4] = {0, 0, 0, 0};
  read_iop_t iop;
  read_iop_init(&iop, in, buf, 1);
  ASSERT_FALSE(read_iop_at_eof(&iop));
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(1, read_iop_read_size(&iop));
  ASSERT_EQ(11, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(read_iop_at_eof(&iop));
  read_iop_recycle(&iop, buf, 2);
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(2, read_iop_read_size(&iop));
  ASSERT_EQ(10, buf[0]);
  ASSERT_EQ(9, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(read_iop_at_eof(&iop));
  read_iop_recycle(&iop, buf, 3);
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(3, read_iop_read_size(&iop));
  ASSERT_EQ(8, buf[0]);
  ASSERT_EQ(7, buf[1]);
  ASSERT_EQ(6, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(read_iop_at_eof(&iop));
  read_iop_recycle(&iop, buf, 4);
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(4, read_iop_read_size(&iop));
  ASSERT_EQ(5, buf[0]);
  ASSERT_EQ(4, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_FALSE(read_iop_at_eof(&iop));
  read_iop_recycle_same_state(&iop);
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(2, read_iop_read_size(&iop));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_TRUE(read_iop_at_eof(&iop));
  read_iop_recycle_same_state(&iop);
  ASSERT_TRUE(read_iop_exec_sync(&iop));
  ASSERT_EQ(0, read_iop_read_size(&iop));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  read_iop_dispose(&iop);
  byte_in_stream_destroy(in);
}

TEST(stream, byte_out_stream) {
  ByteOutStream out;
  byte_t data[4] = {11, 0, 0, 0};
  ASSERT_EQ(1, out.write_bytes(data, 1));
  ASSERT_EQ(1, out.size());
  data[0] = 10;
  data[1] = 9;
  ASSERT_EQ(2, out.write_bytes(data, 2));
  ASSERT_EQ(3, out.size());
  data[0] = 8;
  data[1] = 7;
  data[2] = 6;
  ASSERT_EQ(3, out.write_bytes(data, 3));
  ASSERT_EQ(6, out.size());
  data[0] = 5;
  data[1] = 4;
  data[2] = 3;
  data[3] = 2;
  ASSERT_EQ(4, out.write_bytes(data, 4));
  ASSERT_EQ(10, out.size());
  for (size_t i = 0; i < 10; i++)
    ASSERT_EQ(11 - i, out.data()[i]);
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(stream, msvc_stuff) {
#if defined(IS_MSVC)
  ASSERT_PTREQ(AbstractStream::kNullNakedFileHandle, INVALID_HANDLE_VALUE);
#endif
}

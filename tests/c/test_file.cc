//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "io/file.hh"

using namespace tclib;

TEST(file, byte_in_stream_cpp) {
  byte_t data[12] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  ByteInStream in(data, 12);
  byte_t buf[4] = {0, 0, 0, 0};
  ASSERT_FALSE(in.at_eof());
  ASSERT_EQ(1, in.read_bytes(buf, 1));
  ASSERT_EQ(11, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in.at_eof());
  ASSERT_EQ(2, in.read_bytes(buf, 2));
  ASSERT_EQ(10, buf[0]);
  ASSERT_EQ(9, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in.at_eof());
  ASSERT_EQ(3, in.read_bytes(buf, 3));
  ASSERT_EQ(8, buf[0]);
  ASSERT_EQ(7, buf[1]);
  ASSERT_EQ(6, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in.at_eof());
  ASSERT_EQ(4, in.read_bytes(buf, 4));
  ASSERT_EQ(5, buf[0]);
  ASSERT_EQ(4, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_FALSE(in.at_eof());
  ASSERT_EQ(2, in.read_bytes(buf, 4));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_TRUE(in.at_eof());
  ASSERT_EQ(0, in.read_bytes(buf, 4));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
}

TEST(file, byte_in_stream_c) {
  byte_t data[12] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  in_stream_t *in = byte_in_stream_open(data, 12);
  byte_t buf[4] = {0, 0, 0, 0};
  ASSERT_FALSE(in_stream_at_eof(in));
  ASSERT_EQ(1, in_stream_read_bytes(in, buf, 1));
  ASSERT_EQ(11, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in_stream_at_eof(in));
  ASSERT_EQ(2, in_stream_read_bytes(in, buf, 2));
  ASSERT_EQ(10, buf[0]);
  ASSERT_EQ(9, buf[1]);
  ASSERT_EQ(0, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in_stream_at_eof(in));
  ASSERT_EQ(3, in_stream_read_bytes(in, buf, 3));
  ASSERT_EQ(8, buf[0]);
  ASSERT_EQ(7, buf[1]);
  ASSERT_EQ(6, buf[2]);
  ASSERT_EQ(0, buf[3]);
  ASSERT_FALSE(in_stream_at_eof(in));
  ASSERT_EQ(4, in_stream_read_bytes(in, buf, 4));
  ASSERT_EQ(5, buf[0]);
  ASSERT_EQ(4, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_FALSE(in_stream_at_eof(in));
  ASSERT_EQ(2, in_stream_read_bytes(in, buf, 4));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  ASSERT_TRUE(in_stream_at_eof(in));
  ASSERT_EQ(0, in_stream_read_bytes(in, buf, 4));
  ASSERT_EQ(1, buf[0]);
  ASSERT_EQ(0, buf[1]);
  ASSERT_EQ(3, buf[2]);
  ASSERT_EQ(2, buf[3]);
  byte_in_stream_dispose(in);
}

TEST(file, byte_out_stream) {
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

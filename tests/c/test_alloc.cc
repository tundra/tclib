//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
END_C_INCLUDES

typedef struct {
  int64_t x;
  int64_t y;
} point_t;

TEST(alloc, zero) {
  point_t p0 = {10000000, 10000001};
  point_t p1 = {20000000, 20000001};
  point_t p2 = {30000000, 30000001};
  ASSERT_EQ(10000000, p0.x);
  ASSERT_EQ(10000001, p0.y);
  ASSERT_EQ(20000000, p1.x);
  ASSERT_EQ(20000001, p1.y);
  ASSERT_EQ(30000000, p2.x);
  ASSERT_EQ(30000001, p2.y);
  struct_zero_fill(p0);
  ASSERT_EQ(0, p0.x);
  ASSERT_EQ(0, p0.y);
  ASSERT_EQ(20000000, p1.x);
  ASSERT_EQ(20000001, p1.y);
  ASSERT_EQ(30000000, p2.x);
  ASSERT_EQ(30000001, p2.y);
  p0.x = 10000002;
  p0.y = 10000003;
  struct_zero_fill(p1);
  ASSERT_EQ(10000002, p0.x);
  ASSERT_EQ(10000003, p0.y);
  ASSERT_EQ(0, p1.x);
  ASSERT_EQ(0, p1.y);
  ASSERT_EQ(30000000, p2.x);
  ASSERT_EQ(30000001, p2.y);
  p1.x = 20000002;
  p1.y = 20000003;
  struct_zero_fill(p2);
  ASSERT_EQ(10000002, p0.x);
  ASSERT_EQ(10000003, p0.y);
  ASSERT_EQ(20000002, p1.x);
  ASSERT_EQ(20000003, p1.y);
  ASSERT_EQ(0, p2.x);
  ASSERT_EQ(0, p2.y);
}

static blob_t no_alloc(allocator_t *self, size_t size) {
  return blob_empty();
}

TEST(alloc, malloc_struct) {
  point_t *p = allocator_default_malloc_struct(point_t);
  p->x = 978;
  p->y = 434;
  ASSERT_EQ(978, p->x);
  ASSERT_EQ(434, p->y);
  allocator_default_free_struct(point_t, p);

  allocator_t blocked = {no_alloc, NULL};
  allocator_t *prev = allocator_set_default(&blocked);
  ASSERT_PTREQ(NULL, allocator_default_malloc_struct(point_t));
  allocator_set_default(prev);
}

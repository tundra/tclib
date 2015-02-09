//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdhashmap.hh"
#include "test/unittest.hh"

TEST(stdhashmap, realsimple) {
  platform_hash_map<int, int> mymap;
  mymap[1] = 2;
  ASSERT_EQ(1, mymap.size());
  ASSERT_TRUE(mymap[1] == 2);
}

TEST(stdhashmap, hasher) {
  platform_hash<const char*> hasher;
  hasher("Alley-oop!");
}

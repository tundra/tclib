//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "stdc.h"

#ifdef IS_MSVC
#  pragma warning(push, 0)
#    include <hash_map>
#  pragma warning(pop)
#  define platform_hash_map std::hash_map
#else
// I'm pretty sure this is not how you want to do this but a cursory google
// search didn't give a good answer to what you're really supposed to do it and
// this works. Yucky though.
#  define _GLIBCXX_PERMIT_BACKWARD_HASH
#  include <hash_map>
#  define platform_hash_map __gnu_cxx::hash_map
#endif

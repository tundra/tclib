//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "stdc.h"

#if defined(IS_MSVC)
#  define _HAS_EXCEPTIONS 0
#  pragma warning(push, 0)
#    include <hash_map>
#  pragma warning(pop)
#  define platform_hash_map std::hash_map
#  define platform_hash std::hash
#elif defined(IS_MACH)
#  include <unordered_map>
#  define platform_hash_map std::unordered_map
#  define platform_hash std::hash
#else
// I'm pretty sure this is not how you want to do this but a cursory google
// search didn't give a good answer to what you're really supposed to do it and
// this works. Yucky though.
// this works. Yucky though. It would be nicer to use unordered_map but on the
// default compiler on my ubuntu that requires c++11 and extra compiler flags
// which this doesn't.
#  define _GLIBCXX_PERMIT_BACKWARD_HASH
#  include <hash_map>
#  define platform_hash_map __gnu_cxx::hash_map
#  include <hash_fun.h>
#  define platform_hash __gnu_cxx::hash
#endif

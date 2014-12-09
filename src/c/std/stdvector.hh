//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "stdc.h"

#ifdef IS_MSVC
#  define _HAS_EXCEPTIONS 0
#  pragma warning(push, 0)
#    include <vector>
#  pragma warning(pop)
#else
#  include <vector>
#endif

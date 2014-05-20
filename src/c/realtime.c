//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "realtime.h"

#ifdef IS_GCC
#include "realtime-posix.c"
#endif

#ifdef IS_MSVC
#include "realtime-msvc.c"
#endif

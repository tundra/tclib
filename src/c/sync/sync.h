//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Platform-specific definitions.

#ifndef _TCLIB_SYNC_H
#define _TCLIB_SYNC_H

#include "c/stdc.h"
#include "utils/duration.h"

#ifdef IS_GCC
# ifdef IS_MACH
#   include "sync-mach.h"
# else
#   include "sync-posix.h"
# endif
#endif

#ifdef IS_MSVC
# include "sync-msvc.h"
#endif

#endif // _TCLIB_SYNC_H

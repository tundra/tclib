//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Platform-specific definitions.

#ifndef _TCLIB_SYNC_H
#define _TCLIB_SYNC_H

#include "stdc.h"

#ifdef IS_GCC
#include "sync-posix.h"
#endif

#ifdef IS_MSVC
#include "sync-msvc.h"
#endif

#endif // _TCLIB_SYNC_H

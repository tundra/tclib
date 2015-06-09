//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <mach/semaphore.h>
#include <mach/mach.h>

#include "sync-posix.h"

// This is yucky but this isn't a public header so I'll allow it.
#define platform_semaphore_t mach_platform_semaphore_t
typedef semaphore_t mach_platform_semaphore_t;

#undef platform_time_t
#define platform_time_t struct mach_timespec

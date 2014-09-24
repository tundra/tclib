//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// It's tempting but don't include windows.h. It drags in so much funkiness that
// it's better to only include it in windows-specific .c/.cc files.

struct platform_thread_t {
  void *handle_;
  void *result_;
};

typedef int32_t native_thread_id_t;
#define kPlatformThreadInit {INVALID_HANDLE_VALUE, 0}
#define PLATFORM_THREAD_ENTRY_POINT unsigned long __stdcall entry_point(void *data)

typedef void *platform_mutex_t;
#define kPlatformMutexInit INVALID_HANDLE_VALUE

typedef void *platform_semaphore_t;
#define kPlatformSemaphoreInit INVALID_HANDLE_VALUE

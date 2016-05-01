//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// It's tempting but don't include windows.h. It drags in so much funkiness that
// it's better to only include it in windows-specific .c/.cc files.

typedef void *platform_thread_t;

typedef int32_t native_thread_id_t;
#define kPlatformThreadInit INVALID_HANDLE_VALUE
#define PLATFORM_THREAD_ENTRY_POINT unsigned long __stdcall entry_point(void *data)

typedef byte_t platform_mutex_t[IF_32_BIT(24, 40)];
#define kPlatformMutexChecksConsistency false
#define get_platform_mutex(MUTEX) (reinterpret_cast<PCRITICAL_SECTION>(&(MUTEX)->mutex))

typedef void *platform_semaphore_t;
#define kPlatformSemaphoreInit INVALID_HANDLE_VALUE

typedef byte_t platform_condition_t[8];
#define get_platform_condition(COND) (reinterpret_cast<PCONDITION_VARIABLE>(&(COND)->cond))

typedef struct {
  void *read_;
  void *write_;
} platform_pipe_t;

typedef uint32_t platform_time_t;

typedef void *platform_process_t;
#define kPlatformProcessInit INVALID_HANDLE_VALUE

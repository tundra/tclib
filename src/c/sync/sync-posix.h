//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef pthread_t native_thread_id_t;
typedef pthread_t platform_thread_t;
#define kPlatformThreadInit 0
#define PLATFORM_THREAD_ENTRY_POINT void *entry_point(void *data)

typedef pthread_mutex_t platform_mutex_t;
#define kPlatformMutexInit PTHREAD_MUTEX_INITIALIZER
#define kPlatformMutexChecksConsistency true

typedef sem_t platform_semaphore_t;

typedef pthread_cond_t platform_condition_t;

typedef int platform_pipe_t[2];

#define platform_time_t struct timespec

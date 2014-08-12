//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_THREAD_H
#define _TCLIB_THREAD_H

#include "stdc.h"

struct native_thread_t;

native_thread_t *new_native_thread(void *(callback)(void*), void *data);

void dispose_native_thread(native_thread_t *thread);

bool native_thread_start(native_thread_t *thread);

void *native_thread_join(native_thread_t *thread);

#endif // _TCLIB_THREAD_H

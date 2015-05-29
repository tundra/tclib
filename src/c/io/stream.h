//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STREAM_H
#define _TCLIB_STREAM_H

#include "c/stdc.h"
#include <stdarg.h>
#include "utils/string.h"

// The windows handle type. It's convenient to have it available to make the
// intention of a particular void* clear but without having to import windows.h.
typedef void *handle_t;

// A handle for a stream from which bytes can be read.
typedef struct in_stream_t in_stream_t;

// A handle for a stream to which bytes can be written.
typedef struct out_stream_t out_stream_t;

// Works just like normal printf, it just writes to this file.
size_t out_stream_printf(out_stream_t *file, const char *fmt, ...);

// Works just like vprintf, it just writes to this file.
size_t out_stream_vprintf(out_stream_t *file, const char *fmt, va_list argp);

// Flushes any buffered writes to the given stream.
bool out_stream_flush(out_stream_t *stream);

// Returns an in stream that returns bytes from the given block.
in_stream_t *byte_in_stream_open(const void *data, size_t size);

// Disposes the given byte input stream.
void byte_in_stream_destroy(in_stream_t *stream);

#endif // _TCLIB_STREAM_H

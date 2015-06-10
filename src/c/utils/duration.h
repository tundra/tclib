//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// A few abstractions for working with time.

#ifndef _UTILS_DURATION_H
#define _UTILS_DURATION_H

#include "c/stdc.h"

// A period of time, possibly unlimited.
typedef struct {
  bool is_unlimited;
  uint64_t millis;
} duration_t;

// Returns a duration that represents the given number of milliseconds.
static inline duration_t duration_millis(uint64_t value) {
  duration_t result = {false, value};
  return result;
}

// Returns a duration that represents the given number of seconds.
static inline duration_t duration_seconds(double value) {
  return duration_millis((uint64_t) (value * 1000));
}

// Returns the unlimited duration.
static inline duration_t duration_unlimited() {
  duration_t result = {true, 0};
  return result;
}

// Returns the instant duration, that is, zero time.
static inline duration_t duration_instant() {
  duration_t result = {false, 0};
  return result;
}

// Is the given value the unlimited duration?
static inline bool duration_is_unlimited(duration_t duration) {
  return duration.is_unlimited;
}

// Is the given duration the instant duration, that is, zero time?
static inline bool duration_is_instant(duration_t duration) {
  return !duration.is_unlimited && duration.millis == 0;
}

// Returns the number of seconds of this duration.
static inline double duration_to_seconds(duration_t duration) {
  return ((double) duration.millis) / 1000.0;
}

// Returns the number of milliseconds of this duration. If the given duration is
// unlimited or instant 0 will be returned (which means that you probably want
// to test explicitly for unlimited).
static inline uint64_t duration_to_millis(duration_t duration) {
  return duration.millis;
}

// Adds the given duration to a pair of sec/nsec which is what you need to add
// a duration to a posix timespec. For unlimited durations this does nothing.
void duration_add_to_timespec(duration_t duration, uint64_t *sec, uint64_t *nsec);

// Adds the given duration to a pair of sec/usec which is what you need to add
// a duration to a posix timeval. For unlimited durations this does nothing.
void duration_add_to_timeval(duration_t duration, uint64_t *sec, uint64_t *usec);

// Return a timeval that represents the same period as the given duration. For
// unlimited durations returns the zero timeval.
IF_GCC(struct timeval duration_to_timeval(duration_t duration);,)

#endif // _UTILS_DURATION_H

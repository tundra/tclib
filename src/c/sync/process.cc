//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/process.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/strbuf.h"
END_C_INCLUDES

using namespace tclib;

NativeProcess::NativeProcess()
  : stdout_(NULL) {
#if defined(kPlatformProcessInit)
  process = kPlatformProcessInit;
#endif
  state = nsInitial;
  result = -1;
  platform_initialize();
}

NativeProcess::~NativeProcess() {
  if (state != nsInitial)
    platform_dispose();
}

bool NativeProcess::set_env(const char *key, const char *value) {
  string_buffer_t buf;
  string_buffer_init(&buf);
  string_buffer_printf(&buf, "%s=%s", key, value);
  utf8_t raw_binding = string_buffer_flush(&buf);
  std::string binding(raw_binding.chars, raw_binding.size);
  env_.push_back(binding);
  string_buffer_dispose(&buf);
  return true;
}

#ifdef IS_GCC
#include "process-posix.cc"
#endif

#ifdef IS_MSVC
#include "process-msvc.cc"
#endif

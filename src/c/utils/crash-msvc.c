//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"
#include <Dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void initialize_crash_handler() {
  SymInitialize(GetCurrentProcess(), NULL, true);
}

// Print a stack trace if the platform supports it.
void print_stack_trace(FILE *out, int signum) {
  handle_t process = GetCurrentProcess();
  if (!SymRefreshModuleList(process)) {
    fprintf(out, "Error refreshing module list: %i", GetLastError());
    return;
  }

  fprintf(out, "# Received condition %i\n", signum);

  // Capture the stack trace.
  static const size_t kMaxStackSize = 32;
  void *backtrace[kMaxStackSize];
  size_t frame_count = CaptureStackBackTrace(0, kMaxStackSize, backtrace, NULL);

  // Scan through the trace one frame at a time, resolving symbols as we go.
  static const size_t kMaxNameLength = 128;
  static const size_t kSymbolInfoSize = sizeof(SYMBOL_INFO) + (kMaxNameLength * sizeof(char_t));
  // A SYMBOL_INFO is variable size so we stack allocate a blob of memory and
  // cast it rather than stack allocate the info directly.
  uint8_t symbol_info_bytes[kSymbolInfoSize];
  ZeroMemory(symbol_info_bytes, kSymbolInfoSize);
  SYMBOL_INFO *info = reinterpret_cast<SYMBOL_INFO*>(symbol_info_bytes);
  // This isn't strictly true, it's symbol_info_bytes, but SymFromAddr requires
  // it to have this value.
  info->SizeOfStruct = sizeof(SYMBOL_INFO);
  info->MaxNameLen = kMaxNameLength;
  for (size_t i = 0; i < frame_count; i++) {
    void *addr = backtrace[i];
    DWORD64 addr64 = reinterpret_cast<DWORD64>(addr);
    if (SymFromAddr(process, addr64, 0, info)) {
      fprintf(out, "# - 0x%p: %s\n", addr, info->Name);
    } else {
      fprintf(out, "# - 0x%p\n", addr);
    }
  }
}

void propagate_condition(int signum) {
  // It looks like maybe this isn't necessary on windows?
}
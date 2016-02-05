//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_LOG
#define _TCLIB_UTILS_LOG

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

namespace tclib {

// C++ wrapper around a log.
class Log : public log_o {
public:
  Log();

  // Implicitly uninstalls this log if it is installed.
  virtual ~Log() { ensure_uninstalled(); }

  // Override this to implement your custom logging behavior.
  virtual bool record(log_entry_t *entry) = 0;

  // If this log is not already installed, installs it. If it is installed
  // nothing happens.
  void ensure_installed();

  // If this log is installed, uninstalls it. If it is installed it must be the
  // top one.
  void ensure_uninstalled();

  // Is this log installed?
  bool is_installed() { return outer_ != NULL; }

protected:
  // Passed the given entry on to the enclosing logger.
  bool propagate(log_entry_t *entry);

private:
  static bool log_trampoline(log_o *self, log_entry_t *entry);
  static log_o_vtable_t kVTable;
  log_o *outer_;
};

} // namespace tclib

#endif // _TCLOB_UTILS_LOG

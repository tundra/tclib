//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_LOG
#define _TCLIB_UTILS_LOG

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

namespace tclib {

class Log : public log_o {
public:
  Log();
  virtual ~Log() { ensure_uninstalled(); }
  virtual bool record(log_entry_t *entry) = 0;
  void ensure_installed();
  void ensure_uninstalled();
  bool is_installed() { return outer_ != NULL; }

protected:
  bool propagate(log_entry_t *entry);

private:
  static bool log_trampoline(log_o *self, log_entry_t *entry);
  static log_o_vtable_t kVTable;
  log_o *outer_;
};

} // namespace tclib

#endif // _TCLOB_UTILS_LOG

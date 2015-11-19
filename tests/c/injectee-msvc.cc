//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

static void on_attach() {
  int a = atoi(getenv("A"));
  int b = atoi(getenv("B"));
  char buf[256];
  sprintf(buf, "C=%i", a + b);
  _putenv(buf);
}

static void on_detach() {
  _putenv("C=1234567");
}

// A dll that, when attached,
bool APIENTRY DllMain(module_t module, dword_t reason, void *reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      on_attach();
      break;
    case DLL_PROCESS_DETACH:
      on_detach();
      break;
  }
  return true;
}

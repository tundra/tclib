//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"
#include "sync/injectee.hh"
#include "utils/alloc.hh"
#include "sync/pipe.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

static void on_attach() {
  const char *a_str = getenv("A");
  if (a_str == NULL)
    return;
  int a = atoi(a_str);
  const char *b_str = getenv("B");
  if (b_str == NULL)
    return;
  int b = atoi(b_str);
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

CONNECTOR_IMPL(TestInjecteeConnect, data_in, data_out) {
  if (data_in.size != 85)
    return 0x1085;
  uint8_t *bytes_in = static_cast<uint8_t*>(data_in.start);
  uint8_t a = bytes_in[0];
  uint8_t b = bytes_in[1];
  for (size_t i = 2; i < data_in.size; i++) {
    if (bytes_in[i] != static_cast<uint8_t>(a + b))
      return static_cast<dword_t>(i);
    a = b;
    b = bytes_in[i];
  }
  size_t out_size = 85;
  if (data_out.size != out_size)
    return 0x1073;
  uint8_t *bytes_out = static_cast<uint8_t*>(data_out.start);
  for (size_t i = 0; i < out_size; i++)
    bytes_out[i] = static_cast<byte_t>(bytes_in[i] + 13);
  return 0;
}

CONNECTOR_IMPL(TestInjecteeConnectBlocking, data_in, data_out) {
  utf8_t name = new_string(static_cast<char*>(data_in.start), data_in.size);
  def_ref_t<ClientChannel> channel = ClientChannel::create();
  if (!channel->open(name))
    return 0x2001;
  char buf[16];
  ReadIop read(channel->in(), buf, 16);
  if (!read.execute())
    return 0x2002;
  if (strcmp(buf, "Brilg & Spowegg") != 0)
    return 0x2003;
  return 0;
}

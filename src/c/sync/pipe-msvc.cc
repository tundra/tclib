//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

class WindowsPipeUtils {
public:
  // Generates a new pretty-sure-it-will-be-unique name for an "anonymous" pipe.
  static utf8_t gen_pipe_name(char *scratch, size_t scratch_size);

  // Creates a new client/server pipe pair and stores the respective ends in
  // the given out parameters. If inherit is true the handles will be inherited
  // by child processes. If pipe_name_out is non-null the pipe name will be
  // stored there.
  static bool create_overlapped_pipe(handle_t *server_pipe, handle_t *client_pipe,
      bool inherit, utf8_t *pipe_name_out);

  // Creates a named pipe with the given name and stores the server end in the
  // give out handle..
  static bool create_named_pipe(utf8_t name, bool inherit, handle_t *handle_out);

  // Connects to an existing named pipe and stores the client end in the given
  // out handle.
  static bool connect_named_pipe(utf8_t name, bool inherit, handle_t *handle_out);

  // Initializes the given attribs.
  static void config_security_attribs(SECURITY_ATTRIBUTES *attribs,
      bool inherit);
};

utf8_t WindowsPipeUtils::gen_pipe_name(char *scratch, size_t scratch_size) {
  CHECK_REL("not enough scratch", scratch_size, >=, MAX_PATH);
  static size_t next_pipe_serial = 0;
  // Generate a unique name for this pipe. There is a race condition here in
  // that access to next_pipe_serial is unsynchronized but the serial only has
  // to be unique within one thread so if we include the thread id we should
  // be fine. I think we're fine. Pretty sure we're fine.
  int name_len = sprintf(scratch,
      "\\\\.\\pipe\\TclibPipe-%016x-%016x-%016x", GetCurrentProcessId(),
      GetCurrentThreadId(), next_pipe_serial++);
  return new_string(scratch, name_len);
}

bool WindowsPipeUtils::create_overlapped_pipe(handle_t *server_pipe,
    handle_t *client_pipe, bool inherit, utf8_t *pipe_name_out) {
  char pipe_name_buf[MAX_PATH];
  utf8_t pipe_name = gen_pipe_name(pipe_name_buf, MAX_PATH);

  if (pipe_name_out != NULL)
    *pipe_name_out = string_default_dup(pipe_name);

  handle_t server = INVALID_HANDLE_VALUE;
  if (!create_named_pipe(pipe_name, inherit, &server))
    return false;

  handle_t client = INVALID_HANDLE_VALUE;
  if (!connect_named_pipe(pipe_name, inherit, &client))
    return false;

  *server_pipe = server;
  *client_pipe = client;
  return true;
}

bool WindowsPipeUtils::create_named_pipe(utf8_t name, bool inherit,
    handle_t *handle_out) {
  SECURITY_ATTRIBUTES attribs;
  config_security_attribs(&attribs, inherit);

  dword_t size = 4096;
  handle_t handle = CreateNamedPipe(
      name.chars,                                 // lpName
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // dwOpenMode
      PIPE_TYPE_BYTE | PIPE_WAIT,                 // dwPipeMode
      1,                                          // nMaxInstances
      size,                                       // nOutBufferSize
      size,                                       // nInBufferSize
      120 * 1000,                                 // nDefaultTimeOut
      &attribs);                                  // lpSecurityAttributes
  if (handle == INVALID_HANDLE_VALUE) {
    LOG_ERROR("CreateNamedPipe(%s, ...): %i", name.chars, GetLastError());
    return false;
  }
  *handle_out = handle;
  return true;
}

bool WindowsPipeUtils::connect_named_pipe(utf8_t name, bool inherit,
    handle_t *handle_out) {
  SECURITY_ATTRIBUTES attribs;
  config_security_attribs(&attribs, inherit);

  handle_t handle = CreateFile(
      name.chars,                                   // lpFileName
      GENERIC_READ | GENERIC_WRITE,                 // dwDesiredAccess
      0,                                            // dwShareMode
      &attribs,                                     // lpSecurityAttributes
      OPEN_EXISTING,                                // dwCreationDisposition
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // dwFlagsAndAttributes
      NULL);                                        // hTemplateFile

  if (handle == INVALID_HANDLE_VALUE) {
    LOG_ERROR("CreateFile(%s, ...): %i", name.chars, GetLastError());
    return false;
  }
  *handle_out = handle;
  return true;
}

void WindowsPipeUtils::config_security_attribs(SECURITY_ATTRIBUTES *attribs,
    bool inherit) {
  ZeroMemory(attribs, sizeof(SECURITY_ATTRIBUTES));
  attribs->nLength = sizeof(SECURITY_ATTRIBUTES);
  attribs->bInheritHandle = inherit;
  attribs->lpSecurityDescriptor = NULL;
}

bool NativePipe::open(uint32_t flags) {
  bool result = WindowsPipeUtils::create_overlapped_pipe(
      &this->pipe_.read_,
      &this->pipe_.write_,
      (flags & pfInherit) != 0,
      NULL);

  if (result) {
    in_ = InOutStream::from_raw_handle(this->pipe_.read_);
    out_ = InOutStream::from_raw_handle(this->pipe_.write_);
  }
  return result;
}

class WindowsServerPipe : public ServerPipe {
public:
  WindowsServerPipe();
  virtual ~WindowsServerPipe();

  virtual void self_destruct() { default_delete_concrete(this); }

  virtual bool open(uint32_t flags);

  virtual utf8_t name() { return string_empty(); }

  virtual InStream *in() { return incoming_.in(); }

  virtual OutStream *out() { return outgoing_.out(); }

  static ServerPipe *create();

private:
  NativePipe incoming_;
  NativePipe outgoing_;
};

WindowsServerPipe::WindowsServerPipe() { }

WindowsServerPipe::~WindowsServerPipe() {

}

bool WindowsServerPipe::open(uint32_t flags) {
  return incoming_.open(flags) && outgoing_.open(flags);
}

ServerPipe *ServerPipe::create() {
  return new (kDefaultAlloc) WindowsServerPipe();
}

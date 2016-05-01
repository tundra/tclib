//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "process.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

#include "c/winhdr.h"

using namespace tclib;

NativeProcessHandle::NativeProcessHandle()
  : id_(INVALID_HANDLE_VALUE) { }

class NativeProcess::PlatformData {
public:
  PlatformData();
  ~PlatformData();
  handle_t child_process() { return info()->hProcess; }
  handle_t child_main_thread() { return info()->hThread; }
  PROCESS_INFORMATION *info() { return &info_; }
  handle_t &wait_handle() { return wait_handle_; }

private:
  PROCESS_INFORMATION info_;
  handle_t wait_handle_;
};

NativeProcess::PlatformData::PlatformData()
  : wait_handle_(INVALID_HANDLE_VALUE) {
  struct_zero_fill(info_);
  info()->hProcess = INVALID_HANDLE_VALUE;
  info()->hThread = INVALID_HANDLE_VALUE;
}

NativeProcess::PlatformData::~PlatformData() {
  if (info()->hProcess != INVALID_HANDLE_VALUE)
    CloseHandle(info()->hProcess);
  if (info()->hThread != INVALID_HANDLE_VALUE)
    CloseHandle(info()->hThread);
  struct_zero_fill(info_);
  if (wait_handle() != INVALID_HANDLE_VALUE) {
    if (!UnregisterWaitEx(wait_handle(), NULL)) {
      // There is a race condition here because the wait callback it invoked
      // from a different thread and we have no guarantee of when that might
      // finish. So it's a possibility that we'll dispose the platform data
      // while control is still in the callback. In that case UnregisterWaitEx
      // will "fail" with ERROR_IO_PENDING but it's my understanding that the
      // wait will still be unregistered after the callback has returned.
      dword_t err = GetLastError();
      if (err != ERROR_IO_PENDING)
        WARN("Call to UnregisterWaitEx failed: %i", err);
    }
    wait_handle_ = INVALID_HANDLE_VALUE;
  }
}

void NativeProcess::mark_terminated(bool timer_or_wait_fired) {
  dword_t exit_code = 0;
  if (!GetExitCodeProcess(
      platform_data()->child_process(), // hProcess
      &exit_code)) { // lpExitCode
    WARN("Call to GetExitCodeProcess failed: %i", GetLastError());
  }
  if (exit_code == STILL_ACTIVE)
    WARN("Marking still active process as terminated.");
  exit_code_.fulfill(exit_code);
}

// A wrapper around a reference to memory in a different process.
template <typename T>
class RemoteMemory {
public:
  // Creates a new empty reference.
  RemoteMemory();

  // If this reference has been allocated, frees the memory.
  ~RemoteMemory();

  // Allocates a block of memory and fills it with the given data.
  fat_bool_t copy_into(handle_t process, const void* start, size_t size,
      bool is_executable);

  // Allocates an empty block of memory of the given size.
  fat_bool_t alloc(handle_t process, size_t size, bool is_executable = false);

  // Returns a pointer to the remote memory.
  T* operator*() { return static_cast<T*>(remote_data_.start); }
  T* operator->() { return this->operator*(); }

  // Returns the remote memory as a blob.
  blob_t remote() { return remote_data_; }

private:
  handle_t process_;
  blob_t remote_data_;
};

class NativeProcessHandle::InjectState {
public:
  InjectState(InjectRequest *request, handle_t child_process)
    : request_(request)
    , child_process_(child_process)
    , loader_thread_(INVALID_HANDLE_VALUE) { }

  fat_bool_t start_inject_dll();

  fat_bool_t complete_inject_dll(Duration timeout);

private:
  // Types of the functions being passed in.
  typedef module_t (WINAPI *load_library_a_t)(const char *filename);
  typedef FARPROC (WINAPI *get_proc_address_t)(module_t module, const char *proc_name);
  typedef dword_t (WINAPI *get_last_error_t)();

  // Type of the function defined in the DLL that we'll call to initialize
  // the DLL.
  typedef dword_t (*dll_connect_t)(blob_t data_in, blob_t data_out);

  // The block of data that is passed to the injectee.
  typedef struct {
    // The name of the DLL to load.
    const char *dll_name;
    const char *connect_name;
    blob_t data_in;
    blob_t data_out_scratch;
    // The address of these functions are actually available within the process
    // but we need the call to be absolute since if it's relative then it breaks
    // when we copy the function to what will almost certainly be a different
    // place. Passing the address explicitly ensures that the call is absolute.
    load_library_a_t load_library_a;
    get_proc_address_t get_proc_address;
    get_last_error_t get_last_error;
  } inject_in_t;

  typedef struct {
    // A character that identifies which step of injection failed if it failed,
    // otherwise 0.
    char error_step;
    // If error_step is not 0 this holds an error code that indicates what the
    // problem is.
    dword_t error_code;
  } inject_out_t;

  typedef struct {
    inject_in_t in;
    inject_out_t out;
  } inject_data_t;

  InjectRequest *request_;
  InjectRequest *request() { return request_; }

  handle_t child_process_;
  handle_t loader_thread_;

  RemoteMemory<char> remote_path_;
  RemoteMemory<char> remote_connector_name_;
  RemoteMemory<uint8_t> remote_data_in_;
  RemoteMemory<uint8_t> remote_data_out_;
  RemoteMemory<inject_data_t> remote_data_;
  RemoteMemory<byte_t> remote_entry_point_;

  static const size_t kMaxEntryPointSize = 2048;

  // Entry point that is copied into the process and called.
  static dword_t __stdcall inject_entry_point(void *raw_data);

  template <typename T>
  fat_bool_t read_data_from_child_process(const void *addr, T *dest, size_t size);
};

dword_t __stdcall NativeProcessHandle::InjectState::inject_entry_point(void *raw_data) {
  // This code is *very* special indeed because it will be copied into the
  // child process. It must be smaller than kMaxEntryPointSize bytes long,
  // can't use literal strings (because they won't be copied along with it) and
  // can't call library functions (because the calls may be relative and will
  // break when the code moves). I'm sure other restrictions apply too so just
  // be super careful.
  inject_data_t *data = static_cast<inject_data_t*>(raw_data);
  const inject_in_t *in = &data->in;
  inject_out_t *out = &data->out;
  // Set the error code *before* doing an operation that might fail because
  // that way, if we crash the step will already be set and should be visible
  // to the parent process.
  out->error_step = 'L';
  module_t module = (in->load_library_a)(in->dll_name);
  if (module == NULL) {
    out->error_code = (in->get_last_error)();
    return 1;
  }
  if (in->connect_name != NULL) {
    out->error_step = 'P';
    void *raw_connect = (in->get_proc_address)(module, in->connect_name);
    if (raw_connect == NULL) {
      out->error_code = (in->get_last_error)();
      return 1;
    }
    out->error_step = 'C';
    dll_connect_t connect = reinterpret_cast<dll_connect_t>(raw_connect);
    dword_t result = connect(in->data_in, in->data_out_scratch);
    if (result != 0) {
      out->error_code = result;
      return 1;
    }
  }
  out->error_step = 0;
  return 0;
}

template <typename T>
fat_bool_t NativeProcessHandle::InjectState::read_data_from_child_process(const void *addr,
    T *dest, size_t size) {
  win_size_t bytes_read = 0;
  if (!ReadProcessMemory(child_process_, addr, dest, size, &bytes_read)) {
    LOG_ERROR("ReadProcessMemory(-, %p, -, %i, -): %i", addr, size, GetLastError());
    return F_FALSE;
  }
  return F_TRUE;
}

fat_bool_t NativeProcess::start_inject_library(NativeProcessHandle::InjectRequest *request) {
  CHECK_TRUE("injecting non-suspended", (flags() & pfStartSuspendedOnWindows) != 0);
  CHECK_TRUE("already injecting", request->state() == NULL);
  return handle()->start_inject_library(request);
}

fat_bool_t NativeProcessHandle::start_inject_library(InjectRequest *request) {
  InjectState *state = new (kDefaultAlloc) InjectState(request, id());
  request->set_state(state);
  return state->start_inject_dll();
}

fat_bool_t NativeProcessHandle::complete_inject_library(InjectRequest *request, Duration timeout) {
  CHECK_TRUE("not injecting", request->state() != NULL);
  InjectState *state = request->state();
  request->set_state(NULL);
  fat_bool_t injected = state->complete_inject_dll(timeout);
  default_delete_concrete(state);
  return injected;
}

template <typename T>
RemoteMemory<T>::RemoteMemory()
  : process_(INVALID_HANDLE_VALUE)
  , remote_data_(blob_empty()) { }

template <typename T>
RemoteMemory<T>::~RemoteMemory() {
  if (blob_is_empty(remote_data_))
    return;
  if (!VirtualFreeEx(process_, remote_data_.start, 0, MEM_RELEASE))
    WARN("VirtualFreeEx(_): %i", GetLastError());
  remote_data_ = blob_empty();
}

template <typename T>
fat_bool_t RemoteMemory<T>::copy_into(handle_t process, const void* start, size_t size,
    bool is_executable) {
  F_TRY(alloc(process, size, is_executable));
  win_size_t bytes_written = 0;
  if (!WriteProcessMemory(process, remote_data_.start, start, size, &bytes_written)) {
    LOG_ERROR("WriteProcessMemory(_, %p, %p, %i, _): %i", remote_data_.start,
        start, size, GetLastError());
    return F_FALSE;
  }
  if (bytes_written != size) {
    LOG_ERROR("Write of data into child process was incomplete: %i < %i.",
        bytes_written, size);
    return F_FALSE;
  }
  return F_TRUE;
}

template <typename T>
fat_bool_t RemoteMemory<T>::alloc(handle_t process, size_t size, bool is_executable) {
  process_ = process;
  void *start = VirtualAllocEx(process, NULL, size, MEM_RESERVE | MEM_COMMIT,
      is_executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
  if (start == NULL) {
    LOG_ERROR("VirtualAllocEx(_, _, %i, _, _): %i", size, GetLastError());
    return F_FALSE;
  }
  remote_data_ = blob_new(start, size);
  return F_TRUE;
}

fat_bool_t NativeProcessHandle::InjectState::start_inject_dll() {
  // Create a local version of the injected data which we'll later copy into
  // the process.
  inject_data_t proto_data;
  struct_zero_fill(proto_data);

  inject_in_t *in = &proto_data.in;
  in->load_library_a = LoadLibraryA;
  in->get_proc_address = GetProcAddress;
  in->get_last_error = GetLastError;

  // Copy the dll name and store it in the data.
  utf8_t path = request()->path();
  F_TRY(remote_path_.copy_into(child_process_, path.chars, path.size + 1, false));
  in->dll_name = *remote_path_;

  // If there's a connector copy that too.
  utf8_t connector_name = request()->connector_name();
  if (!string_is_empty(connector_name)) {
    F_TRY(remote_connector_name_.copy_into(child_process_, connector_name.chars,
        connector_name.size + 1, false));
    in->connect_name = *remote_connector_name_;
  }

  // If there's input data copy that into the process.
  blob_t data_in = request()->data_in();
  if (!blob_is_empty(data_in)) {
    F_TRY(remote_data_in_.copy_into(child_process_, data_in.start, data_in.size, false));
    in->data_in = blob_new(*remote_data_in_, data_in.size);
  }

  // If the caller expects output data also copy that into the process.
  blob_t data_out = request()->data_out();
  if (!blob_is_empty(data_out)) {
    F_TRY(remote_data_out_.alloc(child_process_, data_out.size));
    in->data_out_scratch = remote_data_out_.remote();
  }

  // Copy the injected data into the process.
  F_TRY(remote_data_.copy_into(child_process_, &proto_data, sizeof(proto_data),
      false));

  // Copy the binary code of the entry point function into the process.
  F_TRY(remote_entry_point_.copy_into(child_process_, NativeProcess::resolve_jump_thunk(inject_entry_point),
      kMaxEntryPointSize, true));
  LPTHREAD_START_ROUTINE start = reinterpret_cast<LPTHREAD_START_ROUTINE>(*remote_entry_point_);

  // Create a thread in the child process that runs the entry point.
  loader_thread_ = CreateRemoteThread(child_process_, NULL, 0, start,
      *remote_data_, NULL, NULL);
  if (loader_thread_ == NULL) {
    LOG_ERROR("Failed to create remote DLL loader thread: %i", GetLastError());
    return F_FALSE;
  }

  // Start the loader thread running.
  dword_t retval = ResumeThread(loader_thread_);
  if (retval == -1) {
    LOG_ERROR("Failed to start loader thread: %i", GetLastError());
    return F_FALSE;
  }

  return F_TRUE;
}

fat_bool_t NativeProcessHandle::InjectState::complete_inject_dll(Duration timeout) {
  // Wait the the loader thread to complete.
  if (WaitForSingleObject(loader_thread_, timeout.to_winapi_millis()) != WAIT_OBJECT_0)
    return F_FALSE;

  // If the injector messes up we may kill the process. Check whether we've done
  // that because it's easier to debug when you know this happened rather than
  // chase down why reading fails below.
  dword_t exit_code = 0;
  if (!GetExitCodeProcess(child_process_, &exit_code)) {
    LOG_ERROR("GetExitCodeProcess(_): %i", GetLastError());
    return F_FALSE;
  }
  if (exit_code != STILL_ACTIVE) {
    LOG_ERROR("Process died during DLL injection: %p", exit_code);
    return F_FALSE;
  }

  // Copy the output data back from the process.
  inject_out_t out;
  struct_zero_fill(out);
  F_TRY(read_data_from_child_process(&remote_data_->out, &out, sizeof(out)));

  if (out.error_step == 'C') {
    dword_t code = out.error_code;
    int32_t file_id = (code >> 20) & 0xFFF;
    int32_t line = (code >> 10) & 0x3FF;
    int32_t last_error = code & 0x3FF;
    LOG_ERROR("Connecting failed at file 0x?%03x line %i, last error: %i",
        file_id, line, last_error);
    return F_FALSE;
  } else if (out.error_step != 0) {
    LOG_ERROR("Injecting failed at step %c: 0x%8x", out.error_step, out.error_code);
    return F_FALSE;
  }

  blob_t data_out = request()->data_out();
  if (!blob_is_empty(data_out)) {
    // If there's a data_out parameter we copy the output data into it.
    F_TRY(read_data_from_child_process(*remote_data_out_, data_out.start,
        data_out.size));
  }

  return F_TRUE;
}

fat_bool_t NativeProcess::resume() {
  CHECK_TRUE("resuming non-suspended", (flags() & pfStartSuspendedOnWindows) != 0);
  dword_t retval = ResumeThread(platform_data()->child_main_thread());
  if (retval == -1) {
    LOG_ERROR("Failed to resume suspended process: %i", GetLastError());
    return F_FALSE;
  }
  return F_TRUE;
}

fat_bool_t NativeProcess::kill() {
  if (!TerminateProcess(platform_data()->child_process(), 1)) {
    LOG_ERROR("TerminateProcess(-, 1): %i", GetLastError());
    return F_FALSE;
  }
  return F_TRUE;
}

namespace tclib {
class NativeProcessStart {
public:
  NativeProcessStart(NativeProcess *process);
  ~NativeProcessStart();
  utf8_t build_cmdline(utf8_t executable, size_t argc, utf8_t *argv);

  // Set up any standard stream redirection in the startup_info.
  fat_bool_t configure_standard_streams();

  // If necessary set up redirection using the given stream, storing the result
  // in the three out parameters. This is a little messy but we need to do this
  // a couple of times so it seems worth it.
  fat_bool_t maybe_redirect_standard_stream(const char *name, stdio_stream_t stream,
      handle_t *handle_out, bool *has_redirected);

  NativeProcess *process() { return process_; }
  fat_bool_t configure_sub_environment();
  fat_bool_t launch(utf8_t executable);
  fat_bool_t post_launch();
private:
  NativeProcess *process_;
  string_buffer_t cmdline_buf_;
  utf8_t cmdline_;
  STARTUPINFO startup_info_;
  string_buffer_t new_env_buf_;
  utf8_t new_env_;
};
}

NativeProcessStart::NativeProcessStart(NativeProcess *process)
  : process_(process)
  , new_env_(string_empty()) {
  string_buffer_init(&cmdline_buf_);
  ZeroMemory(&startup_info_, sizeof(startup_info_));
  startup_info_.cb = sizeof(startup_info_);
  string_buffer_init(&new_env_buf_);
}

NativeProcessStart::~NativeProcessStart() {
  string_buffer_dispose(&cmdline_buf_);
  string_buffer_dispose(&new_env_buf_);
}

static void string_buffer_append_escaped(string_buffer_t *buf, utf8_t str) {
  // Escaping commands on windows is tricky. Quoting strings takes care of most
  // luckily but quotes themselves must be escaped with \s. You'd think you'd
  // then need to also escape \s themselves but if you try that fails -- two
  // slashes in a row is read as two actual slashes, not one escaped one, except
  // for immediately before an escaped quote and then at the end of the string
  // because we'll add an unescaped quote there.
  string_buffer_putc(buf, '"');
  size_t length = str.size;
  for (size_t ic = 0; ic < length; ic++) {
    // Count slashes from this point. Usually it will be 0.
    size_t slash_count = 0;
    while (str.chars[ic] == '\\' && ic < length) {
      ic++;
      slash_count++;
    }
    // At this point if we started at a sequence of slashes we will now be
    // immediately past it, possibly just past the end of the string.
    if (ic == length || str.chars[ic] == '"') {
      // If we're at the end or before a quote slashes need to be escaped.
      for (size_t is = 0; is < slash_count; is++) {
        string_buffer_putc(buf, '\\');
        string_buffer_putc(buf, '\\');
      }
      // If we're at the end there's nothing to do, the quote will be added
      // below, otherwise escape the quote.
      if (str.chars[ic] == '"') {
        string_buffer_putc(buf, '\\');
        string_buffer_putc(buf, '"');
      }
    } else {
      // The slashes were followed by a non-special character so we can just
      // output them, no need to escape.
      for (size_t is = 0; is < slash_count; is++)
        string_buffer_putc(buf, '\\');
      string_buffer_putc(buf, str.chars[ic]);
    }
  }
  string_buffer_putc(buf, '"');
}

utf8_t NativeProcessStart::build_cmdline(utf8_t executable, size_t argc,
    utf8_t *argv) {
  string_buffer_append_escaped(&cmdline_buf_, executable);
  for (size_t i = 0; i < argc; i++) {
    string_buffer_append(&cmdline_buf_, new_c_string(" "));
    string_buffer_append_escaped(&cmdline_buf_, argv[i]);
  }
  return cmdline_ = string_buffer_flush(&cmdline_buf_);
}

fat_bool_t NativeProcessStart::maybe_redirect_standard_stream(const char *name,
    stdio_stream_t stream, handle_t *handle_out, bool *has_redirected) {
  StreamRedirect redirect = process()->stdio_[stream];
  if (redirect.is_empty())
    return F_TRUE;
  if (!redirect.prepare_launch())
    return F_FALSE;
  handle_t handle = redirect.remote_handle();
  if (handle == AbstractStream::kNullNakedFileHandle) {
    WARN("Invalid %s", name);
    return F_FALSE;
  }
  *handle_out = handle;
  *has_redirected = true;
  return F_TRUE;
}

fat_bool_t NativeProcessStart::configure_standard_streams() {
  bool has_redirected = false;

  F_TRY(maybe_redirect_standard_stream("stdin", siStdin,
      &startup_info_.hStdInput, &has_redirected));

  F_TRY(maybe_redirect_standard_stream("stdout", siStdout,
      &startup_info_.hStdOutput, &has_redirected));

  F_TRY(maybe_redirect_standard_stream("stderr", siStderr,
      &startup_info_.hStdError, &has_redirected));

  if (has_redirected)
    startup_info_.dwFlags |= STARTF_USESTDHANDLES;

  return F_TRUE;
}

fat_bool_t NativeProcessStart::configure_sub_environment() {
  if (process()->env_.empty())
    // If we don't want the environment to change we just leave new_env_ empty
    // and launch will do the right thing.
    return F_TRUE;

  // Copy the new variables also.
  for (size_t i = process()->env_.size(); i > 0; i--) {
    std::string entry = process()->env_[i - 1];
    string_buffer_append(&new_env_buf_, new_string(entry.c_str(), entry.length()));
    string_buffer_putc(&new_env_buf_, '\0');
  }

  // Copy the existing environment into the new env block.
  for (char **entry = environ; *entry != NULL; entry++) {
    string_buffer_append(&new_env_buf_, new_c_string(*entry));
    string_buffer_putc(&new_env_buf_, '\0');
  }

  // Flushing adds the last of the two null terminators that ends the whole
  // thing.
  new_env_ = string_buffer_flush(&new_env_buf_);
  return F_TRUE;
}

static void CALLBACK mark_terminated_bridge(void *context, BOOLEAN timer_or_wait_fired) {
  static_cast<NativeProcess*>(context)->mark_terminated(timer_or_wait_fired);
}

fat_bool_t NativeProcessStart::launch(utf8_t executable) {
  char *cmdline_chars = const_cast<char*>(cmdline_.chars);
  void *env = NULL;
  if (!string_is_empty(new_env_))
    env = static_cast<void*>(const_cast<char*>(new_env_.chars));

  NativeProcess::PlatformData *data = process()->platform_data_;
  // Create the child process.
  dword_t creation_flags = 0;
  int32_t process_flags = process()->flags();
  if ((process_flags & pfStartSuspendedOnWindows) != 0)
    creation_flags |= CREATE_SUSPENDED;
  if ((process_flags & pfNewHiddenConsoleOnWindows) != 0)
    creation_flags |= CREATE_NO_WINDOW;
  if ((process_flags & pfNewVisibleConsoleOnWindows) != 0)
    creation_flags |= CREATE_NEW_CONSOLE;
  bool created = CreateProcess(
    executable.chars,// lpApplicationName
    cmdline_chars,   // lpCommandLine
    NULL,            // lpProcessAttributes
    NULL,            // lpThreadAttributes
    true,            // bInheritHandles
    creation_flags,  // dwCreationFlags
    env,             // lpEnvironment
    NULL,            // lpCurrentDirectory
    &startup_info_,  // lpStartupInfo
    data->info());   // lpProcessInformation

  if (!created) {
    process()->state = NativeProcess::nsCouldntCreate;
    process()->exit_code_.fulfill(GetLastError());

    // It might seem counter-intuitive to succeed when CreateProcess returns
    // false, but it is done to keep the interface consistent across platforms.
    // On posix we won't know if creating the child failed until after we wait
    // for it so even in that case start will succeed. So we simulate that on
    // windows by succeeding here and recording the error so it can be reported
    // later.
    return F_TRUE;
  }

  process()->handle()->set_id(data->child_process());
  process()->state = NativeProcess::nsRunning;
  if (!RegisterWaitForSingleObject(
      &data->wait_handle(),    // phNewWaitObject
      data->child_process(),   // hObject
      &mark_terminated_bridge, // Callback
      process(),               // Context
      INFINITE,                // dwMilliseconds
      WT_EXECUTEONLYONCE))     // dwFlags
    WARN("Call to RegisterWaitForSingleObject failed: %i", GetLastError());

  return F_TRUE;
}

fat_bool_t NativeProcessStart::post_launch() {
  // Close the parent's clone of the stdout handle since it belongs to the
  // child now.
  fat_bool_t all_succeeded = F_TRUE;
  for (size_t i = 0; i < kStdioStreamCount; i++) {
    StreamRedirect redirect = process()->stdio_[i];
    if (!redirect.is_empty()) {
      fat_bool_t closed = redirect.parent_side_close();
      if (all_succeeded && !closed)
        // Capture the first failure.
        all_succeeded = closed;
    }
  }
  return all_succeeded;
}

fat_bool_t PipeRedirector::prepare_launch(StreamRedirect *redirect) const {
  // Do inherit the remote side of this pipe.
  if (!SetHandleInformation(
          remote_side(redirect)->to_raw_handle(), // hObject
          HANDLE_FLAG_INHERIT,                    // dwMask
          1)) {                                   // dwFlags
    WARN("Failed to set remote pipe flags while redirecting");
    return F_FALSE;
  }

  // Don't inherit the local side of this pipe.
  if (!SetHandleInformation(
          local_side(redirect)->to_raw_handle(), // hObject
          HANDLE_FLAG_INHERIT,                   // dwMask
          0)) {                                  // dwFlags
    WARN("Failed to set local pipe flags while redirecting");
    return F_FALSE;
  }
  return F_TRUE;
}

fat_bool_t PipeRedirector::parent_side_close(StreamRedirect *redirect) const {
  return F_BOOL(remote_side(redirect)->close());
}

fat_bool_t PipeRedirector::child_side_close(StreamRedirect *redirect) const {
  // There is no child side, or -- there is but we don't have access to it.
  return F_FALSE;
}

fat_bool_t NativeProcess::start(utf8_t executable, size_t argc, utf8_t *argv) {
  CHECK_EQ("starting process already running", nsInitial, state);
  platform_data_ = new NativeProcess::PlatformData();
  NativeProcessStart start(this);
  start.build_cmdline(executable, argc, argv);
  F_TRY(start.configure_standard_streams());
  F_TRY(start.configure_sub_environment());
  F_TRY(start.launch(executable));
  F_TRY(start.post_launch());
  return F_TRUE;
}

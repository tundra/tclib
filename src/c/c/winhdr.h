//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// The standard windows header with some alternative names for the built-in
// windows types for consistency with the rest of the code.

#ifndef TCLIB_WINHDR_NOISY
// Disable everything we can from windows.h. Otherwise it pollutes the macro
// namespace something awful. If you're sure you want everything define
// TCLIB_WINHDR_NOISY. Don't do that in a header though.
#  define NOGDICAPMASKS 1
#  define NOVIRTUALKEYCODES 1
#  define NOWINMESSAGES 1
#  define NOWINSTYLES 1
#  define NOSYSMETRICS 1
#  define NOMENUS 1
#  define NOICONS 1
#  define NOKEYSTATES 1
#  define NOSYSCOMMANDS 1
#  define NORASTEROPS 1
#  define NOSHOWWINDOW 1
#  define OEMRESOURCE 1
#  define NOATOM 1
#  define NOCLIPBOARD 1
#  define NOCOLOR 1
#  define NOCTLMGR 1
#  define NODRAWTEXT 1
#  define NOGDI 1
#  define NOKERNEL 1
#  define NOUSER 1
#  define NONLS 1
#  define NOMB 1
#  define NOMEMMGR 1
#  define NOMETAFILE 1
#  define NOMINMAX 1
#  define NOMSG 1
#  define NOOPENFILE 1
#  define NOSCROLL 1
#  define NOSERVICE 1
#  define NOSOUND 1
#  define NOTEXTMETRIC 1
#  define NOWH 1
#  define NOWINOFFSETS 1
#  define NOCOMM 1
#  define NOKANJI 1
#  define NOHELP 1
#  define NOPROFILER 1
#  define NODEFERWINDOWPOS 1
#  define NOMCX 1
#  define VC_EXTRALEAN 1
#  define WIN32_LEAN_AND_MEAN 1
#endif

#define STRICT 1

#pragma warning(push, 0)
#  include <windows.h>
#pragma warning(pop)

#define dword_t DWORD
#define word_t WORD
#define short_t SHORT
#define ushort_t USHORT
#define win_size_t SIZE_T
#define bool_t BOOL
#define ntstatus_t LONG
#define ulong_t ULONG
#define uint_t UINT

#define ansi_char_t CHAR
#define ansi_str_t LPSTR
#define ansi_cstr_t LPCSTR

#define wide_char_t WCHAR
#define wide_str_t LPWSTR
#define wide_cstr_t LPCWSTR

#define str_t LPTSTR
#define cstr_t LPCTSTR
#define char_t TCHAR

#define ssize_t SSIZE_T

#define module_t HMODULE
#define hwnd_t HWND
#define hkey_t HKEY
#define handle_t HANDLE

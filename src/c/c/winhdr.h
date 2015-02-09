//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// The standard windows header with some alternative names for the built-in
// windows types for consistency with the rest of the code.

#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)

#define dword_t DWORD
#define word_t WORD
#define short_t SHORT
#define ushort_t USHORT
#define win_size_t SIZE_T
#define bool_t BOOL
#define ntstatus_t NTSTATUS
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

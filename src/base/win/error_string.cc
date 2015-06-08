// Copyright (c) 2015 The LibTrace Authors.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the <organization> nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "base/win/error_string.h"

#include <sstream>
#include <strsafe.h>

#include "base/string_utils.h"

namespace base {

std::string GetWindowsErrorString(DWORD error)
{
  LPWSTR error_desc_buffer = NULL;
  LPWSTR error_str_buffer = NULL;

  ::FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&error_desc_buffer,
      0, NULL);

  error_str_buffer = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
    (lstrlen((LPCTSTR)error_desc_buffer) + 40) * sizeof(TCHAR));
  ::StringCchPrintf((LPTSTR)error_str_buffer,
    LocalSize(error_str_buffer) / sizeof(TCHAR),
    TEXT("%d - %s"),
    error, error_desc_buffer);

  std::wstring error_str(error_str_buffer);

  ::LocalFree(error_desc_buffer);
  ::LocalFree(error_str_buffer);

  return WStringToString(error_str);
}

std::string GetLastWindowsErrorString()
{
  return GetWindowsErrorString(::GetLastError());
}

}  // namespace base

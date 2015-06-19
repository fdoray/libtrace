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

#include <vector>

#include "base/logging.h"
#include "base/win/error_string.h"
#include "base/win/scoped_handle.h"
#include "symbols/win/dbghelp_wrapper.h"

namespace symbols {
namespace win {

namespace {

// Address at which images are loaded.
const base::Address kDefaultBaseAddress = 1024;

// DbgHelp callback, used to provide the checksum and timestamp of an image
// that is being loaded. See documentation of PSYMBOL_REGISTERED_CALLBACK64.
BOOL CALLBACK LoadImageCallback(HANDLE /* process */, ULONG action,
                                ULONG64 data, ULONG64 context) {
  // Handle only the CBA_DEFERRED_SYMBOL_LOAD_PARTIAL action.
  if (action != CBA_DEFERRED_SYMBOL_LOAD_PARTIAL)
    return FALSE;

  Image* image = reinterpret_cast<Image*>(context);
  IMAGEHLP_DEFERRED_SYMBOL_LOAD64* loaded_image =
      reinterpret_cast<IMAGEHLP_DEFERRED_SYMBOL_LOAD64*>(data);

  DCHECK(data != NULL);
  DCHECK(context != NULL);

  loaded_image->CheckSum = image->checksum;
  loaded_image->TimeDateStamp = image->timestamp;
  loaded_image->Reparse = TRUE;
  return TRUE;
}

// Retrieve the user space path associated with a path as viewed by the kernel.
// @param path path as viewed by the kernel.
// @param resolved resolved user space path.
// @returns true if the path has been resolved, false otherwise.
bool ResolveKernelPath(const std::wstring& path, std::wstring* resolved) {
  DCHECK(resolved != NULL);

  const std::wstring kDevicePath(L"\\Device\\");
  const std::wstring kSystemRootPath(L"\\SystemRoot\\");

  if (path.compare(0, kDevicePath.size(), kDevicePath) == 0) {
    // |path| is a path to a device, as viewed by the kernel.

    // CreateFileW() can receive a path to a device as viewed by the kernel.
    // It must start with "\\.\", so add this prefix.
    std::wstring device_path = L"\\\\.\\" + path.substr(kDevicePath.size());
    base::win::ScopedHandle hFile(::CreateFileW(
        device_path.c_str(),    // file to open
        GENERIC_READ,           // open for reading
        FILE_SHARE_READ,        // share for reading
        NULL,                   // default security
        OPEN_EXISTING,          // existing file only
        FILE_ATTRIBUTE_NORMAL,  // normal file
        NULL));                 // no attr. template
    if (hFile.get() == INVALID_HANDLE_VALUE)
      return false;

    std::vector<WCHAR> buffer(MAX_PATH);
    // Use the file handle to retrieve the user space path to the file.
    DWORD dwRet = ::GetFinalPathNameByHandleW(hFile.get(), &buffer[0],
                                              buffer.size(), VOLUME_NAME_DOS);
    if (dwRet > buffer.size()) {
      // Try again with a bigger buffer for the path.
      buffer.resize(dwRet);
      dwRet = ::GetFinalPathNameByHandleW(hFile.get(), &buffer[0],
                                          buffer.size(), VOLUME_NAME_DOS);
    }
    if (dwRet == 0 || dwRet > buffer.size())
      return false;
    *resolved = std::wstring(&buffer[0]);
    return true;
  } else if (path.compare(0, kSystemRootPath.size(), kSystemRootPath) == 0) {
    // |path| is a system path as viewed by the kernel. Replace the
    // "\SystemRoot\" prefix with the user space path to the system directory.
    std::vector<WCHAR> buffer(MAX_PATH);
    DWORD dwRet = ::GetSystemWindowsDirectoryW(&buffer[0], buffer.size());
    if (dwRet > buffer.size()) {
      // Try again with a bigger buffer for the path.
      buffer.resize(dwRet);
      dwRet = ::GetSystemWindowsDirectoryW(&buffer[0], buffer.size());
    }
    if (dwRet == 0 || dwRet > buffer.size())
      return false;
    std::wstring user_space_system_folder = std::wstring(&buffer[0]);
    *resolved =
        user_space_system_folder + path.substr(kSystemRootPath.size() - 1);
    return true;
  } else {
    // Fallback: no conversion.
    *resolved = path;
    return true;
  }
}

BOOL CALLBACK EnumerateSymbolsCallbackWrapper(PSYMBOL_INFO symbol_info,
                                              ULONG symbol_size,
                                              PVOID user_context) {
  const DbghelpWrapper::SymbolCallback* callback =
      reinterpret_cast<const DbghelpWrapper::SymbolCallback*>(user_context);

  Symbol symbol;
  symbol.set_offset(symbol_info->Address - kDefaultBaseAddress);
  symbol.set_size(symbol_size);

  if (symbol_info->MaxNameLen != 0)
    symbol.set_name(symbol_info->Name);

  (*callback)(symbol);

  return TRUE;
}

}  // namespace

DbghelpWrapper::DbghelpWrapper() : initialized_(false) {
  // Use the address of this object as a session handle for DbgHelp.
  dbghelp_handle_ = reinterpret_cast<HANDLE>(this);
}

DbghelpWrapper::~DbghelpWrapper() {
  if (initialized_)
    ::SymCleanup(dbghelp_handle_);
}

bool DbghelpWrapper::EnumerateSymbols(
    const Image& image, const SymbolCallback& callback) {
  DCHECK(!image.filename.empty());

  if (!Initialize())
    return false;

  std::wstring resolved_path;
  if (!ResolveKernelPath(image.filename, &resolved_path)) {
    // If the path as viewed by the kernel cannot be resolved to a user space
    // path, pass it directly to DbgHelp. DbgHelp will try to find a matching
    // file name in the paths defined in the _NT_SYMBOL_PATH environment
    // variable. If there is a file name match, |image.checksum| is used to make
    // sure that the right image is used.
    resolved_path = image.filename;
  }

  // Register a callback to provide the image checksum and time stamp to
  // DbgHelp.
  ::SymRegisterCallback64(dbghelp_handle_,
                          LoadImageCallback,
                          reinterpret_cast<ULONG64>(&image));

  // Disable WOW64 system file redirection so that 64 bits images can be loaded
  // from a 32 bits build.
  PVOID disable_redirect_old_value;
  ::Wow64DisableWow64FsRedirection(&disable_redirect_old_value);

  // Open the image from which to extract the symbols.
  base::win::ScopedHandle image_file(::CreateFileW(
      resolved_path.c_str(),  // file to open
      GENERIC_READ,           // open for reading
      FILE_SHARE_READ,        // share for reading
      NULL,                   // default security
      OPEN_EXISTING,          // existing file only
      FILE_ATTRIBUTE_NORMAL,  // normal file
      NULL));                 // no attr. template

  // Restore WOW64 system file redirection.
  ::Wow64DisableWow64FsRedirection(&disable_redirect_old_value);

  if (image_file.get() == INVALID_HANDLE_VALUE)
    return false;

  // Load the image and its symbols. This can take a long time if data must
  // be downloaded from a symbol server.
  DWORD64 dwRet = ::SymLoadModuleEx(dbghelp_handle_, image_file.get(), NULL, NULL,
                                    kDefaultBaseAddress, image.size, NULL, 0);

  // SymLoadModuleEx() returns 0 in case of error, when a module is loaded at
  // address 0 or when the module is already loaded.
  if (dwRet == 0) {
    DWORD error = ::GetLastError();
    if (error != ERROR_SUCCESS) {
      LOG(ERROR) << "Error while loading symbols. "
                 << base::GetWindowsErrorString(error);
      return false;
    }
  }

  // Enumerate the symbols for the image. |callback| is called synchronously for
  // each symbol of the image.
  BOOL res = ::SymEnumSymbols(
      dbghelp_handle_, kDefaultBaseAddress, NULL,
      &EnumerateSymbolsCallbackWrapper,
      const_cast<PVOID>(reinterpret_cast<const void*>(&callback)));
  if (res == FALSE) {
    DWORD error = ::GetLastError();
    if (error != ERROR_SUCCESS) {
      LOG(ERROR) << "Error while loading symbols. "
                 << base::GetWindowsErrorString(error);
      return false;
    }
  }

  // Unload the image information from DbgHelp.
  ::SymUnloadModule64(dbghelp_handle_, 0);

  return res == TRUE ? true : false;
}

bool DbghelpWrapper::Initialize() {
  if (initialized_)
    return true;

  DWORD options = ::SymGetOptions();
  options |= SYMOPT_EXACT_SYMBOLS | SYMOPT_DEBUG;
  ::SymSetOptions(options);

  if (!::SymInitialize(dbghelp_handle_, NULL, FALSE))
    return false;

  initialized_ = true;
  return true;
}

}  // namespace win
}  // namespace symbols

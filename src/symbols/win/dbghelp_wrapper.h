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

#ifndef SYMBOLS_WIN_DBGHELP_WRAPPER_H_
#define SYMBOLS_WIN_DBGHELP_WRAPPER_H_

// Restrict the import to the windows basic includes.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // NOLINT
// Add Unicode support to DbgHelp functions.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>

#include <functional>

#include "base/base.h"
#include "base/types.h"
#include "symbols/image.h"
#include "symbols/symbol.h"

namespace symbols {
namespace win {

class DbghelpWrapper {
 public:
  DbghelpWrapper();
  ~DbghelpWrapper();

  typedef std::function<void(const Symbol&)> SymbolCallback;

  // Enumerates the symbols of an image.
  // @param image image for which to enumerate the symbols.
  // @returns true if the symbols have been enumerated correctly, false
  //     otherwise. Returns false when no debug information is found for the
  //     image.
  bool EnumerateSymbols(const Image& image, const SymbolCallback& callback);

 private:
  // Initializes the Dbghelp API.
  // @returns true if the initialization is successful, false otherwise.
  bool Initialize();

  // Indicates whether the service has been initialized successfully.
  bool initialized_;

  // Handle that identifies the DbgHelp session.
  HANDLE dbghelp_handle_;

  DISALLOW_COPY_AND_ASSIGN(DbghelpWrapper);
};

}  // namespace win
}  // namespace symbols

#endif  // SYMBOLS_SYMBOLS_RESOLVER_H_

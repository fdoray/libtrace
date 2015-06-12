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

#ifndef SYMBOLS_SYMBOL_H_
#define SYMBOLS_SYMBOL_H_

#include <string>

#include "base/types.h"

namespace symbols {

class Symbol {
 public:
  Symbol() {}

  // Name of the symbol.
  const std::wstring& name() const { return name_; }
  void set_name(const std::wstring& name) { name_ = name; }

  // Offset of the symbol in the image.
  const base::Offset& offset() const { return offset_; }
  void set_offset(const base::Offset& offset) { offset_ = offset; }

  // Size of the symbol.
  size_t size() const { return size_; }
  void set_size(size_t size) { size_ = size; }

 private:
  // Name of the symbol.
  std::wstring name_;

  // Offset of the symbol in the image.
  base::Offset offset_;

  // Size of the symbol.
  size_t size_;
};

}  // namespace symbols

#endif  // SYMBOLS_SYMBOL_H_

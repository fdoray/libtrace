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

#include "symbols/symbols_resolver.h"

#include <algorithm>

#include "base/inserter.h"

namespace symbols {

namespace {

bool SymbolOffsetComparison(
  const base::Offset& offset, const Symbol& symbol) {
  return offset < symbol.offset(); 
}

}  // namespace

SymbolsResolver::SymbolsResolver() {
}
SymbolsResolver::~SymbolsResolver() {
}

void SymbolsResolver::LoadImage(
    base::Pid pid, base::Address base_address, const symbols::Image& image) {
  pid_to_images_[pid][base_address] = image;
}

void SymbolsResolver::UnloadImage(base::Pid pid, base::Address base_address) {
  auto process_it = pid_to_images_.find(pid);
  if (process_it == pid_to_images_.end())
    return;
  process_it->second.erase(base_address);
}

bool SymbolsResolver::ResolveSymbol(
    base::Pid pid, base::Address address, Symbol* symbol) {
#if defined(USE_DBGHELP)
  // Find the image to which the symbol belongs.
  base::Address image_base_address = 0;
  const Image* image = FindImage(pid, address, &image_base_address);
  if (image == nullptr)
    return false;

  // Get the symbols for this image.
  const ImageSymbols& image_symbols = GetImageSymbols(*image);

  // Resolve the symbol.
  base::Offset offset = address - image_base_address;
  auto it = std::upper_bound(
      image_symbols.begin(), image_symbols.end(), offset,
      &SymbolOffsetComparison);
  if (it == image_symbols.begin())
    return false;

  --it;

  if (it == image_symbols.end())
    return false;

  if (offset > it->offset() + it->size())
    return false;

  *symbol = *it;

  return true;
#else
  return false;
#endif
}

const Image* SymbolsResolver::FindImage(
    base::Pid pid, base::Address address,
    base::Address* image_base_address) const {
  auto process_it = pid_to_images_.find(pid);
  if (process_it == pid_to_images_.end())
    return nullptr;

  const auto& process_images = process_it->second;

  if (process_images.empty())
    return nullptr;

  auto image_it = process_images.upper_bound(address);
  if (image_it == process_images.begin())
    return nullptr;

  --image_it;

  if (image_it == process_images.end())
    return nullptr;

  if (address >= image_it->first + image_it->second.size)
    return nullptr;

  *image_base_address = image_it->first;
  return &image_it->second;
}

#if defined(USE_DBGHELP)
const SymbolsResolver::ImageSymbols& SymbolsResolver::GetImageSymbols(
    const Image& image) {
  auto look = symbol_cache_.find(image);
  if (look != symbol_cache_.end())
    return look->second;

  ImageSymbols& image_symbols = symbol_cache_[image];
  dbghelp_wrapper_.EnumerateSymbols(
      image, base::BackInserter<Symbol>(&image_symbols));

  std::sort(image_symbols.begin(), image_symbols.end());

  return image_symbols;
}
#endif

}  // namespace symbols

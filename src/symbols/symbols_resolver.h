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

#ifndef SYMBOLS_SYMBOLS_RESOLVER_H_
#define SYMBOLS_SYMBOLS_RESOLVER_H_

#include <map>
#include <unordered_map>

#include "base/base.h"
#include "base/types.h"
#include "gtest/gtest_prod.h"
#include "symbols/image.h"

namespace symbols {

class SymbolsResolver {
 public:
  SymbolsResolver();
  ~SymbolsResolver();

  // Indicates that an image has been loaded into a process.
  // @param pid pid of the process in which the image has been loaded.
  // @param base_address address at which the image has been loaded.
  // @param image image that has been loaded.
  void LoadImage(base::Pid pid,
                 base::Address base_address,
                 const Image& image);

  // Indicates that an image has been unloaded from a process.
  // @param pid pid of the process from which the image has been unloaded.
  // @param base_address address from which the image has been unloaded.
  void UnloadImage(base::Pid pid,
                   base::Address base_address);

 private:
  // Finds to which image of a process an address belongs.
  // @param pid the pid of the process to which the address belongs.
  // @param address the address for which to find the image.
  // @param image_base_address [out] the base address of the image, if found.
  // @returns the image to which the address belongs, or nullptr if not found.
  const Image* FindImage(base::Pid pid,
                         base::Address address,
                         base::Address* image_base_address) const;

  // Images loaded in each process.
  typedef std::map<base::Address, symbols::Image> Images;
  typedef std::unordered_map<base::Pid, Images> PidToImages;
  PidToImages pid_to_images_;

  FRIEND_TEST(SymbolsResolver, FindImage);

  DISALLOW_COPY_AND_ASSIGN(SymbolsResolver);
};

}  // namespace symbols

#endif  // SYMBOLS_SYMBOLS_RESOLVER_H_

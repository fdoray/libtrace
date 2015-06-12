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

#include "gtest/gtest.h"

namespace symbols {

TEST(SymbolsResolver, FindImage) {
  const base::Pid kPid = 42;
  const base::Pid kOtherPid = 13;
  
  const Image* image = nullptr;

  Image image_a;
  image_a.size = 1000;
  image_a.checksum = 12;
  image_a.timestamp = 34;
  image_a.filename = L"image_a.dll";

  Image image_b;
  image_b.size = 2000;
  image_b.checksum = 56;
  image_b.timestamp = 78;
  image_b.filename = L"image_b.dll";

  Image image_c;
  image_c.size = 3000;
  image_c.checksum = 91;
  image_c.timestamp = 23;
  image_c.filename = L"image_c.dll";

  SymbolsResolver resolver;
  base::Address image_base_address = 0;

  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 70, &image_base_address));
  EXPECT_EQ(0, image_base_address);

  resolver.LoadImage(kPid, 10000, image_a);
  resolver.LoadImage(kPid, 20000, image_b);
  resolver.LoadImage(kOtherPid, 0, image_c);

  image_base_address = 0;
  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 5000, &image_base_address));
  EXPECT_EQ(0, image_base_address);
  
  image_base_address = 0;
  image = resolver.FindImage(kPid, 10000, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_a, *image);
  EXPECT_EQ(10000, image_base_address);

  image_base_address = 0;
  image = resolver.FindImage(kPid, 10500, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_a, *image);
  EXPECT_EQ(10000, image_base_address);

  image_base_address = 0;
  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 11000, &image_base_address));
  EXPECT_EQ(0, image_base_address);

  image_base_address = 0;
  image = resolver.FindImage(kPid, 20000, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_b, *image);
  EXPECT_EQ(20000, image_base_address);

  image_base_address = 0;
  image = resolver.FindImage(kPid, 21000, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_b, *image);
  EXPECT_EQ(20000, image_base_address);

  image_base_address = 0;
  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 30000, &image_base_address));
  EXPECT_EQ(0, image_base_address);

  image_base_address = 0;
  image = resolver.FindImage(kOtherPid, 0, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_c, *image);
  EXPECT_EQ(0, image_base_address);

  resolver.UnloadImage(kPid, 20000);

  image_base_address = 0;
  image = resolver.FindImage(kPid, 10000, &image_base_address);
  ASSERT_NE(nullptr, image);
  EXPECT_EQ(image_a, *image);
  EXPECT_EQ(10000, image_base_address);

  image_base_address = 0;
  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 20000, &image_base_address));
  EXPECT_EQ(0, image_base_address);

  resolver.UnloadImage(kPid, 10000);

  image_base_address = 0;
  EXPECT_EQ(nullptr, resolver.FindImage(kPid, 10000, &image_base_address));
  EXPECT_EQ(0, image_base_address);
}

}  // namespace symbols

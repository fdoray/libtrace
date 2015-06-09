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

#include <functional>

#include "base/multi_callback.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace base {

namespace {

class MockObject {
 public:
  MOCK_METHOD1(Method1, void(int));
  MOCK_METHOD1(Method2, void(int));
  MOCK_METHOD2(Method3, void(int, int));
  MOCK_METHOD2(Method4, void(int, int));
};

}  // namespace

TEST(MultiCallback, OneParameter) {
  MockObject mock_object;

  EXPECT_CALL(mock_object, Method1(8));
  EXPECT_CALL(mock_object, Method2(8));

  auto multi_callback = base::MultiCallback(
      base::CallbackVector<std::function<void(int)>>({
          std::bind(&MockObject::Method1, &mock_object, std::placeholders::_1),
          std::bind(&MockObject::Method2, &mock_object, std::placeholders::_1)
      }));

  multi_callback(8);
}

TEST(MultiCallback, TwoParameters) {
  MockObject mock_object;

  EXPECT_CALL(mock_object, Method3(8, 42));
  EXPECT_CALL(mock_object, Method4(8, 42));

  auto multi_callback = base::MultiCallback(
      base::CallbackVector<std::function<void(int, int)>>({
          std::bind(&MockObject::Method3, &mock_object, std::placeholders::_1,
              std::placeholders::_2),
          std::bind(&MockObject::Method4, &mock_object, std::placeholders::_1,
              std::placeholders::_2)
      }));

  multi_callback(8, 42);
}

}  // namespace base

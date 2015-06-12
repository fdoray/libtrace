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

#ifndef BASE_MULTI_CALLBACK_H_
#define BASE_MULTI_CALLBACK_H_

#include <functional>
#include <initializer_list>
#include <vector>

namespace base {

/*
template<typename R, typename... Args>
std::function<R(Args...)> MultiCallback(std::function<R(Args...)> callback) {
  return [=](Args... args) {
    callback(args...);
  };
}
*/

template<typename T>
std::vector<T> CallbackVector(std::initializer_list<T> callbacks) {
  return std::vector<T>(callbacks.begin(), callbacks.end());
}

template<typename R, typename... Args>
std::function<R(Args...)> MultiCallback(std::vector<std::function<R(Args...)>> callbacks) {
  return [=](Args... args) {
    for (const auto& callback : callbacks)
      callback(args...);
  };
}

}  // namespace base

#endif  // BASE_MULTI_CALLBACK_H_
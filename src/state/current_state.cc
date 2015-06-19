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

#include "state/current_state.h"

#include <vector>

#include "event/value.h"

namespace state {

namespace {

// Image events.
const char kImageCategory[] = "Image";
const char kImageLoadOperation[] = "Load";
const char kImageDCStartOperation[] = "DCStart";
const char kImageUnloadOperation[] = "Unload";
const char kImageKernelBase[] = "KernelBase";

// Stackwalk events.
const char kStackWalkCategory[] = "StackWalk";
const char kStackOperation[] = "Stack";

}  // namespace

CurrentState::CurrentState() {
}

CurrentState::~CurrentState() {
}

void CurrentState::OnEvent(const event::Event& event) {
  std::string category;
  std::string operation;

  if (!event.header()->GetFieldAsString(event::kCategoryFieldName, &category) ||
      !event.header()->GetFieldAsString(
          event::kOperationFieldName, &operation)) {
    return;
  }

  if (category == kImageCategory) {
    if (operation == kImageLoadOperation ||
        operation == kImageDCStartOperation) {
      OnImageLoad(event);
    } else if (operation == kImageUnloadOperation) {
      OnImageUnload(event);
    }
    else if (operation == kImageKernelBase) {
      // TODO.
    }
  } else if (category == kStackWalkCategory && operation == kStackOperation) {
    OnStackWalk(event);
  }
}

void CurrentState::OnImageLoad(const event::Event& event) {
  symbols::Image image;
  base::Address base_address = 0;
  base::Pid pid = 0;

  if (!event.payload()->GetFieldAsUInteger("ModuleSize", &image.size) ||
      !event.payload()->GetFieldAsUInteger("ImageCheckSum", &image.checksum) ||
      !event.payload()->GetFieldAsUInteger("TimeDateStamp", &image.timestamp) ||
      !event.payload()->GetFieldAsWString("ImageFileName", &image.filename) ||
      !event.payload()->GetFieldAsULong("BaseAddress", &base_address) ||
      !event.header()->GetFieldAsULong(event::kProcessIdFieldName, &pid)) {
    LOG(WARNING) << "Incomplete Image Load event.";
    return;
  }

  symbols_.LoadImage(pid, base_address, image);
}

void CurrentState::OnImageUnload(const event::Event& event) {
  base::Address base_address = 0;
  base::Pid pid = 0;

  if (!event.payload()->GetFieldAsULong("BaseAddress", &base_address) ||
      !event.header()->GetFieldAsULong(event::kProcessIdFieldName, &pid)) {
    LOG(WARNING) << "Incomplete Image Unload event.";
    return;
  }

  symbols_.UnloadImage(pid, base_address);
}

void CurrentState::OnStackWalk(const event::Event& event) {
  base::Timestamp event_ts = 0;
  base::Pid pid = 0;
  base::Tid tid = 0;
  const event::ArrayValue* stack = nullptr;

  if (!event.payload()->GetFieldAsULong("EventTimeStamp", &event_ts) ||
      !event.payload()->GetFieldAsULong("StackProcess", &pid) ||
      !event.payload()->GetFieldAsULong("StackThread", &tid) ||
      !event.payload()->GetFieldAs<event::ArrayValue>("Stack", &stack)) {
    LOG(WARNING) << "Incomplete StackWalk event.";
    return;
  }

  // Symbolize the stack.
  std::vector<std::wstring> symbolized_stack;
  for (const event::Value* address_value : *stack) {
    base::Address address = 0;
    if (!address_value->GetAsULong(&address)) {
      LOG(WARNING) << "Invalid stack format in StackWalk event.";
      return;
    }

    symbols::Symbol symbol;
    if (symbols_.ResolveSymbol(pid, address, &symbol))
      symbolized_stack.push_back(symbol.name());
  }
}

}  // namespace state

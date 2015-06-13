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

#include "event/value.h"

namespace state {

namespace {

// Image events.
const char kImageCategory[] = "Image";
const char kImageLoadOperation[] = "Load";
const char kImageDCStartOperation[] = "DCStart";
const char kImageUnloadOperation[] = "Unload";

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

}  // namespace state

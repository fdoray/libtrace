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

#include <memory>

#include "base/bind_object.h"
#include "base/logging.h"
#include "event/utils.h"
#include "parser/parser.h"
#include "parser/etw/etw_parser.h"
#include "state/current_state.h"

int wmain(int argc, wchar_t* argv[], wchar_t* /*envp */ []) {
  parser::Parser parser;

  std::unique_ptr<parser::ParserImpl> etw_parser(new parser::etw::ETWParser());
  parser.RegisterParser(std::move(etw_parser));

  for (int i = 1; i < argc; ++i) {
    if (!parser.AddTraceFile(argv[i])) {
      LOG(ERROR) << "Could not parse trace '" << argv[i] << "'.";
      return -1;
    }
  }

  state::CurrentState current_state;
  parser.Parse(base::BindObject(&state::CurrentState::OnEvent, &current_state));

  return 0;
}

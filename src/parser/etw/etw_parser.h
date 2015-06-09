// Copyright (c) 2014 The LibTrace Authors.
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

#ifndef PARSER_ETW_ETW_PARSER_H_
#define PARSER_ETW_ETW_PARSER_H_

// Restrict the import to the windows basic includes.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // NOLINT
#include <evntcons.h>  // NOLINT

#include <functional>
#include <string>
#include <vector>

#include "base/base.h"
#include "event/event.h"
#include "parser/parser.h"

namespace parser {
namespace etw {

// Generate Event objects from ETW trace files.
class ETWParser : public parser::ParserImpl {
 public:
  typedef parser::ParserImpl::EventCallback EventCallback;

  // Constuctor.
  ETWParser();

  // Adds a trace file to the list of traces to parse.
  // @param path absolute path to the trace file.
  bool AddTraceFile(const std::wstring& path) override;

  // Parses the trace files added with AddTraceFile() and sends the resulting
  // events to the provided callback.
  // @param callback a callback that will receive the decoded events.
  void Parse(const EventCallback& callback) override;

 private:
  // Called by the ETW API when an event is read.
  // @param pevent the read event.
  static void WINAPI ProcessEvent(PEVENT_RECORD pevent);

  // Trace files to consume.
  std::vector<std::wstring> traces_;

  // Active event callback.
  const EventCallback* event_callback_;

  // System timestamp of the first event. Used for timestamp conversion.
  uint64_t first_event_system_ts_;

  // RAW timestamp of the first event. Used for timestamp conversion.
  uint64_t first_event_raw_ts_;

  // Period of the high-resolution performance counter, in ns. Used for
  // timestamp conversion.
  double perf_period_;

  DISALLOW_COPY_AND_ASSIGN(ETWParser);
};

}  // namespace etw
}  // namespace parser

#endif  // PARSER_ETW_ETW_PARSER_H_

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

#include "parser/etw/etw_parser.h"

#include "base/logging.h"
#include "base/string_utils.h"
#include "base/win/error_string.h"
#include "event/event.h"
#include "event/value.h"
#include "parser/etw/etw_raw_kernel_payload_decoder.h"

namespace parser {
namespace etw {

namespace {

using event::Event;
using event::IntValue;
using event::StringValue;
using event::StructValue;
using event::Timestamp;
using event::UCharValue;
using event::ULongValue;
using event::Value;

// Constant used to convert the frequency of the high-resolution performance
// counter to a period, in ns.
const double kPerfPeriodMultiplier = 10000000.0;

//  Convert a GUID to a string representation.
std::string GuidToString(const GUID& guid) {
  const int kMaxGuidStringLength = 38;
  char buffer[kMaxGuidStringLength];
  sprintf_s(buffer, kMaxGuidStringLength,
      "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      guid.Data1,
      guid.Data2,
      guid.Data3,
      guid.Data4[0],
      guid.Data4[1],
      guid.Data4[2],
      guid.Data4[3],
      guid.Data4[4],
      guid.Data4[5],
      guid.Data4[6],
      guid.Data4[7]);
  return std::string(buffer);
}

bool DecodeRawETWPayload(const std::string& provider_id,
                         unsigned char version,
                         unsigned char opcode,
                         bool is_64_bit,
                         const char* payload,
                         size_t payload_size,
                         std::string* operation,
                         std::string* category,
                         std::unique_ptr<event::Value>* decoded_payload) {
  if (DecodeRawETWKernelPayload(
          provider_id, version, opcode, is_64_bit, payload, payload_size,
          operation, category, decoded_payload)) {
    return true;
  }
  return false;
}

}  // namespace

ETWParser::ETWParser()
    : event_callback_(nullptr),
      first_event_system_ts_(0),
      first_event_raw_ts_(0),
      perf_period_(0) {
}

bool ETWParser::AddTraceFile(const std::wstring& path) {
  if (!traces_.empty()) {
    LOG(ERROR) << "ETW Parser can only read one trace at a time.";
    return false;
  }
  if (!base::WStringEndsWith(path, L".etl")) {
    return false;
  }
  traces_.push_back(path);
  return true;
}

void ETWParser::Parse(const EventCallback& callback) {
  DCHECK(event_callback_ == nullptr);
  DCHECK_EQ(first_event_system_ts_, 0);
  DCHECK_EQ(first_event_raw_ts_, 0);
  DCHECK_EQ(perf_period_, 0);
  DCHECK_LE(traces_.size(), 1);

  // Initialize the parser.
  event_callback_ = &callback;

  // Open all trace files, and keep handles in a vector.
  bool error = false;
  std::vector<TRACEHANDLE> handles;
  for (size_t i = 0; i < traces_.size(); ++i) {
    EVENT_TRACE_LOGFILE trace;
    ::memset(&trace, 0, sizeof(trace));
    trace.LogFileName = const_cast<LPWSTR>(traces_[i].c_str());
    trace.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD |
        PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    trace.EventRecordCallback = &ETWParser::ProcessEvent;
    trace.Context = this;

    TRACEHANDLE th = ::OpenTrace(&trace);
    if (th == INVALID_PROCESSTRACE_HANDLE) {
      LOG(WARNING) << "OpenTrace failed with error "
                   << base::GetLastWindowsErrorString();
      error = true;
      break;
    }

    first_event_system_ts_ = trace.LogfileHeader.StartTime.QuadPart;
    perf_period_ =
        kPerfPeriodMultiplier / trace.LogfileHeader.PerfFreq.QuadPart;

    handles.push_back(th);
  }

  if (!error && !handles.empty()) {
    // Ask the ETW API to consume all traces and call the registered callbacks.
    ULONG status = ::ProcessTrace(&handles[0], traces_.size(), NULL, NULL);
    if (status != ERROR_SUCCESS) {
      LOG(ERROR) << "ProcessTrace failed with error " << status << ".";
    }
  }

  // Close all trace files.
  for (size_t i = 0; i < handles.size(); ++i) {
    DCHECK(handles[i] != NULL);
    ::CloseTrace(handles[i]);
  }

  // Reset the parser.
  event_callback_ = nullptr;
  first_event_system_ts_ = 0;
  first_event_raw_ts_ = 0;
  perf_period_ = 0;
}

void WINAPI ETWParser::ProcessEvent(PEVENT_RECORD pevent) {
  DCHECK(pevent != NULL);
  ETWParser* event_parser = reinterpret_cast<ETWParser*>(pevent->UserContext);
  DCHECK(event_parser->event_callback_ != nullptr);

  // Record the timestamp of the first event. It will be used to convert raw
  // timestamps to system time.
  if (event_parser->first_event_raw_ts_ == 0)
    event_parser->first_event_raw_ts_ = pevent->EventHeader.TimeStamp.QuadPart;

  // Decode the payload of the event.
  std::string operation;
  std::string category;

  std::string provider_guid = GuidToString(pevent->EventHeader.ProviderId);
  std::unique_ptr<Value> payload;
  if (!DecodeRawETWPayload(
      provider_guid,
      pevent->EventHeader.EventDescriptor.Version,
      pevent->EventHeader.EventDescriptor.Opcode,
      (pevent->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0,
      reinterpret_cast<const char*>(pevent->UserData),
      pevent->UserDataLength,
      &operation,
      &category,
      &payload)) {
      return;
  }

  // Generate the event header fields.
  std::unique_ptr<StructValue> header(new StructValue());
  header->AddField<StringValue>(event::kOperationFieldName, operation);
  header->AddField<StringValue>(event::kCategoryFieldName, category);
  header->AddField<ULongValue>(event::kProcessIdFieldName, pevent->EventHeader.ProcessId);
  header->AddField<ULongValue>(event::kThreadIdFieldName, pevent->EventHeader.ThreadId);
  header->AddField<UCharValue>(event::kProcessorNumberFieldName,
      pevent->BufferContext.ProcessorNumber);

  // Compute the timestamp.
  uint64_t raw_ts = pevent->EventHeader.TimeStamp.QuadPart;
  uint64_t system_ts = event_parser->first_event_system_ts_ +
      static_cast<uint64_t>((raw_ts - event_parser->first_event_raw_ts_) *
          event_parser->perf_period_);

  // Create the event with decoded fields.
  Event event(Timestamp(system_ts), std::move(header), std::move(payload));

  // Send the event to the callback.
  (*event_parser->event_callback_)(event);
}

}  // namespace etw
}  // namespace parser

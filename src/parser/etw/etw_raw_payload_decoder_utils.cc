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

#include "parser/etw/etw_raw_payload_decoder_utils.h"

namespace parser {
namespace etw {

namespace {

using event::IntValue;
using event::UCharValue;
using event::UIntValue;
using event::ULongValue;
using event::ShortValue;
using event::StructValue;
using event::WStringValue;
using event::Value;

}  // namespace

bool DecodeUInteger(const std::string& name,
                    bool is_64_bit,
                    Decoder* decoder,
                    StructValue* fields) {
  if (is_64_bit)
    return Decode<ULongValue>(name, decoder, fields);
  return Decode<UIntValue>(name, decoder, fields);
}

bool DecodeW16String(const std::string& name,
                     Decoder* decoder,
                     StructValue* fields) {
  DCHECK(decoder != NULL);
  DCHECK(fields != NULL);

  std::unique_ptr<WStringValue> decoded(decoder->DecodeW16String());

  if (decoded.get() == NULL ||
      !fields->AddField(name, std::move(decoded))) {
    return false;
  }

  return true;
}

bool DecodeFixedW16String(const std::string& name,
                          size_t length,
                          Decoder* decoder,
                          StructValue* fields) {
  DCHECK(decoder != NULL);
  DCHECK(fields != NULL);

  std::unique_ptr<WStringValue> decoded(decoder->DecodeFixedW16String(length));

  if (decoded.get() == NULL ||
      !fields->AddField(name, std::move(decoded))) {
    return false;
  }

  return true;
}

bool DecodeSID(const std::string& name,
               bool is_64_bit,
               Decoder* decoder,
               StructValue* fields) {
  // Check the minimal SID length to avoid out-of-bound accesses.
  if (decoder->RemainingBytes() < 3 * 8)
    return false;

  // Decode the TOKEN_USER structure.
  std::unique_ptr<StructValue> sid(new StructValue);
  if (!DecodeUInteger("PSid", is_64_bit, decoder, sid.get()) ||
      !Decode<UIntValue>("Attributes", decoder, sid.get())) {
    return false;
  }

  // Skip padding.
  if (is_64_bit) {
    std::unique_ptr<UIntValue> padding(decoder->Decode<UIntValue>());
    if (padding.get() == NULL)
      return false;
  }

  // Decode the SID structure.
  unsigned char revision = decoder->Lookup(0);
  unsigned char subAuthorityCount = decoder->Lookup(1);
  const int kSID_REVISION = 1;
  const int kSID_MAX_SUB_AUTHORITIES = 15;
  DCHECK_EQ(revision, kSID_REVISION);
  DCHECK_LE(subAuthorityCount, kSID_MAX_SUB_AUTHORITIES);

  unsigned int length = 4 * subAuthorityCount + 8;
  if (!DecodeArray<UCharValue>("Sid", length, decoder, sid.get()))
    return false;

  // Returns a struct containing all decoded fields.
  return fields->AddField(name, std::move(sid));
}

bool DecodeSystemTime(const std::string& name,
                      Decoder* decoder,
                      StructValue* fields) {
  // Decode the SystemTime structure.
  std::unique_ptr<StructValue> system_time(new StructValue);
  if (!Decode<ShortValue>("wYear", decoder, system_time.get()) ||
      !Decode<ShortValue>("wMonth", decoder, system_time.get()) ||
      !Decode<ShortValue>("wDayOfWeek", decoder, system_time.get()) ||
      !Decode<ShortValue>("wDay", decoder, system_time.get()) ||
      !Decode<ShortValue>("wHour", decoder, system_time.get()) ||
      !Decode<ShortValue>("wMinute", decoder, system_time.get()) ||
      !Decode<ShortValue>("wSecond", decoder, system_time.get()) ||
      !Decode<ShortValue>("wMilliseconds", decoder, system_time.get())) {
    return false;
  }

  // Returns a struct containing all decoded fields.
  return fields->AddField(name, std::move(system_time));
}

bool DecodeTimeZoneInformation(const std::string& name,
                               Decoder* decoder,
                               StructValue* fields) {

  // Decode the TimeZone structure.
  std::unique_ptr<StructValue> timezone(new StructValue);
  if (!Decode<IntValue>("Bias", decoder, timezone.get()) ||
      !DecodeFixedW16String("StandardName", 32, decoder, timezone.get()) ||
      !DecodeSystemTime("StandardDate", decoder, timezone.get()) ||
      !Decode<IntValue>("StandardBias", decoder, timezone.get()) ||
      !DecodeFixedW16String("DaylightName", 32, decoder, timezone.get()) ||
      !DecodeSystemTime("DaylightDate",  decoder, timezone.get()) ||
      !Decode<IntValue>("DaylightBias", decoder, timezone.get())) {
    return false;
  }

  // Returns a struct containing all decoded fields.
  return fields->AddField(name, std::move(timezone));
}

}  // namespace etw
}  // namespace parser

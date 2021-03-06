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

#include "parser/etw/etw_raw_kernel_payload_decoder.h"

#include <memory>

#include "base/logging.h"
#include "event/utils.h"
#include "event/value.h"
#include "gtest/gtest.h"

namespace parser {
namespace etw {

namespace {

using event::ArrayValue;
using event::CharValue;
using event::IntValue;
using event::LongValue;
using event::ShortValue;
using event::StringValue;
using event::StructValue;
using event::UCharValue;
using event::UIntValue;
using event::ULongValue;
using event::UShortValue;
using event::Value;
using event::WStringValue;

// ETW payload version.
const unsigned char kVersion0 = 0;
const unsigned char kVersion1 = 1;
const unsigned char kVersion2 = 2;
const unsigned char kVersion3 = 3;
const unsigned char kVersion4 = 4;
const unsigned char kVersion5 = 5;

// Flag indicating to decode 64-bit integer.
const bool k32bit = false;
const bool k64bit = true;

// Constants for EventTrace events.
const std::string kEventTraceEventProviderId =
    "68FDD900-4A3E-11D1-84F4-0000F80464E3";
const unsigned char kEventTraceEventHeaderOpcode = 0;
const unsigned char kEventTraceEventExtensionOpcode = 5;

// Constants for Image events.
const std::string kImageProviderId = "2CB15D1D-5FC1-11D2-ABE1-00A0C911F518";
const unsigned char kImageUnloadOpcode = 2;
const unsigned char kImageDCStartOpcode = 3;
const unsigned char kImageDCEndOpcode = 4;
const unsigned char kImageLoadOpcode = 10;
const unsigned char kImageKernelBaseOpcode = 33;

// Constants for PerfInfo events.
const std::string kPerfInfoProviderId = "CE1DBFB4-137E-4DA6-87B0-3F59AA102CBC";
const unsigned char kPerfInfoSampleProfOpcode = 46;
const unsigned char kPerfInfoISRMSIOpcode = 50;
const unsigned char kPerfInfoSysClEnterOpcode = 51;
const unsigned char kPerfInfoSysClExitOpcode = 52;
const unsigned char kPerfInfoDebuggerEnabledOpcode = 58;
const unsigned char kPerfInfoThreadedDPCOpcode = 66;
const unsigned char kPerfInfoISROpcode = 67;
const unsigned char kPerfInfoDPCOpcode = 68;
const unsigned char kPerfInfoTimerDPCOpcode = 69;
const unsigned char kPerfInfoCollectionStartOpcode = 73;
const unsigned char kPerfInfoCollectionEndOpcode = 74;
const unsigned char kPerfInfoCollectionStartSecondOpcode = 75;
const unsigned char kPerfInfoCollectionEndSecondOpcode = 76;

// Constants for Process events.
const char kProcessProviderId[] = "3D6FA8D0-FE05-11D0-9DDA-00C04FD7BA7C";
const unsigned char kProcessStartOpcode = 1;
const unsigned char kProcessEndOpcode = 2;
const unsigned char kProcessDCStartOpcode = 3;
const unsigned char kProcessDCEndOpcode = 4;
const unsigned char kProcessTerminateOpcode = 11;
const unsigned char kProcessPerfCtrOpcode = 32;
const unsigned char kProcessPerfCtrRundownOpcode = 33;
const unsigned char kProcessDefunctOpcode = 39;

// Constants for Thread events.
const std::string kThreadProviderId = "3D6FA8D1-FE05-11D0-9DDA-00C04FD7BA7C";
const unsigned char kThreadStartOpcode = 1;
const unsigned char kThreadEndOpcode = 2;
const unsigned char kThreadDCStartOpcode = 3;
const unsigned char kThreadDCEndOpcode = 4;
const unsigned char kThreadCSwitchOpcode = 36;
const unsigned char kThreadSpinLockOpcode = 41;
const unsigned char kThreadSetPriorityOpcode = 48;
const unsigned char kThreadSetBasePriorityOpcode = 49;
const unsigned char kThreadReadyThreadOpcode = 50;
const unsigned char kThreadSetPagePriorityOpcode = 51;
const unsigned char kThreadSetIoPriorityOpcode = 52;
const unsigned char kThreadAutoBoostSetFloorOpcode = 66;
const unsigned char kThreadAutoBoostClearFloorOpcode = 67;
const unsigned char kThreadAutoBoostEntryExhaustionOpcode = 68;

// Constants for Tcplp events.
const std::string kTcplpProviderId = "9A280AC0-C8E0-11D1-84E2-00C04FB998A2";
const unsigned char kTcplpSendIPV4Opcode = 10;
const unsigned char kTcplpRecvIPV4Opcode = 11;
const unsigned char kTcplpConnectIPV4Opcode = 12;
const unsigned char kTcplpDisconnectIPV4Opcode = 13;
const unsigned char kTcplpRetransmitIPV4Opcode = 14;
const unsigned char kTcplpTCPCopyIPV4Opcode = 18;

// Constants for Registry events.
const std::string kRegistryProviderId = "AE53722E-C863-11D2-8659-00C04FA321A1";
const unsigned char kRegistryCreateOpcode = 10;
const unsigned char kRegistryOpenOpcode = 11;
const unsigned char kRegistryQueryOpcode = 13;
const unsigned char kRegistrySetValueOpcode = 14;
const unsigned char kRegistryQueryValueOpcode = 16;
const unsigned char kRegistryEnumerateKeyOpcode = 17;
const unsigned char kRegistryEnumerateValueKeyOpcode = 18;
const unsigned char kRegistryQueryMultipleValueOpcode = 19;
const unsigned char kRegistrySetInformationOpcode = 20;
const unsigned char kRegistryFlushOpcode = 21;
const unsigned char kRegistryKCBCreateOpcode = 22;
const unsigned char kRegistryKCBDeleteOpcode = 23;
const unsigned char kRegistryKCBRundownEndOpcode = 25;
const unsigned char kRegistryCloseOpcode = 27;
const unsigned char kRegistrySetSecurityOpcode = 28;
const unsigned char kRegistryQuerySecurityOpcode = 29;
const unsigned char kRegistryCountersOpcode = 34;
const unsigned char kRegistryConfigOpcode = 35;

// Constants for FileIO events.
const std::string kFileIOProviderId = "90CBDC39-4A3E-11D1-84F4-0000F80464E3";
const unsigned char kFileIOFileCreateOpcode = 32;
const unsigned char kFileIOFileDeleteOpcode = 35;
const unsigned char kFileIOFileRundownOpcode = 36;
const unsigned char kFileIOCreateOpcode = 64;
const unsigned char kFileIOCleanupOpcode = 65;
const unsigned char kFileIOCloseOpcode = 66;
const unsigned char kFileIOReadOpcode = 67;
const unsigned char kFileIOWriteOpcode = 68;
const unsigned char kFileIOSetInfoOpcode = 69;
const unsigned char kFileIODeleteOpcode = 70;
const unsigned char kFileIORenameOpcode = 71;
const unsigned char kFileIODirEnumOpcode = 72;
const unsigned char kFileIOFlushOpcode = 73;
const unsigned char kFileIOQueryInfoOpcode = 74;
const unsigned char kFileIOFSControlOpcode = 75;
const unsigned char kFileIOOperationEndOpcode = 76;
const unsigned char kFileIODirNotifyOpcode = 77;
const unsigned char kFileIOUnknown78Opcode = 78;
const unsigned char kFileIODletePathOpcode = 79;
const unsigned char kFileIORenamePathOpcode = 80;

// Constants for DiskIO events.
const std::string kDiskIOProviderId = "3D6FA8D4-FE05-11D0-9DDA-00C04FD7BA7C";
const unsigned char kDiskIOReadOpcode = 10;
const unsigned char kDiskIOWriteOpcode = 11;
const unsigned char kDiskIOReadInitOpcode = 12;
const unsigned char kDiskIOWriteInitOpcode = 13;
const unsigned char kDiskIOFlushBuffersOpcode = 14;
const unsigned char kDiskIOFlushInitOpcode = 15;

// Constants for StackWalk events.
const std::string kStackWalkProviderId = "DEF2FE46-7BD6-4B80-BD94-F57FE20D0CE3";
const unsigned char kStackWalkStackOpcode = 32;

// Constants for PageFault events.
const std::string kPageFaultProviderId = "3D6FA8D3-FE05-11D0-9DDA-00C04FD7BA7C";
const unsigned char kPageFaultTransitionFaultOpcode = 10;
const unsigned char kPageFaultDemandZeroFaultOpcode = 11;
const unsigned char kPageFaultCopyOnWriteOpcode = 12;
const unsigned char kPageFaultGuardPageFaultOpcode = 13;
const unsigned char kPageFaultHardPageFaultOpcode = 14;
const unsigned char kPageFaultAccessViolationOpcode = 15;
const unsigned char kPageFaultHardFaultOpcode = 32;
const unsigned char kPageFaultVirtualAllocOpcode = 98;
const unsigned char kPageFaultVirtualFreeOpcode = 99;

const unsigned char kEventTraceEventHeaderPayloadV2[] = {
    0x00, 0x00, 0x01, 0x00, 0x06, 0x01, 0x01, 0x05,
    0xB1, 0x1D, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x3B, 0x2E, 0xCD, 0x14, 0x58, 0x2C, 0xCF, 0x01,
    0x61, 0x61, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0xB6, 0x01, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x1F, 0x00, 0x00, 0x00, 0xA0, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2C, 0x01, 0x00, 0x00, 0x40, 0x00, 0x74, 0x00,
    0x7A, 0x00, 0x72, 0x00, 0x65, 0x00, 0x73, 0x00,
    0x2E, 0x00, 0x64, 0x00, 0x6C, 0x00, 0x6C, 0x00,
    0x2C, 0x00, 0x2D, 0x00, 0x31, 0x00, 0x31, 0x00,
    0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x74, 0x00, 0x7A, 0x00, 0x72, 0x00,
    0x65, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x64, 0x00,
    0x6C, 0x00, 0x6C, 0x00, 0x2C, 0x00, 0x2D, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x31, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC4, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x59, 0x43, 0x25, 0xA2, 0xC0, 0x2B, 0xCF, 0x01,
    0x7D, 0x46, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2D, 0x64, 0x99, 0x04, 0x58, 0x2C, 0xCF, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x52, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6F, 0x00,
    0x67, 0x00, 0x67, 0x00, 0x65, 0x00, 0x72, 0x00,
    0x00, 0x00, 0x43, 0x00, 0x3A, 0x00, 0x5C, 0x00,
    0x6B, 0x00, 0x65, 0x00, 0x72, 0x00, 0x6E, 0x00,
    0x65, 0x00, 0x6C, 0x00, 0x2E, 0x00, 0x65, 0x00,
    0x74, 0x00, 0x6C, 0x00, 0x00, 0x00 };

const unsigned char kEventTraceEventHeaderPayload32bitsV2[] = {
    0x00, 0x00, 0x01, 0x00, 0x06, 0x01, 0x01, 0x05,
    0xB0, 0x1D, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x11, 0x2C, 0xD5, 0x61, 0xC8, 0x08, 0xCC, 0x01,
    0x61, 0x61, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5A, 0x09, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x2C, 0x01, 0x00, 0x00, 0x40, 0x00, 0x74, 0x00,
    0x7A, 0x00, 0x72, 0x00, 0x65, 0x00, 0x73, 0x00,
    0x2E, 0x00, 0x64, 0x00, 0x6C, 0x00, 0x6C, 0x00,
    0x2C, 0x00, 0x2D, 0x00, 0x31, 0x00, 0x31, 0x00,
    0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x74, 0x00, 0x7A, 0x00, 0x72, 0x00,
    0x65, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x64, 0x00,
    0x6C, 0x00, 0x6C, 0x00, 0x2C, 0x00, 0x2D, 0x00,
    0x31, 0x00, 0x31, 0x00, 0x31, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC4, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x7F, 0x43, 0x9B, 0xDF, 0xAF, 0x05, 0xCC, 0x01,
    0x9D, 0xAC, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2C, 0x34, 0xA3, 0x60, 0xC8, 0x08, 0xCC, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4D, 0x00, 0x61, 0x00, 0x6B, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x54, 0x00, 0x65, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x20, 0x00, 0x44, 0x00, 0x61, 0x00,
    0x74, 0x00, 0x61, 0x00, 0x20, 0x00, 0x53, 0x00,
    0x65, 0x00, 0x73, 0x00, 0x73, 0x00, 0x69, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x63, 0x00,
    0x3A, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x72, 0x00,
    0x63, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x75, 0x00, 0x6E, 0x00, 0x6B, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x72, 0x00, 0x63, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x61, 0x00, 0x77, 0x00, 0x62, 0x00,
    0x75, 0x00, 0x63, 0x00, 0x6B, 0x00, 0x5C, 0x00,
    0x6C, 0x00, 0x6F, 0x00, 0x67, 0x00, 0x5F, 0x00,
    0x6C, 0x00, 0x69, 0x00, 0x62, 0x00, 0x5C, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x5F, 0x00, 0x64, 0x00, 0x61, 0x00, 0x74, 0x00,
    0x61, 0x00, 0x5C, 0x00, 0x69, 0x00, 0x6D, 0x00,
    0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x5F, 0x00,
    0x64, 0x00, 0x61, 0x00, 0x74, 0x00, 0x61, 0x00,
    0x5F, 0x00, 0x33, 0x00, 0x32, 0x00, 0x5F, 0x00,
    0x76, 0x00, 0x30, 0x00, 0x2E, 0x00, 0x65, 0x00,
    0x74, 0x00, 0x6C, 0x00, 0x00, 0x00 };

const unsigned char kEventTraceEventExtensionPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00 };

const unsigned char kEventTraceEventExtensionPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00 };

const unsigned char kImageUnloadPayloadV2[] = {
    0x00, 0x00, 0x78, 0xF7, 0xFE, 0x07, 0x00, 0x00,
    0x00, 0x20, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x44, 0x17, 0x00, 0x00, 0xA1, 0x77, 0x0E, 0x00,
    0xFE, 0xDE, 0x5B, 0x4A, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0xF7, 0xFE, 0x07, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x57, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x64, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x73, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x33, 0x00,
    0x32, 0x00, 0x5C, 0x00, 0x77, 0x00, 0x62, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x5C, 0x00, 0x66, 0x00,
    0x61, 0x00, 0x73, 0x00, 0x74, 0x00, 0x70, 0x00,
    0x72, 0x00, 0x6F, 0x00, 0x78, 0x00, 0x2E, 0x00,
    0x64, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x00, 0x00
    };

const unsigned char kImageUnloadPayloadV3[] = {
    0x00, 0x00, 0xF3, 0xA3, 0xFC, 0x7F, 0x00, 0x00,
    0x00, 0x40, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF8, 0x07, 0x00, 0x00, 0x7B, 0x2E, 0x0E, 0x00,
    0xB8, 0xDE, 0x15, 0x52, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xF3, 0xA3, 0xFC, 0x7F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x57, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x64, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x73, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x33, 0x00,
    0x32, 0x00, 0x5C, 0x00, 0x77, 0x00, 0x62, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x5C, 0x00, 0x66, 0x00,
    0x61, 0x00, 0x73, 0x00, 0x74, 0x00, 0x70, 0x00,
    0x72, 0x00, 0x6F, 0x00, 0x78, 0x00, 0x2E, 0x00,
    0x64, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x00, 0x00
    };

const unsigned char kImageDCStartPayload32bitsV0[] = {
    0x00, 0x00, 0x16, 0x01, 0x00, 0xE0, 0x19, 0x00,
    0x43, 0x00, 0x3A, 0x00, 0x5C, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x64, 0x00, 0x65, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x61, 0x00, 0x77, 0x00, 0x62, 0x00,
    0x75, 0x00, 0x63, 0x00, 0x6B, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x72, 0x00, 0x63, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x61, 0x00, 0x77, 0x00, 0x62, 0x00,
    0x75, 0x00, 0x63, 0x00, 0x6B, 0x00, 0x5C, 0x00,
    0x44, 0x00, 0x65, 0x00, 0x62, 0x00, 0x75, 0x00,
    0x67, 0x00, 0x5C, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x73, 0x00, 0x74, 0x00, 0x5F, 0x00, 0x70, 0x00,
    0x72, 0x00, 0x6F, 0x00, 0x67, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x6D, 0x00, 0x2E, 0x00, 0x65, 0x00,
    0x78, 0x00, 0x65, 0x00, 0x00, 0x00 };

const unsigned char kImageDCStartPayload32bitsV1[] = {
    0x00, 0x00, 0x16, 0x01, 0x00, 0xE0, 0x19, 0x00,
    0xDC, 0x1D, 0x00, 0x00, 0x43, 0x00, 0x3A, 0x00,
    0x5C, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x64, 0x00,
    0x65, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x72, 0x00,
    0x63, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x44, 0x00, 0x65, 0x00,
    0x62, 0x00, 0x75, 0x00, 0x67, 0x00, 0x5C, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x5F, 0x00, 0x70, 0x00, 0x72, 0x00, 0x6F, 0x00,
    0x67, 0x00, 0x72, 0x00, 0x61, 0x00, 0x6D, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
    0x00, 0x00 };

const unsigned char kImageDCStartPayload32bitsV2[] = {
    0x00, 0x00, 0x16, 0x01, 0x00, 0xE0, 0x19, 0x00,
    0xDC, 0x1D, 0x00, 0x00, 0x67, 0x68, 0xA2, 0x4B,
    0xBE, 0xBA, 0xFE, 0xCA, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x3A, 0x00,
    0x5C, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x64, 0x00,
    0x65, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x72, 0x00,
    0x63, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x44, 0x00, 0x65, 0x00,
    0x62, 0x00, 0x75, 0x00, 0x67, 0x00, 0x5C, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x5F, 0x00, 0x70, 0x00, 0x72, 0x00, 0x6F, 0x00,
    0x67, 0x00, 0x72, 0x00, 0x61, 0x00, 0x6D, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
    0x00, 0x00 };

const unsigned char kImageDCStartPayloadV2[] = {
    0x00, 0x80, 0xE0, 0x02, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x60, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x45, 0xA2, 0x55, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x52, 0x00,
    0x6F, 0x00, 0x6F, 0x00, 0x74, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x33, 0x00, 0x32, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x6F, 0x00,
    0x73, 0x00, 0x6B, 0x00, 0x72, 0x00, 0x6E, 0x00,
    0x6C, 0x00, 0x2E, 0x00, 0x65, 0x00, 0x78, 0x00,
    0x65, 0x00, 0x00, 0x00 };

const unsigned char kImageDCStartPayloadV3[] = {
    0x00, 0x00, 0x45, 0x77, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x18, 0xBF, 0x16, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x45, 0x77, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x44, 0x00, 0x65, 0x00, 0x76, 0x00,
    0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x5C, 0x00,
    0x48, 0x00, 0x61, 0x00, 0x72, 0x00, 0x64, 0x00,
    0x64, 0x00, 0x69, 0x00, 0x73, 0x00, 0x6B, 0x00,
    0x56, 0x00, 0x6F, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x6D, 0x00, 0x65, 0x00, 0x34, 0x00, 0x5C, 0x00,
    0x57, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x64, 0x00,
    0x6F, 0x00, 0x77, 0x00, 0x73, 0x00, 0x5C, 0x00,
    0x53, 0x00, 0x79, 0x00, 0x73, 0x00, 0x57, 0x00,
    0x4F, 0x00, 0x57, 0x00, 0x36, 0x00, 0x34, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x64, 0x00,
    0x6C, 0x00, 0x6C, 0x00, 0x2E, 0x00, 0x64, 0x00,
    0x6C, 0x00, 0x6C, 0x00, 0x00, 0x00 };

const unsigned char kImageDCEndPayloadV2[] = {
    0x00, 0x90, 0xE1, 0x02, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x50, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xB3, 0xCB, 0x54, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x52, 0x00,
    0x6F, 0x00, 0x6F, 0x00, 0x74, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x33, 0x00, 0x32, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x6F, 0x00,
    0x73, 0x00, 0x6B, 0x00, 0x72, 0x00, 0x6E, 0x00,
    0x6C, 0x00, 0x2E, 0x00, 0x65, 0x00, 0x78, 0x00,
    0x65, 0x00, 0x00, 0x00 };

const unsigned char kImageDCEndPayloadV3[] = {
    0x00, 0xF0, 0x86, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xD6, 0x20, 0x71, 0x00,
    0x9C, 0x8D, 0x71, 0x52, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x52, 0x00,
    0x6F, 0x00, 0x6F, 0x00, 0x74, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x33, 0x00, 0x32, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x6F, 0x00,
    0x73, 0x00, 0x6B, 0x00, 0x72, 0x00, 0x6E, 0x00,
    0x6C, 0x00, 0x2E, 0x00, 0x65, 0x00, 0x78, 0x00,
    0x65, 0x00, 0x00, 0x00 };

const unsigned char kImageLoadPayloadV0[] = {
    0x00, 0x00, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xE0, 0x19, 0x00, 0x43, 0x00, 0x3A, 0x00,
    0x5C, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x64, 0x00,
    0x65, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x72, 0x00,
    0x63, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x61, 0x00,
    0x77, 0x00, 0x62, 0x00, 0x75, 0x00, 0x63, 0x00,
    0x6B, 0x00, 0x5C, 0x00, 0x44, 0x00, 0x65, 0x00,
    0x62, 0x00, 0x75, 0x00, 0x67, 0x00, 0x5C, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x5F, 0x00, 0x70, 0x00, 0x72, 0x00, 0x6F, 0x00,
    0x67, 0x00, 0x72, 0x00, 0x61, 0x00, 0x6D, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
    0x00, 0x00 };

const unsigned char kImageLoadPayloadV2[] = {
    0x00, 0x00, 0x40, 0x71, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF4, 0x0E, 0x00, 0x00, 0x9A, 0xFE, 0x00, 0x00,
    0xE4, 0xC3, 0x5B, 0x4A, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x40, 0x71,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x57, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x64, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x73, 0x00,
    0x5C, 0x00, 0x53, 0x00, 0x79, 0x00, 0x73, 0x00,
    0x57, 0x00, 0x4F, 0x00, 0x57, 0x00, 0x36, 0x00,
    0x34, 0x00, 0x5C, 0x00, 0x77, 0x00, 0x73, 0x00,
    0x63, 0x00, 0x69, 0x00, 0x73, 0x00, 0x76, 0x00,
    0x69, 0x00, 0x66, 0x00, 0x2E, 0x00, 0x64, 0x00,
    0x6C, 0x00, 0x6C, 0x00, 0x00, 0x00 };

const unsigned char kImageLoadPayloadV3[] = {
    0x00, 0x00, 0x49, 0x3A, 0xF7, 0x7F, 0x00, 0x00,
    0x00, 0x90, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x8C, 0x0A, 0x00, 0x00, 0x31, 0x6E, 0x07, 0x00,
    0x9D, 0x9D, 0x10, 0x50, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x3A, 0xF7, 0x7F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5C, 0x00, 0x44, 0x00, 0x65, 0x00, 0x76, 0x00,
    0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x5C, 0x00,
    0x48, 0x00, 0x61, 0x00, 0x72, 0x00, 0x64, 0x00,
    0x64, 0x00, 0x69, 0x00, 0x73, 0x00, 0x6B, 0x00,
    0x56, 0x00, 0x6F, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x6D, 0x00, 0x65, 0x00, 0x34, 0x00, 0x5C, 0x00,
    0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x67, 0x00,
    0x72, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x20, 0x00,
    0x46, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00,
    0x73, 0x00, 0x20, 0x00, 0x28, 0x00, 0x78, 0x00,
    0x38, 0x00, 0x36, 0x00, 0x29, 0x00, 0x5C, 0x00,
    0x57, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x64, 0x00,
    0x6F, 0x00, 0x77, 0x00, 0x73, 0x00, 0x20, 0x00,
    0x4B, 0x00, 0x69, 0x00, 0x74, 0x00, 0x73, 0x00,
    0x5C, 0x00, 0x38, 0x00, 0x2E, 0x00, 0x30, 0x00,
    0x5C, 0x00, 0x57, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x64, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x73, 0x00,
    0x20, 0x00, 0x50, 0x00, 0x65, 0x00, 0x72, 0x00,
    0x66, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x6D, 0x00,
    0x61, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x20, 0x00, 0x54, 0x00, 0x6F, 0x00, 0x6F, 0x00,
    0x6C, 0x00, 0x6B, 0x00, 0x69, 0x00, 0x74, 0x00,
    0x5C, 0x00, 0x78, 0x00, 0x70, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x66, 0x00, 0x2E, 0x00, 0x65, 0x00,
    0x78, 0x00, 0x65, 0x00, 0x00, 0x00 };

const unsigned char kImageKernelBasePayloadV2[] = {
    0x00, 0x90, 0xE1, 0x02, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kPerfInfoSampleProfPayload32bitsV2[] = {
    0x45, 0x1A, 0xFC, 0x82, 0xB4, 0x0C, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00 };

const unsigned char kPerfInfoSampleProfPayloadV2[] = {
    0x4B, 0xAB, 0x8C, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x70, 0x1F, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00
    };

const unsigned char kPerfInfoISRMSIPayload32bitsV2[] = {
    0xF8, 0x4F, 0xDE, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0x0E, 0xA9, 0x8C, 0x8B, 0x01, 0xB0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kPerfInfoISRMSIPayloadV2[] = {
    0xEB, 0xED, 0x3A, 0xA8, 0x66, 0x04, 0x00, 0x00,
    0x20, 0x7E, 0x93, 0x00, 0x00, 0xF8, 0xFF, 0xFF,
    0x01, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kPerfInfoSysClEnterPayload32bitsV2[] = {
    0x4F, 0x87, 0xA7, 0x82 };

const unsigned char kPerfInfoSysClEnterPayloadV2[] = {
    0x24, 0x1D, 0x90, 0x74, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kPerfInfoSysClExitPayload32bitsV2[] = {
    0x03, 0x01, 0x00, 0x00 };

const unsigned char kPerfInfoSysClExitPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kPerfInfoISRPayload32bitsV2[] = {
    0xD4, 0xC0, 0xB1, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0x00, 0xEF, 0xDC, 0x94, 0x00, 0xB2, 0x00, 0x00
    };

const unsigned char kPerfInfoDebuggerEnabledPayloadV2[] = {
    0x00 };  // This byte is dummy. The payload is an empty array.

const unsigned char kPerfInfoISRPayloadV2[] = {
    0xAC, 0x4D, 0x42, 0xA8, 0x66, 0x04, 0x00, 0x00,
    0xC0, 0x15, 0xF9, 0x02, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x81, 0x00, 0x00 };

const unsigned char kPerfInfoThreadedDPCPayload32bitsV2[] = {
    0x0A, 0x4D, 0xFD, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0x07, 0x71, 0x83, 0x82 };

const unsigned char kPerfInfoDPCPayload32bitsV2[] = {
    0x34, 0xC1, 0xB1, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0x1D, 0xEB, 0x0C, 0x90 };

const unsigned char kPerfInfoDPCPayloadV2[] = {
    0xCD, 0xEC, 0x3A, 0xA8, 0x66, 0x04, 0x00, 0x00,
    0xE4, 0xBC, 0x96, 0x74, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kPerfInfoTimerDPCPayload32bitsV2[] = {
    0xC3, 0x3B, 0xB1, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0xB0, 0x27, 0xFE, 0x93 };

const unsigned char kPerfInfoTimerDPCPayloadV2[] = {
    0x75, 0x24, 0x3C, 0xA8, 0x66, 0x04, 0x00, 0x00,
    0xD8, 0x04, 0x11, 0x03, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kPerfInfoCollectionStartPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00 };

const unsigned char kPerfInfoCollectionStartPayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00, 0x54, 0x00, 0x69, 0x00,
    0x6D, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00
    };

const unsigned char kPerfInfoCollectionEndPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00 };

const unsigned char kPerfInfoCollectionEndPayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00, 0x54, 0x00, 0x69, 0x00,
    0x6D, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00
    };

const unsigned char kPerfInfoCollectionStartSecondPayloadV3[] = {
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kPerfInfoCollectionEndSecondPayloadV3[] = {
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kProcessStartPayload32bitsV1[] = {
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x06, 0x00, 0x00,
    0xDC, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0x96, 0x2C, 0xEC, 0x2C, 0x68, 0xFD, 0x31, 0x06,
    0xF1, 0xDC, 0xA4, 0xD3, 0xE8, 0x03, 0x00, 0x00,
    0x6E, 0x6F, 0x74, 0x65, 0x70, 0x61, 0x64, 0x2E,
    0x65, 0x78, 0x65, 0x00 };

const unsigned char kProcessStartPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x06, 0x00, 0x00,
    0xDC, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0x96, 0x2C, 0xEC, 0x2C, 0x68, 0xFD, 0x31, 0x06,
    0xF1, 0xDC, 0xA4, 0xD3, 0xE8, 0x03, 0x00, 0x00,
    0x6E, 0x6F, 0x74, 0x65, 0x70, 0x61, 0x64, 0x2E,
    0x65, 0x78, 0x65, 0x00, 0x22, 0x00, 0x43, 0x00,
    0x3A, 0x00, 0x5C, 0x00, 0x57, 0x00, 0x69, 0x00,
    0x6E, 0x00, 0x64, 0x00, 0x6F, 0x00, 0x77, 0x00,
    0x73, 0x00, 0x5C, 0x00, 0x73, 0x00, 0x79, 0x00,
    0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6D, 0x00,
    0x33, 0x00, 0x32, 0x00, 0x5C, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x74, 0x00, 0x65, 0x00, 0x70, 0x00,
    0x61, 0x00, 0x64, 0x00, 0x2E, 0x00, 0x65, 0x00,
    0x78, 0x00, 0x65, 0x00, 0x22, 0x00, 0x20, 0x00,
    0x00, 0x00 };

const unsigned char kProcessStartPayload32bitsV3[] = {
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x06, 0x00, 0x00,
    0xDC, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00, 0x6E, 0x6F, 0x74, 0x65,
    0x70, 0x61, 0x64, 0x2E, 0x65, 0x78, 0x65, 0x00,
    0x22, 0x00, 0x43, 0x00, 0x3A, 0x00, 0x5C, 0x00,
    0x57, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x64, 0x00,
    0x6F, 0x00, 0x77, 0x00, 0x73, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x33, 0x00, 0x32, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x70, 0x00, 0x61, 0x00, 0x64, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
    0x22, 0x00, 0x20, 0x00, 0x00, 0x00 };

const unsigned char kProcessStartPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x06, 0x00, 0x00, 0xDC, 0x03, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00, 0x6E, 0x6F, 0x74, 0x65,
    0x70, 0x61, 0x64, 0x2E, 0x65, 0x78, 0x65, 0x00,
    0x22, 0x00, 0x43, 0x00, 0x3A, 0x00, 0x5C, 0x00,
    0x57, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x64, 0x00,
    0x6F, 0x00, 0x77, 0x00, 0x73, 0x00, 0x5C, 0x00,
    0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x6D, 0x00, 0x33, 0x00, 0x32, 0x00,
    0x5C, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x70, 0x00, 0x61, 0x00, 0x64, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x78, 0x00, 0x65, 0x00,
    0x22, 0x00, 0x20, 0x00, 0x00, 0x00 };

const unsigned char kProcessStartPayloadV3[] = {
    0x60, 0x80, 0x62, 0x0F, 0x80, 0xFA, 0xFF, 0xFF,
    0x00, 0x1A, 0x00, 0x00, 0xA0, 0x1C, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00,
    0x00, 0xF0, 0x43, 0x1D, 0x01, 0x00, 0x00, 0x00,
    0x30, 0x56, 0x53, 0x15, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xA0, 0xF8, 0xFF, 0xFF,
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x02, 0x03, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x00, 0x00, 0x78, 0x70, 0x65, 0x72,
    0x66, 0x2E, 0x65, 0x78, 0x65, 0x00, 0x78, 0x00,
    0x70, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x2D, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x65, 0x00, 0x74, 0x00, 0x6C, 0x00,
    0x00, 0x00 };

const unsigned char kProcessStartPayloadV4[] = {
    0x80, 0x40, 0xFC, 0x1A, 0x00, 0xE0, 0xFF, 0xFF,
    0x8C, 0x0A, 0x00, 0x00, 0x08, 0x17, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00,
    0x00, 0xB0, 0xA2, 0xA3, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x90, 0xF0, 0x57, 0x04,
    0x00, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x06, 0xE9, 0x03, 0x00, 0x00,
    0x78, 0x70, 0x65, 0x72, 0x66, 0x2E, 0x65, 0x78,
    0x65, 0x00, 0x78, 0x00, 0x70, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x66, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x2D, 0x00, 0x73, 0x00, 0x74, 0x00, 0x6F, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kProcessEndPayload32bitsV1[] = {
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x06, 0x00, 0x00,
    0xDC, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0x96, 0x2C, 0xEC, 0x2C, 0x68, 0xFD, 0x31, 0x06,
    0xF1, 0xDC, 0xA4, 0xD3, 0xE8, 0x03, 0x00, 0x00,
    0x6E, 0x6F, 0x74, 0x65, 0x70, 0x61, 0x64, 0x2E,
    0x65, 0x78, 0x65, 0x00 };

const unsigned char kProcessEndPayloadV3[] = {
    0x60, 0x80, 0x62, 0x0F, 0x80, 0xFA, 0xFF, 0xFF,
    0x2C, 0x20, 0x00, 0x00, 0xA0, 0x1C, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xA0, 0x3F, 0xA4, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0xB1, 0x2B, 0x11, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xF8, 0xFF, 0xFF,
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
    0x0D, 0x03, 0x00, 0x00, 0x78, 0x70, 0x65, 0x72,
    0x66, 0x2E, 0x65, 0x78, 0x65, 0x00, 0x78, 0x00,
    0x70, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x2D, 0x00, 0x6F, 0x00,
    0x6E, 0x00, 0x20, 0x00, 0x50, 0x00, 0x52, 0x00,
    0x4F, 0x00, 0x43, 0x00, 0x5F, 0x00, 0x54, 0x00,
    0x48, 0x00, 0x52, 0x00, 0x45, 0x00, 0x41, 0x00,
    0x44, 0x00, 0x2B, 0x00, 0x4C, 0x00, 0x4F, 0x00,
    0x41, 0x00, 0x44, 0x00, 0x45, 0x00, 0x52, 0x00,
    0x2B, 0x00, 0x43, 0x00, 0x53, 0x00, 0x57, 0x00,
    0x49, 0x00, 0x54, 0x00, 0x43, 0x00, 0x48, 0x00,
    0x20, 0x00, 0x2D, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x6B, 0x00, 0x77, 0x00,
    0x61, 0x00, 0x6C, 0x00, 0x6B, 0x00, 0x20, 0x00,
    0x49, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x67, 0x00,
    0x65, 0x00, 0x4C, 0x00, 0x6F, 0x00, 0x61, 0x00,
    0x64, 0x00, 0x2B, 0x00, 0x49, 0x00, 0x6D, 0x00,
    0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x55, 0x00,
    0x6E, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x61, 0x00,
    0x64, 0x00, 0x00, 0x00 };

const unsigned char kProcessEndPayloadV4[] = {
    0x80, 0x40, 0xFC, 0x1A, 0x00, 0xE0, 0xFF, 0xFF,
    0xF8, 0x07, 0x00, 0x00, 0x08, 0x17, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0xC0, 0xBD, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xA0, 0xC8, 0xFC, 0x15,
    0x00, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0x12, 0x13, 0x0F, 0x12, 0x13, 0x42, 0x24, 0x33,
    0xCC, 0xCA, 0xCC, 0xCB, 0xBA, 0xBE, 0x00, 0x00,
    0x78, 0x70, 0x65, 0x72, 0x66, 0x2E, 0x65, 0x78,
    0x65, 0x00, 0x78, 0x00, 0x70, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x66, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x2D, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x20, 0x00,
    0x50, 0x00, 0x52, 0x00, 0x4F, 0x00, 0x43, 0x00,
    0x5F, 0x00, 0x54, 0x00, 0x48, 0x00, 0x52, 0x00,
    0x45, 0x00, 0x41, 0x00, 0x44, 0x00, 0x2B, 0x00,
    0x4C, 0x00, 0x4F, 0x00, 0x41, 0x00, 0x44, 0x00,
    0x45, 0x00, 0x52, 0x00, 0x2B, 0x00, 0x50, 0x00,
    0x52, 0x00, 0x4F, 0x00, 0x46, 0x00, 0x49, 0x00,
    0x4C, 0x00, 0x45, 0x00, 0x2B, 0x00, 0x43, 0x00,
    0x53, 0x00, 0x57, 0x00, 0x49, 0x00, 0x54, 0x00,
    0x43, 0x00, 0x48, 0x00, 0x2B, 0x00, 0x44, 0x00,
    0x49, 0x00, 0x53, 0x00, 0x50, 0x00, 0x41, 0x00,
    0x54, 0x00, 0x43, 0x00, 0x48, 0x00, 0x45, 0x00,
    0x52, 0x00, 0x2B, 0x00, 0x44, 0x00, 0x50, 0x00,
    0x43, 0x00, 0x2B, 0x00, 0x49, 0x00, 0x4E, 0x00,
    0x54, 0x00, 0x45, 0x00, 0x52, 0x00, 0x52, 0x00,
    0x55, 0x00, 0x50, 0x00, 0x54, 0x00, 0x2B, 0x00,
    0x53, 0x00, 0x59, 0x00, 0x53, 0x00, 0x43, 0x00,
    0x41, 0x00, 0x4C, 0x00, 0x4C, 0x00, 0x2B, 0x00,
    0x50, 0x00, 0x52, 0x00, 0x49, 0x00, 0x4F, 0x00,
    0x52, 0x00, 0x49, 0x00, 0x54, 0x00, 0x59, 0x00,
    0x2B, 0x00, 0x53, 0x00, 0x50, 0x00, 0x49, 0x00,
    0x4E, 0x00, 0x4C, 0x00, 0x4F, 0x00, 0x43, 0x00,
    0x4B, 0x00, 0x2B, 0x00, 0x50, 0x00, 0x45, 0x00,
    0x52, 0x00, 0x46, 0x00, 0x5F, 0x00, 0x43, 0x00,
    0x4F, 0x00, 0x55, 0x00, 0x4E, 0x00, 0x54, 0x00,
    0x45, 0x00, 0x52, 0x00, 0x2B, 0x00, 0x44, 0x00,
    0x49, 0x00, 0x53, 0x00, 0x4B, 0x00, 0x5F, 0x00,
    0x49, 0x00, 0x4F, 0x00, 0x2B, 0x00, 0x44, 0x00,
    0x49, 0x00, 0x53, 0x00, 0x4B, 0x00, 0x5F, 0x00,
    0x49, 0x00, 0x4F, 0x00, 0x5F, 0x00, 0x49, 0x00,
    0x4E, 0x00, 0x49, 0x00, 0x54, 0x00, 0x2B, 0x00,
    0x46, 0x00, 0x49, 0x00, 0x4C, 0x00, 0x45, 0x00,
    0x5F, 0x00, 0x49, 0x00, 0x4F, 0x00, 0x2B, 0x00,
    0x46, 0x00, 0x49, 0x00, 0x4C, 0x00, 0x45, 0x00,
    0x5F, 0x00, 0x49, 0x00, 0x4F, 0x00, 0x5F, 0x00,
    0x49, 0x00, 0x4E, 0x00, 0x49, 0x00, 0x54, 0x00,
    0x2B, 0x00, 0x48, 0x00, 0x41, 0x00, 0x52, 0x00,
    0x44, 0x00, 0x5F, 0x00, 0x46, 0x00, 0x41, 0x00,
    0x55, 0x00, 0x4C, 0x00, 0x54, 0x00, 0x53, 0x00,
    0x2B, 0x00, 0x46, 0x00, 0x49, 0x00, 0x4C, 0x00,
    0x45, 0x00, 0x4E, 0x00, 0x41, 0x00, 0x4D, 0x00,
    0x45, 0x00, 0x2B, 0x00, 0x52, 0x00, 0x45, 0x00,
    0x47, 0x00, 0x49, 0x00, 0x53, 0x00, 0x54, 0x00,
    0x52, 0x00, 0x59, 0x00, 0x2B, 0x00, 0x44, 0x00,
    0x52, 0x00, 0x49, 0x00, 0x56, 0x00, 0x45, 0x00,
    0x52, 0x00, 0x53, 0x00, 0x2B, 0x00, 0x50, 0x00,
    0x4F, 0x00, 0x57, 0x00, 0x45, 0x00, 0x52, 0x00,
    0x2B, 0x00, 0x43, 0x00, 0x43, 0x00, 0x2B, 0x00,
    0x4E, 0x00, 0x45, 0x00, 0x54, 0x00, 0x57, 0x00,
    0x4F, 0x00, 0x52, 0x00, 0x4B, 0x00, 0x54, 0x00,
    0x52, 0x00, 0x41, 0x00, 0x43, 0x00, 0x45, 0x00,
    0x2B, 0x00, 0x56, 0x00, 0x49, 0x00, 0x52, 0x00,
    0x54, 0x00, 0x5F, 0x00, 0x41, 0x00, 0x4C, 0x00,
    0x4C, 0x00, 0x4F, 0x00, 0x43, 0x00, 0x2B, 0x00,
    0x4D, 0x00, 0x45, 0x00, 0x4D, 0x00, 0x49, 0x00,
    0x4E, 0x00, 0x46, 0x00, 0x4F, 0x00, 0x2B, 0x00,
    0x4D, 0x00, 0x45, 0x00, 0x4D, 0x00, 0x4F, 0x00,
    0x52, 0x00, 0x59, 0x00, 0x2B, 0x00, 0x54, 0x00,
    0x49, 0x00, 0x4D, 0x00, 0x45, 0x00, 0x52, 0x00,
    0x20, 0x00, 0x2D, 0x00, 0x66, 0x00, 0x20, 0x00,
    0x43, 0x00, 0x3A, 0x00, 0x5C, 0x00, 0x6B, 0x00,
    0x65, 0x00, 0x72, 0x00, 0x6E, 0x00, 0x65, 0x00,
    0x6C, 0x00, 0x2E, 0x00, 0x65, 0x00, 0x74, 0x00,
    0x6C, 0x00, 0x20, 0x00, 0x2D, 0x00, 0x42, 0x00,
    0x75, 0x00, 0x66, 0x00, 0x66, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x53, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x34, 0x00, 0x30, 0x00,
    0x39, 0x00, 0x36, 0x00, 0x20, 0x00, 0x2D, 0x00,
    0x4D, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x42, 0x00,
    0x75, 0x00, 0x66, 0x00, 0x66, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x20, 0x00, 0x32, 0x00,
    0x35, 0x00, 0x36, 0x00, 0x20, 0x00, 0x2D, 0x00,
    0x4D, 0x00, 0x61, 0x00, 0x78, 0x00, 0x42, 0x00,
    0x75, 0x00, 0x66, 0x00, 0x66, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x20, 0x00, 0x32, 0x00,
    0x35, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00 };

const unsigned char kProcessDCStartPayloadV3[] = {
    0x80, 0x81, 0x01, 0x03, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x56, 0x62, 0x2A, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0xFF, 0xFF,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x10, 0x00, 0x00, 0x00, 0x49, 0x64, 0x6C, 0x65,
    0x00, 0x00, 0x00 };

const unsigned char kProcessDCStartPayloadV4[] = {
    0xC0, 0x53, 0xBB, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC0, 0xBB, 0xE7, 0x2D,
    0x00, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x10, 0x00, 0x00, 0x00,
    0x49, 0x64, 0x6C, 0x65, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00 };

const unsigned char kProcessDCEndPayloadV3[] = {
    0x80, 0x81, 0x01, 0x03, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0xCD, 0x7E, 0x05, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00, 0x49, 0x64, 0x6C, 0x65,
    0x00, 0x00, 0x00 };

const unsigned char kProcessDCEndPayloadV4[] = {
    0xC0, 0x53, 0xBB, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x85, 0x86, 0x16,
    0x00, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x74, 0x00, 0x61, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x10, 0x00, 0x00, 0x00,
    0x49, 0x64, 0x6C, 0x65, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00 };

const unsigned char kProcessTerminatePayloadV2[] = {
    0xF8, 0x07, 0x00, 0x00 };

const unsigned char kProcessPerfCtrPayload32bitsV2[] = {
    0xC4, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x63, 0x02, 0x00, 0xC0, 0x53, 0x00,
    0x00, 0x90, 0x22, 0x00, 0x9C, 0x20, 0x01, 0x00,
    0xCC, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kProcessPerfCtrPayloadV2[] = {
    0xF8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x30, 0xAD, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xC0, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xE0, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kProcessPerfCtrRundownPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x63, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kProcessDefunctPayloadV2[] = {
    0x00, 0xA8, 0x0B, 0x10, 0x80, 0xFA, 0xFF, 0xFF,
    0x28, 0x07, 0x00, 0x00, 0xCC, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0x50, 0x87, 0x22, 0x80, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xFA, 0xFF, 0xFF,
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x3E, 0x66, 0xA1, 0xD8,
    0xD6, 0x0A, 0x05, 0xD1, 0x4F, 0x2E, 0xC7, 0x3C,
    0xEC, 0x03, 0x00, 0x00, 0x63, 0x79, 0x67, 0x72,
    0x75, 0x6E, 0x73, 0x72, 0x76, 0x2E, 0x65, 0x78,
    0x65, 0x00, 0x00, 0x00 };

const unsigned char kProcessDefunctPayloadV3[] = {
    0x60, 0xE0, 0xA6, 0x13, 0x80, 0xFA, 0xFF, 0xFF,
    0x64, 0x0E, 0x00, 0x00, 0x94, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0xEF, 0x97, 0x01, 0x00, 0x00, 0x00,
    0xE0, 0x87, 0x8B, 0x04, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x10, 0x00, 0x00, 0x00, 0x63, 0x6D, 0x64, 0x2E,
    0x65, 0x78, 0x65, 0x00, 0x00, 0x00 };

const unsigned char kProcessDefunctPayloadV5[] = {
    0xC0, 0xC5, 0xF2, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x48, 0x19, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0xCB, 0x4F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xE5, 0x3B, 0x03,
    0x00, 0xC0, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0C, 0x00, 0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x03, 0x00, 0x00,
    0x63, 0x68, 0x72, 0x6F, 0x6D, 0x65, 0x2E, 0x65,
    0x78, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x8D, 0x49, 0xA2, 0xF9, 0xEC, 0xFA, 0xCE,
    0x01 };

const unsigned char kThreadStartPayload32bitsV1[] = {
    0x04, 0x00, 0x00, 0x00, 0x4C, 0x07, 0x00, 0x00,
    0x00, 0x60, 0xB7, 0xF3, 0x00, 0x30, 0xB7, 0xF3,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x85, 0xDB, 0x1E, 0xF7, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00 };

const unsigned char kThreadStartPayload32bitsV3[] = {
    0x2C, 0x02, 0x00, 0x00, 0x2C, 0x13, 0x00, 0x00,
    0x00, 0x50, 0x98, 0xB1, 0x00, 0x20, 0x98, 0xB1,
    0x00, 0x00, 0xD5, 0x00, 0x00, 0xC0, 0xD4, 0x00,
    0x03, 0x00, 0x00, 0x00, 0xE9, 0x03, 0xAB, 0x77,
    0x00, 0xE0, 0xFD, 0x7F, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x05, 0x02, 0x00 };

const unsigned char kThreadStartPayloadV3[] = {
    0x78, 0x21, 0x00, 0x00, 0x94, 0x14, 0x00, 0x00,
    0x00, 0x30, 0x0E, 0x27, 0x00, 0xD0, 0xFF, 0xFF,
    0x00, 0xD0, 0x0D, 0x27, 0x00, 0xD0, 0xFF, 0xFF,
    0x30, 0xFD, 0x0B, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x0B, 0x06, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2C, 0xFD, 0x58, 0x5C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xC0, 0x12, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x05, 0x02, 0x00
    };

const unsigned char kThreadEndPayload32bitsV1[] = {
    0x04, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00
    };

const unsigned char kThreadEndPayload32bitsV3[] = {
    0xC4, 0x12, 0x00, 0x00, 0x64, 0x13, 0x00, 0x00,
    0x00, 0x50, 0x55, 0xAA, 0x00, 0x20, 0x55, 0xAA,
    0x00, 0x00, 0x9C, 0x00, 0x00, 0xE0, 0x9B, 0x00,
    0x03, 0x00, 0x00, 0x00, 0xE9, 0x03, 0xAB, 0x77,
    0x00, 0xD0, 0xFD, 0x7F, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x05, 0x02, 0x00 };

const unsigned char kThreadEndPayloadV3[] = {
    0xF8, 0x07, 0x00, 0x00, 0xD8, 0x0C, 0x00, 0x00,
    0x00, 0x70, 0x8C, 0x29, 0x00, 0xD0, 0xFF, 0xFF,
    0x00, 0x10, 0x8C, 0x29, 0x00, 0xD0, 0xFF, 0xFF,
    0x00, 0x00, 0x1C, 0x42, 0xD2, 0x00, 0x00, 0x00,
    0x00, 0xE0, 0x1B, 0x42, 0xD2, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x85, 0x72, 0xAE, 0xFC, 0x7F, 0x00, 0x00,
    0x00, 0x80, 0xB3, 0x39, 0xF7, 0x7F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x05, 0x02, 0x00
    };

const unsigned char kThreadDCStartPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0xF5, 0x02, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0xF5, 0x02, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x25, 0xC7, 0x01, 0x00, 0xF8, 0xFF, 0xFF,
    0x80, 0x25, 0xC7, 0x01, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kThreadDCStartPayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x48, 0x76, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x10, 0x48, 0x76, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x90, 0x07, 0x9C, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00
    };

const unsigned char kThreadDCEndPayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x48, 0x76, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x10, 0x48, 0x76, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x90, 0x07, 0x9C, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00
    };

const unsigned char kThreadCSwitchPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x2C, 0x11, 0x00, 0x00,
    0x00, 0x09, 0x00, 0x00, 0x17, 0x00, 0x01, 0x00,
    0x12, 0x00, 0x00, 0x00, 0x26, 0x48, 0x00, 0x00
    };

const unsigned char kThreadCSwitchPayloadV2[] = {
    0xCC, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x04,
    0x01, 0x00, 0x00, 0x00, 0x87, 0x6D, 0x88, 0x34
    };

const unsigned char kThreadSpinLockPayloadV2[] = {
    0x60, 0x01, 0xB2, 0x02, 0x00, 0xE0, 0xFF, 0xFF,
    0x10, 0x04, 0x9E, 0x74, 0x00, 0xF8, 0xFF, 0xFF,
    0x9E, 0x8B, 0x93, 0x3C, 0xAC, 0x79, 0x07, 0x00,
    0x27, 0x8E, 0x93, 0x3C, 0xAC, 0x79, 0x07, 0x00,
    0x91, 0x06, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kThreadSetPriorityPayloadV3[] = {
    0x20, 0x02, 0x00, 0x00, 0x0F, 0x10, 0x00, 0x00
    };

const unsigned char kThreadSetBasePriorityPayloadV3[] = {
    0xF0, 0x1A, 0x00, 0x00, 0x04, 0x07, 0x07, 0x00
    };

const unsigned char kThreadReadyThreadPayloadV2[] = {
    0xCC, 0x08, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00
    };

const unsigned char kThreadSetPagePriorityPayloadV3[] = {
    0x6C, 0x1A, 0x00, 0x00, 0x05, 0x06, 0x00, 0x00
    };

const unsigned char kThreadSetIoPriorityPayloadV3[] = {
    0xBC, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00
    };

const unsigned char kThreadAutoBoostSetFloorPayloadV2[] = {
    0x78, 0x51, 0x15, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x1A, 0x00, 0x00, 0x0B, 0x07, 0x20, 0x00
    };

const unsigned char kThreadAutoBoostClearFloorPayloadV2[] = {
    0x78, 0x51, 0x15, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x1A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00
    };

const unsigned char kThreadAutoBoostEntryExhaustionPayloadV2[] = {
    0xF0, 0x34, 0xA4, 0x08, 0x00, 0xE0, 0xFF, 0xFF,
    0xBC, 0x0B, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kTcplpSendIPV4Payload32bitsV2[] = {
    0xB8, 0x0E, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00,
    0x40, 0x04, 0x0B, 0x19, 0xAC, 0x1D, 0x0C, 0x7B,
    0x00, 0x50, 0xFD, 0x59, 0xC1, 0x9C, 0xBF, 0x00,
    0xC1, 0x9C, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kTcplpSendIPV4PayloadV2[] = {
    0x34, 0x21, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0xAB, 0x26, 0x35, 0x00,
    0xAB, 0x26, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kTcplpTCPCopyIPV4PayloadV2[] = {
    0x80, 0x1A, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kTcplpRecvIPV4Payload32bitsV2[] = {
    0xB8, 0x0E, 0x00, 0x00, 0xC2, 0x01, 0x00, 0x00,
    0x40, 0x04, 0x0B, 0x19, 0xAC, 0x1D, 0x0C, 0x7B,
    0x00, 0x50, 0xFD, 0x59, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kTcplpRecvIPV4PayloadV2[] = {
    0x80, 0x1A, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kTcplpConnectIPV4Payload32bitsV2[] = {
    0xB8, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x83, 0xFD, 0x0D, 0x15, 0xAC, 0x1D, 0x0C, 0x7B,
    0x00, 0x50, 0xFD, 0x5A, 0xA0, 0x05, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0xC0, 0x02, 0x01, 0x00,
    0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kTcplpConnectIPV4PayloadV2[] = {
    0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0x96, 0x05, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0xF4, 0x00, 0x01, 0x00,
    0x08, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kTcplpDisconnectIPV4PayloadV2[] = {
    0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kTcplpRetransmitIPV4PayloadV2[] = {
    0x80, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryCountersPayload32bitsV2[] = {
    0x74, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x16, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x57, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0B, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x74, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xE4, 0x1C, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x94, 0xF8, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA2, 0xCF, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryCountersPayloadV2[] = {
    0xA6, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFB, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x77, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x65, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA6, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF8, 0xEF, 0xA1, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x2C, 0x7D, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x77, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryClosePayloadV2[] = {
    0x56, 0x80, 0x46, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0xCC, 0x0B, 0x01, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistryOpenPayload32bitsV2[] = {
    0xF4, 0x24, 0xB2, 0x91, 0xAB, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x52, 0x00,
    0x65, 0x00, 0x67, 0x00, 0x69, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x79, 0x00, 0x5C, 0x00,
    0x4D, 0x00, 0x61, 0x00, 0x63, 0x00, 0x68, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x5C, 0x00,
    0x53, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x74, 0x00,
    0x77, 0x00, 0x61, 0x00, 0x72, 0x00, 0x65, 0x00,
    0x5C, 0x00, 0x4D, 0x00, 0x69, 0x00, 0x63, 0x00,
    0x72, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x6F, 0x00,
    0x66, 0x00, 0x74, 0x00, 0x5C, 0x00, 0x57, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x64, 0x00, 0x6F, 0x00,
    0x77, 0x00, 0x73, 0x00, 0x20, 0x00, 0x4E, 0x00,
    0x54, 0x00, 0x5C, 0x00, 0x43, 0x00, 0x75, 0x00,
    0x72, 0x00, 0x72, 0x00, 0x65, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x56, 0x00, 0x65, 0x00, 0x72, 0x00,
    0x73, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x5C, 0x00, 0x47, 0x00, 0x52, 0x00, 0x45, 0x00,
    0x5F, 0x00, 0x49, 0x00, 0x6E, 0x00, 0x69, 0x00,
    0x74, 0x00, 0x69, 0x00, 0x61, 0x00, 0x6C, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryOpenPayloadV2[] = {
    0x21, 0x90, 0x46, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x00, 0x00 };

const unsigned char kRegistryQueryValuePayloadV2[] = {
    0x58, 0x90, 0x46, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x34, 0x00, 0x00, 0xC0, 0x02, 0x00, 0x00, 0x00,
    0x58, 0xE2, 0x18, 0x08, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x00, 0x00 };

const unsigned char kRegistryQueryPayloadV2[] = {
    0x30, 0x7E, 0x4F, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x58, 0x22, 0x50, 0x01, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistryKCBDeletePayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF8, 0xD6, 0xE5, 0x11, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryKCBCreatePayload32bitsV1[] = {
    0x00, 0x00, 0x00, 0x00, 0x98, 0xC6, 0x5F, 0xE3,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x52, 0x00,
    0x45, 0x00, 0x47, 0x00, 0x49, 0x00, 0x53, 0x00,
    0x54, 0x00, 0x52, 0x00, 0x59, 0x00, 0x5C, 0x00,
    0x4D, 0x00, 0x41, 0x00, 0x43, 0x00, 0x48, 0x00,
    0x49, 0x00, 0x4E, 0x00, 0x45, 0x00, 0x5C, 0x00,
    0x53, 0x00, 0x59, 0x00, 0x53, 0x00, 0x54, 0x00,
    0x45, 0x00, 0x4D, 0x00, 0x5C, 0x00, 0x43, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x6F, 0x00, 0x6C, 0x00, 0x53, 0x00, 0x65, 0x00,
    0x74, 0x00, 0x30, 0x00, 0x30, 0x00, 0x31, 0x00,
    0x5C, 0x00, 0x45, 0x00, 0x6E, 0x00, 0x75, 0x00,
    0x6D, 0x00, 0x5C, 0x00, 0x50, 0x00, 0x43, 0x00,
    0x49, 0x00, 0x5C, 0x00, 0x56, 0x00, 0x45, 0x00,
    0x4E, 0x00, 0x5F, 0x00, 0x38, 0x00, 0x30, 0x00,
    0x38, 0x00, 0x36, 0x00, 0x26, 0x00, 0x44, 0x00,
    0x45, 0x00, 0x56, 0x00, 0x5F, 0x00, 0x32, 0x00,
    0x43, 0x00, 0x32, 0x00, 0x32, 0x00, 0x26, 0x00,
    0x53, 0x00, 0x55, 0x00, 0x42, 0x00, 0x53, 0x00,
    0x59, 0x00, 0x53, 0x00, 0x5F, 0x00, 0x30, 0x00,
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00,
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x26, 0x00,
    0x52, 0x00, 0x45, 0x00, 0x56, 0x00, 0x5F, 0x00,
    0x30, 0x00, 0x35, 0x00, 0x5C, 0x00, 0x33, 0x00,
    0x26, 0x00, 0x33, 0x00, 0x36, 0x00, 0x63, 0x00,
    0x62, 0x00, 0x39, 0x00, 0x37, 0x00, 0x61, 0x00,
    0x33, 0x00, 0x26, 0x00, 0x30, 0x00, 0x26, 0x00,
    0x32, 0x00, 0x32, 0x00, 0x00, 0x00 };

const unsigned char kRegistryKCBCreatePayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA8, 0x84, 0x56, 0x08, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x00, 0x00 };

const unsigned char kRegistrySetInformationPayloadV2[] = {
    0x15, 0x60, 0x5A, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA8, 0x84, 0x56, 0x08, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistryEnumerateValueKeyPayloadV2[] = {
    0x97, 0x60, 0x5A, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA8, 0x84, 0x56, 0x08, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistryEnumerateKeyPayloadV2[] = {
    0x29, 0x64, 0x5A, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xA8, 0x84, 0x56, 0x08, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistrySetValuePayload32bitsV2[] = {
    0x91, 0x97, 0x4A, 0x92, 0xAB, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x70, 0x5E, 0x99, 0x7B, 0x00, 0x51, 0x00,
    0x36, 0x00, 0x35, 0x00, 0x32, 0x00, 0x33, 0x00,
    0x31, 0x00, 0x4F, 0x00, 0x30, 0x00, 0x2D, 0x00,
    0x4F, 0x00, 0x32, 0x00, 0x53, 0x00, 0x31, 0x00,
    0x2D, 0x00, 0x34, 0x00, 0x38, 0x00, 0x35, 0x00,
    0x37, 0x00, 0x2D, 0x00, 0x4E, 0x00, 0x34, 0x00,
    0x50, 0x00, 0x52, 0x00, 0x2D, 0x00, 0x4E, 0x00,
    0x38, 0x00, 0x52, 0x00, 0x37, 0x00, 0x50, 0x00,
    0x36, 0x00, 0x52, 0x00, 0x4E, 0x00, 0x37, 0x00,
    0x51, 0x00, 0x32, 0x00, 0x37, 0x00, 0x7D, 0x00,
    0x5C, 0x00, 0x70, 0x00, 0x7A, 0x00, 0x71, 0x00,
    0x2E, 0x00, 0x72, 0x00, 0x6B, 0x00, 0x72, 0x00,
    0x00, 0x00 };

const unsigned char kRegistrySetValuePayloadV2[] = {
    0x4A, 0xAE, 0x94, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x18, 0x16, 0x09, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x00, 0x00 };

const unsigned char kRegistryCreatePayload32bitsV2[] = {
    0xAC, 0xCE, 0xFE, 0x92, 0xAB, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0xA4, 0x5B, 0x8C, 0x53, 0x00, 0x6F, 0x00,
    0x66, 0x00, 0x74, 0x00, 0x77, 0x00, 0x61, 0x00,
    0x72, 0x00, 0x65, 0x00, 0x5C, 0x00, 0x4D, 0x00,
    0x69, 0x00, 0x63, 0x00, 0x72, 0x00, 0x6F, 0x00,
    0x73, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x74, 0x00,
    0x5C, 0x00, 0x49, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x72, 0x00, 0x6E, 0x00, 0x65, 0x00,
    0x74, 0x00, 0x20, 0x00, 0x45, 0x00, 0x78, 0x00,
    0x70, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x72, 0x00,
    0x65, 0x00, 0x72, 0x00, 0x5C, 0x00, 0x53, 0x00,
    0x51, 0x00, 0x4D, 0x00, 0x00, 0x00 };

const unsigned char kRegistryCreatePayloadV2[] = {
    0x4E, 0x1C, 0x99, 0x49, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x0C, 0x85, 0x03, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x00, 0x00
    };

const unsigned char kRegistryQuerySecurityPayloadV2[] = {
    0x27, 0xAF, 0x41, 0x4B, 0x0D, 0x01, 0x00, 0x00,
    0x23, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0xF8, 0xC6, 0xE1, 0x11, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistrySetSecurityPayloadV2[] = {
    0xED, 0xAF, 0x41, 0x4B, 0x0D, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x18, 0xE6, 0x11, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00 };

const unsigned char kRegistryKCBRundownEndPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x60, 0x02, 0x00, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x00, 0x00 };

const unsigned char kRegistryConfigPayloadV2[] = {
    0x01, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFileCreatePayload32bitsV2[] = {
    0xF8, 0xF0, 0x91, 0xAE, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x46, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x73, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x76, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x75, 0x00, 0x65, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x46, 0x00, 0x61, 0x00, 0x6B, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x63, 0x00, 0x68, 0x00,
    0x61, 0x00, 0x72, 0x00, 0x61, 0x00, 0x63, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFileCreatePayloadV2[] = {
    0x30, 0x0C, 0x57, 0x05, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFileDeletePayload32bitsV2[] = {
    0xF8, 0x90, 0x8B, 0xB1, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x46, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x73, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x76, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x75, 0x00, 0x65, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x46, 0x00, 0x61, 0x00, 0x6B, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x63, 0x00, 0x68, 0x00,
    0x61, 0x00, 0x72, 0x00, 0x61, 0x00, 0x63, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOFileDeletePayloadV2[] = {
    0x30, 0x2C, 0xF3, 0x15, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFileRundownPayload32bitsV2[] = {
    0x98, 0x66, 0xB8, 0x89, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFileRundownPayloadV2[] = {
    0xC0, 0x75, 0xF6, 0x00, 0x00, 0xC0, 0xFF, 0xFF,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x00, 0x00 };

const unsigned char kFileIOCreatePayload32bitsV2[] = {
    0x40, 0xCE, 0xE3, 0x84, 0x34, 0x0A, 0x00, 0x00,
    0x98, 0x41, 0xD9, 0x84, 0x00, 0x00, 0x20, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOCreatePayloadV2[] = {
    0x60, 0xEC, 0x64, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x38, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0xE4, 0x17, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0x60, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x46, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x73, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x76, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x75, 0x00, 0x65, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x46, 0x00, 0x61, 0x00, 0x6B, 0x00,
    0x65, 0x00, 0x20, 0x00, 0x63, 0x00, 0x68, 0x00,
    0x61, 0x00, 0x72, 0x00, 0x61, 0x00, 0x63, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00,
    0x00, 0x00 };

const unsigned char kFileIOCreatePayloadV3[] = {
    0x98, 0x19, 0x7E, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x1F, 0xFB, 0x04, 0x00, 0xE0, 0xFF, 0xFF,
    0xC0, 0x19, 0x00, 0x00, 0x60, 0x00, 0x02, 0x01,
    0x80, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x00, 0x00 };

const unsigned char kFileIOCleanupPayloadV2[] = {
    0x60, 0x0E, 0x91, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x1C, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x09, 0x12, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0xA0, 0x28, 0x5F, 0x01, 0xA0, 0xF8, 0xFF, 0xFF
    };

const unsigned char kFileIOCleanupPayload32bitsV2[] = {
    0x40, 0xCE, 0xE3, 0x84, 0x34, 0x0A, 0x00, 0x00,
    0x98, 0x41, 0xD9, 0x84, 0x20, 0x25, 0x8E, 0xB1
    };

const unsigned char kFileIOCleanupPayloadV3[] = {
    0x38, 0x16, 0x33, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x10, 0xEC, 0xCB, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x43, 0x08, 0x02, 0x00, 0xC0, 0xFF, 0xFF,
    0x98, 0x0D, 0x00, 0x00 };

const unsigned char kFileIOClosePayloadV2[] = {
    0x60, 0x0E, 0x91, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x1C, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x09, 0x12, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0xA0, 0x28, 0x5F, 0x01, 0xA0, 0xF8, 0xFF, 0xFF
    };

const unsigned char kFileIOClosePayload32bitsV2[] = {
    0x40, 0xCE, 0xE3, 0x84, 0x34, 0x0A, 0x00, 0x00,
    0x98, 0x41, 0xD9, 0x84, 0x20, 0x25, 0x8E, 0xB1
    };

const unsigned char kFileIOClosePayloadV3[] = {
    0x38, 0x16, 0x33, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x10, 0xEC, 0xCB, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x43, 0x08, 0x02, 0x00, 0xC0, 0xFF, 0xFF,
    0x98, 0x0D, 0x00, 0x00 };

const unsigned char kFileIOReadPayloadV2[] = {
    0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0x28, 0x15, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0xFC, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x09, 0x12, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0xA1, 0x31, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0xFF, 0x1F, 0x00, 0x00, 0x00, 0x09, 0x06, 0x00
    };

const unsigned char kFileIOReadPayload32bitsV2[] = {
    0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x29, 0xD2, 0x84, 0x6C, 0x0B, 0x00, 0x00,
    0xF0, 0xA8, 0xDD, 0x84, 0xA0, 0xA5, 0x1B, 0xA2,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOReadPayloadV3[] = {
    0xE0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x98, 0x19, 0x7E, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x1F, 0xFB, 0x04, 0x00, 0xE0, 0xFF, 0xFF,
    0x30, 0xDC, 0x6E, 0x18, 0x00, 0xC0, 0xFF, 0xFF,
    0xC0, 0x19, 0x00, 0x00, 0xFF, 0x1F, 0x00, 0x00,
    0x00, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOWritePayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x0E, 0x91, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x38, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0xE4, 0x17, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0xF1, 0xAE, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0x42, 0x0D, 0x05, 0x00, 0x00, 0x0A, 0x06, 0x00
    };

const unsigned char kFileIOWritePayload32bitsV2[] = {
    0xA4, 0x72, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0xBA, 0xEF, 0x84, 0x6C, 0x0B, 0x00, 0x00,
    0xD8, 0xE0, 0xDA, 0x84, 0x30, 0xC4, 0x9A, 0x9F,
    0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOWritePayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0x23, 0xD0, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0xC0, 0xF9, 0x3F, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x41, 0xA7, 0x1B, 0x00, 0xC0, 0xFF, 0xFF,
    0x0C, 0x07, 0x00, 0x00, 0xD2, 0x02, 0x00, 0x00,
    0x00, 0x0A, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOSetInfoPayloadV2[] = {
    0x60, 0x0E, 0x91, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0xD0, 0x9C, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x70, 0x96, 0x13, 0x00, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00 };

const unsigned char kFileIOSetInfoPayload32bitsV2[] = {
    0x38, 0x15, 0xE0, 0x84, 0xCC, 0x02, 0x00, 0x00,
    0x78, 0x4D, 0xD4, 0x85, 0x78, 0xDD, 0xBF, 0x8A,
    0x00, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOSetInfoPayloadV3[] = {
    0xB8, 0xEB, 0xD4, 0x00, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x53, 0x5F, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x41, 0xA7, 0x1B, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xAC, 0x06, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00
    };

const unsigned char kFileIODeletePayloadV2[] = {
    0x90, 0x24, 0x99, 0x03, 0x80, 0xFA, 0xFF, 0xFF,
    0xDC, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x36, 0x19, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0x35, 0x35, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0D, 0x00, 0x00, 0x00 };

const unsigned char kFileIODeletePayload32bitsV2[] = {
    0x38, 0x15, 0xE0, 0x84, 0x6C, 0x0B, 0x00, 0x00,
    0x10, 0x47, 0xD8, 0x85, 0xF8, 0x90, 0x8B, 0xB1,
    0x01, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00
    };

const unsigned char kFileIODeletePayloadV3[] = {
    0xB8, 0x3B, 0xE9, 0x00, 0x00, 0xE0, 0xFF, 0xFF,
    0x80, 0xB8, 0x04, 0x0A, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x41, 0xA7, 0x1B, 0x00, 0xC0, 0xFF, 0xFF,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x07, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00
    };

const unsigned char kFileIORenamePayloadV2[] = {
    0x60, 0xEC, 0x64, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x94, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x70, 0xEE, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x70, 0xCC, 0xEB, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x00, 0x00, 0x00 };

const unsigned char kFileIORenamePayload32bitsV2[] = {
    0x10, 0xBA, 0xEF, 0x84, 0x14, 0x0C, 0x00, 0x00,
    0x38, 0xE9, 0x7C, 0x87, 0x20, 0x35, 0x00, 0x9C,
    0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00
    };

const unsigned char kFileIORenamePayloadV3[] = {
    0x98, 0x19, 0x7E, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0x70, 0x90, 0x44, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0xA0, 0xE4, 0x81, 0x13, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0x1E, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00
    };

const unsigned char kFileIODirEnumPayloadV2[] = {
    0xC0, 0xB0, 0x06, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xD0, 0x39, 0x20, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0xF1, 0x1C, 0x00, 0xA0, 0xF8, 0xFF, 0xFF,
    0x78, 0x02, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x00, 0x00
    };

const unsigned char kFileIODirEnumPayload32bitsV2[] = {
    0x50, 0x29, 0xD2, 0x84, 0x34, 0x0A, 0x00, 0x00,
    0x98, 0x41, 0xD9, 0x84, 0x20, 0x25, 0x8E, 0xB1,
    0x68, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x6E, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x6D, 0x00,
    0x69, 0x00, 0x7A, 0x00, 0x65, 0x00, 0x64, 0x00,
    0x20, 0x00, 0x73, 0x00, 0x74, 0x00, 0x72, 0x00,
    0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x2E, 0x00,
    0x20, 0x00, 0x44, 0x00, 0x75, 0x00, 0x6D, 0x00,
    0x6D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x63, 0x00,
    0x6F, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x6E, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x00, 0x00 };

const unsigned char kFileIODirEnumPayloadV3[] = {
    0xD8, 0x1C, 0x00, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x8F, 0xCD, 0x05, 0x00, 0xE0, 0xFF, 0xFF,
    0xC0, 0x75, 0xF6, 0x00, 0x00, 0xC0, 0xFF, 0xFF,
    0x40, 0x07, 0x00, 0x00, 0x78, 0x02, 0x00, 0x00,
    0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x00, 0x00 };

const unsigned char kFileIOFlushPayloadV2[] = {
    0x60, 0x0E, 0x91, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0xA4, 0x8C, 0x01, 0x80, 0xFA, 0xFF, 0xFF,
    0x10, 0xFB, 0x92, 0x00, 0xA0, 0xF8, 0xFF, 0xFF
    };

const unsigned char kFileIOFlushPayload32bitsV2[] = {
    0x08, 0x4C, 0xCC, 0x86, 0x28, 0x0B, 0x00, 0x00,
    0x80, 0xE6, 0xDB, 0x84, 0x78, 0xBD, 0x6A, 0xA3
    };

const unsigned char kFileIOFlushPayloadV3[] = {
    0x08, 0x9B, 0xD4, 0x00, 0x00, 0xE0, 0xFF, 0xFF,
    0x60, 0x66, 0xA7, 0x00, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x91, 0x77, 0x1C, 0x00, 0xC0, 0xFF, 0xFF,
    0x6C, 0x0D, 0x00, 0x00 };

const unsigned char kFileIOQueryInfoPayloadV2[] = {
    0x60, 0xEC, 0x64, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x38, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xB0, 0xE4, 0x17, 0x04, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0xF1, 0xAE, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00 };

const unsigned char kFileIOQueryInfoPayload32bitsV2[] = {
    0x40, 0xCE, 0xE3, 0x84, 0x34, 0x0A, 0x00, 0x00,
    0x98, 0x41, 0xD9, 0x84, 0x08, 0xED, 0x8F, 0x9F,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOQueryInfoPayloadV3[] = {
    0x38, 0x16, 0x33, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0xE0, 0x87, 0xB6, 0x02, 0x00, 0xE0, 0xFF, 0xFF,
    0x00, 0xA6, 0xBF, 0x00, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x98, 0x0D, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00
    };

const unsigned char kFileIOFSControlPayloadV2[] = {
    0xC0, 0xB0, 0x06, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x64, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x50, 0xC2, 0x03, 0x80, 0xFA, 0xFF, 0xFF,
    0x10, 0xD0, 0x8E, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF4, 0x00, 0x09, 0x00 };

const unsigned char kFileIOFSControlPayload32bitsV2[] = {
    0x40, 0xCE, 0xE3, 0x84, 0xE8, 0x0E, 0x00, 0x00,
    0xA8, 0x41, 0x76, 0x87, 0x98, 0x9D, 0xAF, 0x85,
    0x00, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x09, 0x00
    };

const unsigned char kFileIOFSControlPayloadV3[] = {
    0xD8, 0x6C, 0x1E, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0xCF, 0x94, 0x04, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0xE7, 0xA6, 0x02, 0x00, 0xE0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xAC, 0x03, 0x00, 0x00, 0xBB, 0x00, 0x09, 0x00
    };

const unsigned char kFileIOOperationEndPayload32bitsV2[] = {
    0x50, 0x29, 0xD2, 0x84, 0xE0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kFileIOOperationEndPayloadV3[] = {
    0x38, 0x16, 0x33, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 };

const unsigned char kFileIODirNotifyPayloadV2[] = {
    0x60, 0x47, 0x4C, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x40, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x20, 0xAF, 0x39, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0x90, 0x9B, 0x5D, 0x06, 0xA0, 0xF8, 0xFF, 0xFF,
    0x00, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const unsigned char kFileIODirNotifyPayload32bitsV2[] = {
    0x20, 0x66, 0xE7, 0x84, 0x98, 0x15, 0x00, 0x00,
    0x28, 0x7C, 0xEC, 0x84, 0xF8, 0xF0, 0x9B, 0x9C,
    0x20, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const unsigned char kFileIODirNotifyPayloadV3[] = {
    0xA8, 0x49, 0x5C, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0x20, 0x0C, 0xE3, 0x05, 0x00, 0xE0, 0xFF, 0xFF,
    0x80, 0xEB, 0x48, 0x02, 0x00, 0xC0, 0xFF, 0xFF,
    0xBC, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00 };

const unsigned char kFileIODletePathPayloadV3[] = {
    0xB8, 0x3B, 0xE9, 0x00, 0x00, 0xE0, 0xFF, 0xFF,
    0x80, 0xB8, 0x04, 0x0A, 0x00, 0xE0, 0xFF, 0xFF,
    0x40, 0x41, 0xA7, 0x1B, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x07, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x00, 0x00 };

const unsigned char kFileIORenamePathPayloadV3[] = {
    0xD8, 0x1C, 0x00, 0x01, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x42, 0xF6, 0x04, 0x00, 0xE0, 0xFF, 0xFF,
    0x30, 0xEC, 0x02, 0x06, 0x00, 0xC0, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0x1E, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00,
    0x67, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x44, 0x00,
    0x75, 0x00, 0x6D, 0x00, 0x6D, 0x00, 0x79, 0x00,
    0x20, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00,
    0x2E, 0x00, 0x20, 0x00, 0x46, 0x00, 0x61, 0x00,
    0x6C, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x76, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x75, 0x00,
    0x65, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x46, 0x00,
    0x61, 0x00, 0x6B, 0x00, 0x65, 0x00, 0x20, 0x00,
    0x63, 0x00, 0x68, 0x00, 0x61, 0x00, 0x72, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x74, 0x00, 0x65, 0x00,
    0x72, 0x00, 0x73, 0x00, 0x2E, 0x00, 0x20, 0x00,
    0x41, 0x00, 0x6E, 0x00, 0x6F, 0x00, 0x6E, 0x00,
    0x79, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x7A, 0x00,
    0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x73, 0x00,
    0x74, 0x00, 0x72, 0x00, 0x00, 0x00 };

const unsigned char kDiskIOReadPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x06, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xC0, 0xA4, 0x43, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x9C, 0x22, 0x08, 0xA0, 0xF8, 0xFF, 0xFF,
    0x10, 0x15, 0x45, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0xA0, 0x7A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kDiskIOReadPayloadV3[] = {
    0x01, 0x00, 0x00, 0x00, 0x43, 0x00, 0x06, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0xD6, 0xAC, 0x01, 0x00, 0x00,
    0x40, 0x78, 0x47, 0x06, 0x00, 0xE0, 0xFF, 0xFF,
    0x10, 0x4B, 0xE1, 0x05, 0x00, 0xE0, 0xFF, 0xFF,
    0xAD, 0x8E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x90, 0x1B, 0x00, 0x00 };

const unsigned char kDiskIOWritePayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x06, 0x00,
    0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7F, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x50, 0xF7, 0xED, 0x02, 0xA0, 0xF8, 0xFF, 0xFF,
    0x60, 0xCB, 0x4E, 0x02, 0x80, 0xFA, 0xFF, 0xFF,
    0xC9, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kDiskIOWritePayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x06, 0x00,
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0x9C, 0xF5, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x4B, 0xA3, 0x02, 0x00, 0xE0, 0xFF, 0xFF,
    0x10, 0xF0, 0x71, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0xAD, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x1A, 0x00, 0x00 };

const unsigned char kDiskIOReadInitPayloadV2[] = {
    0x10, 0x15, 0x45, 0x02, 0x80, 0xFA, 0xFF, 0xFF
    };

const unsigned char kDiskIOReadInitPayloadV3[] = {
    0x10, 0x4B, 0xE1, 0x05, 0x00, 0xE0, 0xFF, 0xFF,
    0x90, 0x1B, 0x00, 0x00 };

const unsigned char kDiskIOWriteInitPayloadV2[] = {
    0x60, 0xCB, 0x4E, 0x02, 0x80, 0xFA, 0xFF, 0xFF
    };

const unsigned char kDiskIOWriteInitPayloadV3[] = {
    0x10, 0xF0, 0x71, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x1A, 0x00, 0x00 };

const unsigned char kDiskIOFlushBuffersPayloadV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
    0xB6, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x68, 0x3A, 0x02, 0x80, 0xFA, 0xFF, 0xFF
    };

const unsigned char kDiskIOFlushBuffersPayloadV3[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x59, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0x97, 0x55, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x1A, 0x00, 0x00 };

const unsigned char kDiskIOFlushInitPayloadV2[] = {
    0x80, 0x68, 0x3A, 0x02, 0x80, 0xFA, 0xFF, 0xFF
    };

const unsigned char kDiskIOFlushInitPayloadV3[] = {
    0x50, 0x97, 0x55, 0x07, 0x00, 0xE0, 0xFF, 0xFF,
    0xF0, 0x1A, 0x00, 0x00 };

const unsigned char kStackWalkStackPayloadV2[] = {
    0xBC, 0x6E, 0x9D, 0x03, 0x17, 0x01, 0x00, 0x00,
    0x94, 0x1E, 0x00, 0x00, 0x7C, 0x05, 0x00, 0x00,
    0x2B, 0x37, 0x5D, 0xED, 0x01, 0xF8, 0xFF, 0xFF,
    0x9A, 0x20, 0xF1, 0x78, 0xFB, 0x7F, 0x00, 0x00,
    0x8B, 0x2A, 0xF1, 0x78, 0xFB, 0x7F, 0x00, 0x00,
    0x5E, 0x5D, 0x44, 0x58, 0xFB, 0x7F, 0x00, 0x00,
    0x04, 0x3A, 0x4F, 0x58, 0xFB, 0x7F, 0x00, 0x00,
    0x45, 0x8E, 0x11, 0x5B, 0xFB, 0x7F, 0x00, 0x00,
    0xB9, 0x8B, 0x11, 0x5B, 0xFB, 0x7F, 0x00, 0x00,
    0x97, 0x8B, 0x11, 0x5B, 0xFB, 0x7F, 0x00, 0x00,
    0x91, 0x42, 0x10, 0x5B, 0xFB, 0x7F, 0x00, 0x00,
    0x73, 0xD1, 0x19, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x2E, 0xD0, 0x19, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x13, 0x5B, 0x23, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x49, 0x3A, 0x36, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x19, 0x4C, 0x1A, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0xA0, 0x4B, 0x1A, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x11, 0x4B, 0x1A, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x53, 0x4C, 0x1A, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0x22, 0x39, 0x36, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0xE2, 0xF3, 0x19, 0x60, 0xFB, 0x7F, 0x00, 0x00,
    0xCD, 0x15, 0x52, 0x7A, 0xFB, 0x7F, 0x00, 0x00,
    0xD1, 0x43, 0xFB, 0x7A, 0xFB, 0x7F, 0x00, 0x00
    };

const unsigned char kPageFaultTransitionFaultPayload32bitsV2[] = {
    0x2D, 0x8E, 0x38, 0x77, 0x2D, 0x8E, 0x38, 0x77
    };

const unsigned char kPageFaultTransitionFaultPayloadV2[] = {
    0x26, 0x2C, 0xE6, 0xFD, 0xFE, 0x07, 0x00, 0x00,
    0x26, 0x2C, 0xE6, 0xFD, 0xFE, 0x07, 0x00, 0x00
    };

const unsigned char kPageFaultDemandZeroFaultPayloadV2[] = {
    0x20, 0xE0, 0xFA, 0xFF, 0xFF, 0x07, 0x00, 0x00,
    0xD6, 0xFE, 0x17, 0x03, 0x00, 0xF8, 0xFF, 0xFF
    };

const unsigned char kPageFaultCopyOnWritePayloadV2[] = {
    0x28, 0xB2, 0xFF, 0xFD, 0xFE, 0x07, 0x00, 0x00,
    0x69, 0x54, 0x5D, 0x77, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kPageFaultAccessViolationPayloadV2[] = {
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x07, 0x00, 0x00,
    0x8A, 0xCD, 0x22, 0x00, 0x60, 0xF9, 0xFF, 0xFF
    };

const unsigned char kPageFaultHardPageFaultPayloadV2[] = {
    0x00, 0xC0, 0x66, 0x49, 0x80, 0xF9, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

const unsigned char kPageFaultHardFaultPayload32bitsV2[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x6B, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x5B, 0xA5, 0x08, 0xB0, 0xB1, 0x85,
    0x90, 0x13, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00
    };

const unsigned char kPageFaultHardFaultPayloadV2[] = {
    0x5D, 0xA5, 0x88, 0x13, 0x19, 0x00, 0x00, 0x00,
    0x00, 0x50, 0xFB, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x3B, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x5A, 0xA4, 0x11, 0x80, 0xFA, 0xFF, 0xFF,
    0x1C, 0x27, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00
    };

const unsigned char kPageFaultVirtualAllocPayloadV2[] = {
    0x00, 0x40, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x18, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00
    };

const unsigned char kPageFaultVirtualFreePayload32bitsV2[] = {
    0x00, 0x00, 0x42, 0x01, 0x00, 0x00, 0x04, 0x00,
    0xD8, 0x0D, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00
    };

const unsigned char kPageFaultVirtualFreePayloadV2[] = {
    0x00, 0x40, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x18, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00
    };

std::unique_ptr<Value> MakeSID32(uint32_t psid,
                                 uint32_t attributes,
                                 const unsigned char bytes[],
                                 size_t length) {
  std::unique_ptr<StructValue> sid_struct(new StructValue());
  sid_struct->AddField<UIntValue>("PSid", psid);
  sid_struct->AddField<UIntValue>("Attributes", attributes);

  std::unique_ptr<ArrayValue> sid_array(new ArrayValue());
  sid_array->AppendAll<UCharValue>(bytes, length);
  sid_struct->AddField("Sid", std::move(sid_array));

  return std::move(sid_struct);
}

std::unique_ptr<Value> MakeSID64(uint64_t psid,
                                 uint32_t attributes,
                                 const unsigned char bytes[],
                                 size_t length) {
  std::unique_ptr<StructValue> sid_struct(new StructValue());
  sid_struct->AddField<ULongValue>("PSid", psid);
  sid_struct->AddField<UIntValue>("Attributes", attributes);

  std::unique_ptr<ArrayValue> sid_array(new ArrayValue());
  sid_array->AppendAll<UCharValue>(bytes, length);
  sid_struct->AddField("Sid", std::move(sid_array));

  return std::move(sid_struct);
}

std::unique_ptr<Value> MakeSystemTime(int16_t year, int16_t month, int16_t dayOfWeek,
    int16_t day, int16_t hour, int16_t minute, int16_t second, int16_t milliseconds) {
  std::unique_ptr<StructValue> systemtime_struct(new StructValue());
  systemtime_struct->AddField<ShortValue>("wYear", year);
  systemtime_struct->AddField<ShortValue>("wMonth", month);
  systemtime_struct->AddField<ShortValue>("wDayOfWeek", dayOfWeek);
  systemtime_struct->AddField<ShortValue>("wDay", day);
  systemtime_struct->AddField<ShortValue>("wHour", hour);
  systemtime_struct->AddField<ShortValue>("wMinute", minute);
  systemtime_struct->AddField<ShortValue>("wSecond", second);
  systemtime_struct->AddField<ShortValue>("wMilliseconds", milliseconds);

  return std::move(systemtime_struct);
}

}  // namespace

TEST(EtwRawDecoderTest, EventTraceHeaderV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kEventTraceEventProviderId,
          kVersion2, kEventTraceEventHeaderOpcode, k64bit,
          reinterpret_cast<const char*>(&kEventTraceEventHeaderPayloadV2[0]),
          sizeof(kEventTraceEventHeaderPayloadV2),
          &operation, &category, &fields));

  // Expected TimeZone structure.
  std::unique_ptr<StructValue> timezone(new StructValue);
  timezone->AddField<IntValue>("Bias", 0x12C);
  const std::wstring standard_name = L"@tzres.dll,-112";
  timezone->AddField<WStringValue>("StandardName", standard_name);
  timezone->AddField("StandardDate",
      MakeSystemTime(0, 11, 0, 1, 2, 0, 0, 0));
  timezone->AddField<IntValue>("StandardBias", 0);
  const std::wstring daylight_name = L"@tzres.dll,-111";
  timezone->AddField<WStringValue>("DaylightName", daylight_name);
  timezone->AddField("DaylightDate",
      MakeSystemTime(0, 3, 0, 2, 2, 0, 0, 0));
  timezone->AddField<IntValue>("DaylightBias", -60);

  // Expected structure.
  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BufferSize", 65536);
  expected->AddField<UIntValue>("Version", 83951878);
  expected->AddField<UIntValue>("ProviderVersion", 7601);
  expected->AddField<UIntValue>("NumberOfProcessors", 4);
  expected->AddField<ULongValue>("EndTime", 130371671034768955ULL);
  expected->AddField<UIntValue>("TimerResolution", 156001);
  expected->AddField<UIntValue>("MaxFileSize", 0);
  expected->AddField<UIntValue>("LogFileMode", 0x10001);
  expected->AddField<UIntValue>("BuffersWritten", 438);
  expected->AddField<UIntValue>("StartBuffers", 1);
  expected->AddField<UIntValue>("PointerSize", 8);
  expected->AddField<UIntValue>("EventsLost", 31);
  expected->AddField<UIntValue>("CPUSpeed", 1696);
  expected->AddField<ULongValue>("LoggerName", 0);
  expected->AddField<ULongValue>("LogFileName", 0);
  expected->AddField("TimeZoneInformation", std::move(timezone));
  expected->AddField<UIntValue>("Padding", 0);
  expected->AddField<ULongValue>("BootTime", 130371020571099993ULL);
  expected->AddField<ULongValue>("PerfFreq", 1656445);
  expected->AddField<ULongValue>("StartTime", 130371670762939437ULL);
  expected->AddField<UIntValue>("ReservedFlags", 0x1);
  expected->AddField<UIntValue>("BuffersLost", 0);

  const std::wstring kSessionName = L"Relogger";
  expected->AddField<WStringValue>("SessionNameString", kSessionName);
  const std::wstring kLogfileName = L"C:\\kernel.etl";
  expected->AddField<WStringValue>("LogFileNameString", kLogfileName);

  EXPECT_STREQ("EventTraceEvent", category.c_str());
  EXPECT_STREQ("Header", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, EventTraceHeader32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kEventTraceEventProviderId,
          kVersion2, kEventTraceEventHeaderOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kEventTraceEventHeaderPayload32bitsV2[0]),
          sizeof(kEventTraceEventHeaderPayload32bitsV2),
          &operation, &category, &fields));

  // Expected TimeZone structure.
  std::unique_ptr<StructValue> timezone(new StructValue);
  timezone->AddField<IntValue>("Bias", 300);
  const std::wstring kStandardName = L"@tzres.dll,-112";
  timezone->AddField<WStringValue>("StandardName", kStandardName);
  timezone->AddField("StandardDate",
      MakeSystemTime(0, 11, 0, 1, 2, 0, 0, 0));
  timezone->AddField<IntValue>("StandardBias", 0);
  const std::wstring kDaylightName = L"@tzres.dll,-111";
  timezone->AddField<WStringValue>("DaylightName", kDaylightName);
  timezone->AddField("DaylightDate",
      MakeSystemTime(0, 3, 0, 2, 2, 0, 0, 0));
  timezone->AddField<IntValue>("DaylightBias", -60);

  // Expected structure.
  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BufferSize", 65536);
  expected->AddField<UIntValue>("Version", 83951878);
  expected->AddField<UIntValue>("ProviderVersion", 7600);
  expected->AddField<UIntValue>("NumberOfProcessors", 16);
  expected->AddField<ULongValue>("EndTime", 129488146014743569ULL);
  expected->AddField<UIntValue>("TimerResolution", 156001);
  expected->AddField<UIntValue>("MaxFileSize", 100);
  expected->AddField<UIntValue>("LogFileMode", 1);
  expected->AddField<UIntValue>("BuffersWritten", 3);
  expected->AddField<UIntValue>("StartBuffers", 1);
  expected->AddField<UIntValue>("PointerSize", 4);
  expected->AddField<UIntValue>("EventsLost", 0);
  expected->AddField<UIntValue>("CPUSpeed", 2394);
  expected->AddField<UIntValue>("LoggerName", 5);
  expected->AddField<UIntValue>("LogFileName", 6);
  expected->AddField("TimeZoneInformation", std::move(timezone));
  expected->AddField<UIntValue>("Padding", 0);
  expected->AddField<ULongValue>("BootTime", 129484742215811967ULL);
  expected->AddField<ULongValue>("PerfFreq", 2337949);
  expected->AddField<ULongValue>("StartTime", 129488145994691628ULL);
  expected->AddField<UIntValue>("ReservedFlags", 0x1);
  expected->AddField<UIntValue>("BuffersLost", 0);

  const std::wstring kSessionName = L"Make Test Data Session";
  expected->AddField<WStringValue>("SessionNameString", kSessionName);
  const std::wstring kLogfileName = L"c:\\src\\sawbuck\\trunk\\src\\sawbuck\\"
      L"log_lib\\test_data\\image_data_32_v0.etl";
  expected->AddField<WStringValue>("LogFileNameString", kLogfileName);

  EXPECT_STREQ("EventTraceEvent", category.c_str());
  EXPECT_STREQ("Header", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, EventTraceExtension32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kEventTraceEventProviderId,
          kVersion2, kEventTraceEventExtensionOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kEventTraceEventExtensionPayload32bitsV2[0]),
          sizeof(kEventTraceEventExtensionPayload32bitsV2),
          &operation, &category, &fields));

  // Expected structure.
  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("GroupMask1", 0);
  expected->AddField<UIntValue>("GroupMask2", 0);
  expected->AddField<UIntValue>("GroupMask3", 0);
  expected->AddField<UIntValue>("GroupMask4", 0);
  expected->AddField<UIntValue>("GroupMask5", 0);
  expected->AddField<UIntValue>("GroupMask6", 0);
  expected->AddField<UIntValue>("GroupMask7", 0);
  expected->AddField<UIntValue>("GroupMask8", 0);
  expected->AddField<UIntValue>("KernelEventVersion", 25);

  EXPECT_STREQ("EventTraceEvent", category.c_str());
  EXPECT_STREQ("Extension", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, EventTraceExtensionV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kEventTraceEventProviderId,
          kVersion2, kEventTraceEventExtensionOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kEventTraceEventExtensionPayloadV2[0]),
          sizeof(kEventTraceEventExtensionPayloadV2),
          &operation, &category, &fields));

  // Expected structure.
  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("GroupMask1", 0);
  expected->AddField<UIntValue>("GroupMask2", 0);
  expected->AddField<UIntValue>("GroupMask3", 0);
  expected->AddField<UIntValue>("GroupMask4", 0);
  expected->AddField<UIntValue>("GroupMask5", 0);
  expected->AddField<UIntValue>("GroupMask6", 0);
  expected->AddField<UIntValue>("GroupMask7", 0);
  expected->AddField<UIntValue>("GroupMask8", 0);
  expected->AddField<UIntValue>("KernelEventVersion", 25);

  EXPECT_STREQ("EventTraceEvent", category.c_str());
  EXPECT_STREQ("Extension", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageUnloadV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageUnloadOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageUnloadPayloadV2[0]),
          sizeof(kImageUnloadPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 0x7FEF7780000ULL);
  expected->AddField<ULongValue>("ModuleSize", 0xE2000ULL);
  expected->AddField<UIntValue>("ProcessId", 5956U);
  expected->AddField<UIntValue>("ImageCheckSum", 948129U);
  expected->AddField<UIntValue>("TimeDateStamp", 1247534846U);
  expected->AddField<UIntValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 0x7FEF7780000ULL);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\Windows\\System32\\wbem\\fastprox.dll";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("Unload", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageUnloadV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion3, kImageUnloadOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageUnloadPayloadV3[0]),
          sizeof(kImageUnloadPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 140723059097600ULL);
  expected->AddField<ULongValue>("ModuleSize", 933888ULL);
  expected->AddField<UIntValue>("ProcessId", 2040U);
  expected->AddField<UIntValue>("ImageCheckSum", 929403U);
  expected->AddField<UIntValue>("TimeDateStamp", 1377164984U);
  expected->AddField<UCharValue>("SignatureLevel", 0);
  expected->AddField<UCharValue>("SignatureType", 0);
  expected->AddField<UShortValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 140723059097600ULL);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\Windows\\System32\\wbem\\fastprox.dll";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("Unload", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCStart32bitsV0) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion0, kImageDCStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kImageDCStartPayload32bitsV0[0]),
          sizeof(kImageDCStartPayload32bitsV0),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BaseAddress", 0x1160000);
  expected->AddField<UIntValue>("ModuleSize", 1695744);
  const std::wstring kFilename =
      L"C:\\code\\sawbuck\\src\\sawbuck\\Debug\\test_program.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCStart32bitsV1) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion1, kImageDCStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kImageDCStartPayload32bitsV1[0]),
          sizeof(kImageDCStartPayload32bitsV1),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BaseAddress", 0x1160000);
  expected->AddField<UIntValue>("ModuleSize", 0x19E000);
  expected->AddField<UIntValue>("ProcessId", 7644);
  const std::wstring kFilename =
      L"C:\\code\\sawbuck\\src\\sawbuck\\Debug\\test_program.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCStart32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageDCStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kImageDCStartPayload32bitsV2[0]),
          sizeof(kImageDCStartPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BaseAddress", 0x1160000);
  expected->AddField<UIntValue>("ModuleSize", 0x19E000);
  expected->AddField<UIntValue>("ProcessId", 7644U);
  expected->AddField<UIntValue>("ImageCheckSum", 1268934759U);
  expected->AddField<UIntValue>("TimeDateStamp", 3405691582U);
  expected->AddField<UIntValue>("Reserved0", 0U);
  expected->AddField<UIntValue>("DefaultBase", 0U);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename =
      L"C:\\code\\sawbuck\\src\\sawbuck\\Debug\\test_program.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCStartV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageDCStartPayloadV2[0]),
          sizeof(kImageDCStartPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 18446735277664796672ULL);
  expected->AddField<ULongValue>("ModuleSize", 0x5E6000ULL);
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("ImageCheckSum", 5612101U);
  expected->AddField<UIntValue>("TimeDateStamp", 0U);
  expected->AddField<UIntValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 0U);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\SystemRoot\\system32\\ntoskrnl.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion3, kImageDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageDCStartPayloadV3[0]),
          sizeof(kImageDCStartPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 2001010688ULL);
  expected->AddField<ULongValue>("ModuleSize", 1474560ULL);
  expected->AddField<UIntValue>("ProcessId", 4U);
  expected->AddField<UIntValue>("ImageCheckSum", 1490712);
  expected->AddField<UIntValue>("TimeDateStamp", 0U);
  expected->AddField<UCharValue>("SignatureLevel", 12);
  expected->AddField<UCharValue>("SignatureType", 1);
  expected->AddField<UShortValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 2001010688ULL);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename =
      L"\\Device\\HarddiskVolume4\\Windows\\SysWOW64\\ntdll.dll";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCEndV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageDCEndOpcode, true,
          reinterpret_cast<const char*>(&kImageDCEndPayloadV2[0]),
          sizeof(kImageDCEndPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 18446735277664866304ULL);
  expected->AddField<ULongValue>("ModuleSize", 0x5E5000ULL);
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("ImageCheckSum", 5557171U);
  expected->AddField<UIntValue>("TimeDateStamp", 0U);
  expected->AddField<UIntValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 0U);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\SystemRoot\\system32\\ntoskrnl.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageDCEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion3, kImageDCEndOpcode, true,
          reinterpret_cast<const char*>(&kImageDCEndPayloadV3[0]),
          sizeof(kImageDCEndPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 18446735279571529728ULL);
  expected->AddField<ULongValue>("ModuleSize", 7868416ULL);
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("ImageCheckSum", 7413974U);
  expected->AddField<UIntValue>("TimeDateStamp", 1383173532U);
  expected->AddField<UCharValue>("SignatureLevel", 0);
  expected->AddField<UCharValue>("SignatureType", 1);
  expected->AddField<UShortValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 0U);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\SystemRoot\\system32\\ntoskrnl.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("DCEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageLoadV0) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion0, kImageLoadOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageLoadPayloadV0[0]),
          sizeof(kImageLoadPayloadV0),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 0x01160000);
  expected->AddField<UIntValue>("ModuleSize", 0x0019e000);
  const std::wstring kFilename =
      L"C:\\code\\sawbuck\\src\\sawbuck\\Debug\\test_program.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("Load", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageLoadV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageLoadOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageLoadPayloadV2[0]),
          sizeof(kImageLoadPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 0x71400000ULL);
  expected->AddField<ULongValue>("ModuleSize", 0x8000ULL);
  expected->AddField<UIntValue>("ProcessId", 3828U);
  expected->AddField<UIntValue>("ImageCheckSum", 65178U);
  expected->AddField<UIntValue>("TimeDateStamp", 1247527908U);
  expected->AddField<UIntValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 0x7140000000005000ULL);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename = L"\\Windows\\SysWOW64\\wscisvif.dll";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("Load", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageLoadV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion3, kImageLoadOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageLoadPayloadV3[0]),
          sizeof(kImageLoadPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 140699811512320ULL);
  expected->AddField<ULongValue>("ModuleSize", 430080U);
  expected->AddField<UIntValue>("ProcessId", 2700U);
  expected->AddField<UIntValue>("ImageCheckSum", 486961U);
  expected->AddField<UIntValue>("TimeDateStamp", 1343266205U);
  expected->AddField<UCharValue>("SignatureLevel", 0);
  expected->AddField<UCharValue>("SignatureType", 0);
  expected->AddField<UShortValue>("Reserved0", 0U);
  expected->AddField<ULongValue>("DefaultBase", 140699811512320ULL);
  expected->AddField<UIntValue>("Reserved1", 0U);
  expected->AddField<UIntValue>("Reserved2", 0U);
  expected->AddField<UIntValue>("Reserved3", 0U);
  expected->AddField<UIntValue>("Reserved4", 0U);
  const std::wstring kFilename =
      L"\\Device\\HarddiskVolume4\\Program Files (x86)\\"
      L"Windows Kits\\8.0\\Windows Performance Toolkit\\xperf.exe";
  expected->AddField<WStringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("Load", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ImageKernelBaseV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kImageProviderId,
          kVersion2, kImageKernelBaseOpcode, k64bit,
          reinterpret_cast<const char*>(&kImageKernelBasePayloadV2[0]),
          sizeof(kImageKernelBasePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 18446735277664866304ULL);

  EXPECT_STREQ("Image", category.c_str());
  EXPECT_STREQ("KernelBase", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}


TEST(EtwRawDecoderTest, ProcessStart32bitsV1) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion1, kProcessStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kProcessStartPayload32bitsV1[0]),
          sizeof(kProcessStartPayload32bitsV1),
          &operation, &category, &fields));

  const unsigned char sid[] = {
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PageDirectoryBase", 0);
  expected->AddField<UIntValue>("ProcessId", 1776);
  expected->AddField<UIntValue>("ParentId", 988);
  expected->AddField<UIntValue>("SessionId", 1);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField("UserSID", MakeSID32(0,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "notepad.exe";
  expected->AddField<StringValue>("ImageFileName", kFilename);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessStart32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kProcessStartPayload32bitsV2[0]),
          sizeof(kProcessStartPayload32bitsV2),
          &operation, &category, &fields));

  const unsigned char sid[] = {
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("UniqueProcessKey", 0);
  expected->AddField<UIntValue>("ProcessId", 1776);
  expected->AddField<UIntValue>("ParentId", 988);
  expected->AddField<UIntValue>("SessionId", 1);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField("UserSID", MakeSID32(0,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "notepad.exe";
  expected->AddField<StringValue>("ImageFileName", kFilename);
  const std::wstring kCommandLine = L"\"C:\\Windows\\system32\\notepad.exe\" ";
  expected->AddField<WStringValue>("CommandLine", kCommandLine);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessStart32bitsV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion3, kProcessStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kProcessStartPayload32bitsV3[0]),
          sizeof(kProcessStartPayload32bitsV3),
          &operation, &category, &fields));

  const unsigned char sid[] = {
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("UniqueProcessKey", 0);
  expected->AddField<UIntValue>("ProcessId", 1776);
  expected->AddField<UIntValue>("ParentId", 988);
  expected->AddField<UIntValue>("SessionId", 1);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField<UIntValue>("DirectoryTableBase", 0);
  expected->AddField("UserSID", MakeSID32(0,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "notepad.exe";
  expected->AddField<StringValue>("ImageFileName", kFilename);
  const std::wstring kCommandLine = L"\"C:\\Windows\\system32\\notepad.exe\" ";
  expected->AddField<WStringValue>("CommandLine", kCommandLine);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessStartV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessStartPayloadV2[0]),
          sizeof(kProcessStartPayloadV2),
          &operation, &category, &fields));

  const unsigned char sid[] = {
    0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00, 0x96, 0x2C, 0xEC, 0x2C,
    0x68, 0xFD, 0x31, 0x06, 0xF1, 0xDC, 0xA4, 0xD3,
    0xE8, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 0);
  expected->AddField<UIntValue>("ProcessId", 1776);
  expected->AddField<UIntValue>("ParentId", 988);
  expected->AddField<UIntValue>("SessionId", 1);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField("UserSID", MakeSID64(0,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "notepad.exe";
  expected->AddField<StringValue>("ImageFileName", kFilename);
  const std::wstring kCommandLine = L"\"C:\\Windows\\system32\\notepad.exe\" ";
  expected->AddField<WStringValue>("CommandLine", kCommandLine);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion3, kProcessStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessStartPayloadV3[0]),
          sizeof(kProcessStartPayloadV3),
          &operation, &category, &fields));

  const unsigned char sid[] = { 1, 5, 0, 0, 0, 0, 0, 5, 21, 0, 0, 0, 2, 3,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446738026653712480ULL);
  expected->AddField<UIntValue>("ProcessId", 6656);
  expected->AddField<UIntValue>("ParentId", 7328);
  expected->AddField<UIntValue>("SessionId", 1);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField<ULongValue>("DirectoryTableBase", 4785958912);
  expected->AddField("UserSID", MakeSID64(18446735965169079856ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string filename = "xperf.exe";
  expected->AddField<StringValue>("ImageFileName", filename);
  const std::wstring commandline = L"xperf  -d out.etl";
  expected->AddField<WStringValue>("CommandLine", commandline);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessStartV4) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion4, kProcessStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessStartPayloadV4[0]),
          sizeof(kProcessStartPayloadV4),
          &operation, &category, &fields));

  const unsigned char sid[] = {
      0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x15, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
      0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x06,
      0xE9, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446708889790201984ULL);
  expected->AddField<UIntValue>("ProcessId", 2700U);
  expected->AddField<UIntValue>("ParentId", 5896U);
  expected->AddField<UIntValue>("SessionId", 5U);
  expected->AddField<IntValue>("ExitStatus", 259);
  expected->AddField<ULongValue>("DirectoryTableBase", 2745348096ULL);
  expected->AddField<UIntValue>("Flags", 0U);
  expected->AddField("UserSID", MakeSID64(18446673705038246032ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));

  expected->AddField<StringValue>("ImageFileName", "xperf.exe");
  expected->AddField<WStringValue>("CommandLine", L"xperf  -stop");
  expected->AddField<WStringValue>("PackageFullName", L"");
  expected->AddField<WStringValue>("ApplicationId", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion3, kProcessEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessEndPayloadV3[0]),
          sizeof(kProcessEndPayloadV3),
          &operation, &category, &fields));

  const unsigned char sid[] = { 1, 5, 0, 0, 0, 0, 0, 5, 21, 0, 0, 0, 1, 2, 3,
      4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 3, 0, 0 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446738026653712480ULL);
  expected->AddField<UIntValue>("ProcessId", 8236ULL);
  expected->AddField<UIntValue>("ParentId", 7328U);
  expected->AddField<UIntValue>("SessionId", 1U);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 2755633152U);
  expected->AddField("UserSID", MakeSID64(18446735965099372992ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "xperf.exe";
  expected->AddField<StringValue>("ImageFileName", kFilename);
  const std::wstring kCommandLine =
     L"xperf  -on PROC_THREAD+LOADER+CSWITCH -stackwalk ImageLoad+ImageUnload";
  expected->AddField<WStringValue>("CommandLine", kCommandLine);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("End", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessEndV4) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion4, kProcessEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessEndPayloadV4[0]),
          sizeof(kProcessEndPayloadV4),
          &operation, &category, &fields));

  const unsigned char sid[] = {
      0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x15, 0x00, 0x00, 0x00, 0x12, 0x13, 0x0F, 0x12,
      0x13, 0x42, 0x24, 0x33, 0xCC, 0xCA, 0xCC, 0xCB,
      0xBA, 0xBE, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446708889790201984ULL);
  expected->AddField<UIntValue>("ProcessId", 2040U);
  expected->AddField<UIntValue>("ParentId", 5896U);
  expected->AddField<UIntValue>("SessionId", 5U);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 7478476800ULL);
  expected->AddField<UIntValue>("Flags", 0U);
  expected->AddField("UserSID", MakeSID64(18446673705334261920ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));

  expected->AddField<StringValue>("ImageFileName", "xperf.exe");
  expected->AddField<WStringValue>("CommandLine",
      L"xperf  -on PROC_THREAD+LOADER+PROFILE+CSWITCH+DISPATCHER+DPC+INTERRUPT"
      L"+SYSCALL+PRIORITY+SPINLOCK+PERF_COUNTER+DISK_IO+DISK_IO_INIT+FILE_IO+"
      L"FILE_IO_INIT+HARD_FAULTS+FILENAME+REGISTRY+DRIVERS+POWER+CC+"
      L"NETWORKTRACE+VIRT_ALLOC+MEMINFO+MEMORY+TIMER -f C:\\kernel.etl "
      L"-BufferSize 4096 -MinBuffers 256 -MaxBuffers 256");
  expected->AddField<WStringValue>("PackageFullName", L"");
  expected->AddField<WStringValue>("ApplicationId", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("End", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDCStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion3, kProcessDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDCStartPayloadV3[0]),
          sizeof(kProcessDCStartPayloadV3),
          &operation, &category, &fields));

  const unsigned char sid[] = { 1, 1, 0, 0, 0, 0, 0, 5, 16, 0, 0, 0 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey",  18446735277666959744ULL);
  expected->AddField<UIntValue>("ProcessId", 0);
  expected->AddField<UIntValue>("ParentId", 0U);
  expected->AddField<UIntValue>("SessionId", 0xFFFFFFFFU);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 1601536);
  expected->AddField("UserSID", MakeSID64(18446735965522384448ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  const std::string kFilename = "Idle";
  expected->AddField<StringValue>("ImageFileName", kFilename);
  const std::wstring kCommandLine = L"";
  expected->AddField<WStringValue>("CommandLine", kCommandLine);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDCStartV4) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion4, kProcessDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDCStartPayloadV4[0]),
          sizeof(kProcessDCStartPayloadV4),
          &operation, &category, &fields));

  const unsigned char sid[] = { 1, 1, 0, 0, 0, 0, 0, 5, 16, 0, 0, 0 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446735279574963136ULL);
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("ParentId", 0U);
  expected->AddField<UIntValue>("SessionId", 4294967295U);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 1736704ULL);
  expected->AddField<UIntValue>("Flags", 0U);
  expected->AddField("UserSID", MakeSID64(18446673705735535552ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  expected->AddField<StringValue>("ImageFileName", "Idle");
  expected->AddField<WStringValue>("CommandLine", L"");
  expected->AddField<WStringValue>("PackageFullName", L"");
  expected->AddField<WStringValue>("ApplicationId", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDCEndV4) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion4, kProcessDCEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDCEndPayloadV4[0]),
          sizeof(kProcessDCEndPayloadV4),
          &operation, &category, &fields));

  const unsigned char sid[] = {
      0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x10, 0x00, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446735279574963136ULL);
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("ParentId", 0U);
  expected->AddField<UIntValue>("SessionId", 4294967295U);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 1736704ULL);
  expected->AddField<UIntValue>("Flags", 0U);
  expected->AddField("UserSID", MakeSID64(18446673705343288816ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  expected->AddField<StringValue>("ImageFileName", "Idle");
  expected->AddField<WStringValue>("CommandLine", L"");
  expected->AddField<WStringValue>("PackageFullName", L"");
  expected->AddField<WStringValue>("ApplicationId", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("DCEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessTerminateV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessTerminateOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessTerminatePayloadV2[0]),
          sizeof(kProcessTerminatePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 2040U);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Terminate", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessPerfCtr32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessPerfCtrOpcode, k32bit,
          reinterpret_cast<const char*>(&kProcessPerfCtrPayload32bitsV2[0]),
          sizeof(kProcessPerfCtrPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 4804);
  expected->AddField<UIntValue>("PageFaultCount", 0U);
  expected->AddField<UIntValue>("HandleCount", 0U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<UIntValue>("PeakVirtualSize", 40046592);
  expected->AddField<UIntValue>("PeakWorkingSetSize", 5488640);
  expected->AddField<UIntValue>("PeakPagefileUsage", 2265088);
  expected->AddField<UIntValue>("QuotaPeakPagedPoolUsage", 73884);
  expected->AddField<UIntValue>("QuotaPeakNonPagedPoolUsage", 5068);
  expected->AddField<UIntValue>("VirtualSize", 0);
  expected->AddField<UIntValue>("WorkingSetSize", 0);
  expected->AddField<UIntValue>("PagefileUsage", 0);
  expected->AddField<UIntValue>("QuotaPagedPoolUsage", 0);
  expected->AddField<UIntValue>("QuotaNonPagedPoolUsage", 0);
  expected->AddField<UIntValue>("PrivatePageCount", 0ULL);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("PerfCtr", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessPerfCtrV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessPerfCtrOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessPerfCtrPayloadV2[0]),
          sizeof(kProcessPerfCtrPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 2040U);
  expected->AddField<UIntValue>("PageFaultCount", 0U);
  expected->AddField<UIntValue>("HandleCount", 0U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("PeakVirtualSize", 61681664ULL);
  expected->AddField<ULongValue>("PeakWorkingSetSize", 6537216ULL);
  expected->AddField<ULongValue>("PeakPagefileUsage", 2191360ULL);
  expected->AddField<ULongValue>("QuotaPeakPagedPoolUsage", 113160ULL);
  expected->AddField<ULongValue>("QuotaPeakNonPagedPoolUsage", 9696ULL);
  expected->AddField<ULongValue>("VirtualSize", 0ULL);
  expected->AddField<ULongValue>("WorkingSetSize", 0ULL);
  expected->AddField<ULongValue>("PagefileUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaNonPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("PrivatePageCount", 0ULL);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("PerfCtr", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessPerfCtrRundownV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessPerfCtrRundownOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessPerfCtrRundownPayloadV2[0]),
          sizeof(kProcessPerfCtrRundownPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("PageFaultCount", 1U);
  expected->AddField<UIntValue>("HandleCount", 1123U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("PeakVirtualSize", 65536ULL);
  expected->AddField<ULongValue>("PeakWorkingSetSize", 24576ULL);
  expected->AddField<ULongValue>("PeakPagefileUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaPeakPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaPeakNonPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("VirtualSize", 65536ULL);
  expected->AddField<ULongValue>("WorkingSetSize", 24576ULL);
  expected->AddField<ULongValue>("PagefileUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("QuotaNonPagedPoolUsage", 0ULL);
  expected->AddField<ULongValue>("PrivatePageCount", 0ULL);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("PerfCtrRundown", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDefunctV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion2, kProcessDefunctOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDefunctPayloadV2[0]),
          sizeof(kProcessDefunctPayloadV2),
          &operation, &category, &fields));

  const unsigned char sid[] = {
      0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x15, 0x00, 0x00, 0x00, 0x3E, 0x66, 0xA1, 0xD8,
      0xD6, 0x0A, 0x05, 0xD1, 0x4F, 0x2E, 0xC7, 0x3C,
      0xEC, 0x03, 0x00, 0x00
  };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446738026664798208ULL);
  expected->AddField<UIntValue>("ProcessId", 1832);
  expected->AddField<UIntValue>("ParentId", 716);
  expected->AddField<UIntValue>("SessionId", 0);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField("UserSID", MakeSID64(18446735827951636656ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  expected->AddField<StringValue>("ImageFileName", "cygrunsrv.exe");
  expected->AddField<WStringValue>("CommandLine", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Defunct", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDefunctV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;

  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion3, kProcessDefunctOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDefunctPayloadV3[0]),
          sizeof(kProcessDefunctPayloadV3),
          &operation, &category, &fields));

  const unsigned char sid[] = { 1, 1, 0, 0, 0, 0, 0, 5, 16, 0, 0, 0 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446738026725302368ULL);
  expected->AddField<UIntValue>("ProcessId", 3684U);
  expected->AddField<UIntValue>("ParentId", 2196U);
  expected->AddField<UIntValue>("SessionId", 0);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 6844006400ULL);
  expected->AddField("UserSID", MakeSID64(18446735964887549920ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  expected->AddField<StringValue>("ImageFileName", "cmd.exe");
  expected->AddField<WStringValue>("CommandLine", L"");

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Defunct", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ProcessDefunctV5) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kProcessProviderId,
          kVersion5, kProcessDefunctOpcode, k64bit,
          reinterpret_cast<const char*>(&kProcessDefunctPayloadV5[0]),
          sizeof(kProcessDefunctPayloadV5),
          &operation, &category, &fields));

  const unsigned char sid[] = {
      0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x15, 0x00, 0x00, 0x00, 0xC0, 0xC1, 0xC2, 0xC3,
      0xC4, 0xC5, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3,
      0xD4, 0x03, 0x00, 0x00 };

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("UniqueProcessKey", 18446708889454036416ULL);
  expected->AddField<UIntValue>("ProcessId", 6472U);
  expected->AddField<UIntValue>("ParentId", 2064U);
  expected->AddField<UIntValue>("SessionId", 1U);
  expected->AddField<IntValue>("ExitStatus", 0);
  expected->AddField<ULongValue>("DirectoryTableBase", 1338728448ULL);
  expected->AddField<UIntValue>("Flags", 0U);
  expected->AddField("UserSID", MakeSID64(18446673705019631088ULL,
                                          0,
                                          &sid[0],
                                          sizeof(sid)));
  expected->AddField<StringValue>("ImageFileName", "chrome.exe");
  expected->AddField<WStringValue>("CommandLine", L"");
  expected->AddField<WStringValue>("PackageFullName", L"");
  expected->AddField<WStringValue>("ApplicationId", L"");
  expected->AddField<ULongValue>("ExitTime", 130317334947711373ULL);

  EXPECT_STREQ("Process", category.c_str());
  EXPECT_STREQ("Defunct", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSampleProf32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSampleProfOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPerfInfoSampleProfPayload32bitsV2[0]),
          sizeof(kPerfInfoSampleProfPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("InstructionPointer", 0x82fc1a45);
  expected->AddField<UIntValue>("ThreadId", 3252);
  expected->AddField<UShortValue>("Count", 1);
  expected->AddField<UShortValue>("Reserved", 0);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SampleProf", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSampleProfV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSampleProfOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoSampleProfPayloadV2[0]),
          sizeof(kPerfInfoSampleProfPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InstructionPointer",
                                 18446735279571905355ULL);
  expected->AddField<UIntValue>("ThreadId", 8048U);
  expected->AddField<UShortValue>("Count", 1);
  expected->AddField<UShortValue>("Reserved", 64);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SampleProf", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoISRMSI32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoISRMSIOpcode, k32bit,
          reinterpret_cast<const char*>(&kPerfInfoISRMSIPayload32bitsV2[0]),
          sizeof(kPerfInfoISRMSIPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x000002AB91DE4FF8ULL);
  expected->AddField<UIntValue>("Routine", 0x8B8CA90E);
  expected->AddField<UCharValue>("ReturnValue", 1);
  expected->AddField<UShortValue>("Vector", 176);
  expected->AddField<UCharValue>("Reserved", 0);
  expected->AddField<UIntValue>("MessageNumber", 0U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("ISR-MSI", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoISRMSIV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoISRMSIOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoISRMSIPayloadV2[0]),
          sizeof(kPerfInfoISRMSIPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 4838955609579ULL);
  expected->AddField<ULongValue>("Routine", 18446735277626195488ULL);
  expected->AddField<UCharValue>("ReturnValue", 1);
  expected->AddField<UShortValue>("Vector", 145);
  expected->AddField<UCharValue>("Reserved", 0);
  expected->AddField<UIntValue>("MessageNumber", 0U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("ISR-MSI", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSysClEnter32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSysClEnterOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPerfInfoSysClEnterPayload32bitsV2[0]),
          sizeof(kPerfInfoSysClEnterPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("SysCallAddress", 0x82A7874F);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SysClEnter", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSysClEnterV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSysClEnterOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoSysClEnterPayloadV2[0]),
          sizeof(kPerfInfoSysClEnterPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("SysCallAddress", 18446735279572131108ULL);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SysClEnter", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSysClExit32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSysClExitOpcode, k32bit,
          reinterpret_cast<const char*>(&kPerfInfoSysClExitPayload32bitsV2[0]),
          sizeof(kPerfInfoSysClExitPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("SysCallNtStatus", 0x103U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SysClExit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoSysClExitV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoSysClExitOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoSysClExitPayloadV2[0]),
          sizeof(kPerfInfoSysClExitPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("SysCallNtStatus", 0U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("SysClExit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoISR32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoISROpcode, k32bit,
          reinterpret_cast<const char*>(&kPerfInfoISRPayload32bitsV2[0]),
          sizeof(kPerfInfoISRPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x000002AB91B1C0D4);
  expected->AddField<UIntValue>("Routine", 0x94DCEF00);
  expected->AddField<UCharValue>("ReturnValue", 0);
  expected->AddField<UShortValue>("Vector", 178);
  expected->AddField<UCharValue>("Reserved", 0);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("ISR", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoDebuggerEnabledV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoDebuggerEnabledOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoDebuggerEnabledPayloadV2[0]),
          0,  // This payload is empty.
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("DebuggerEnabled", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoDebuggerEnabledV2WithANullPayload) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoDebuggerEnabledOpcode, k64bit,
          NULL,
          0,  // This payload is empty.
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("DebuggerEnabled", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoISRV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoISROpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoISRPayloadV2[0]),
          sizeof(kPerfInfoISRPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 4838956092844ULL);
  expected->AddField<ULongValue>("Routine", 18446735277666407872ULL);
  expected->AddField<UCharValue>("ReturnValue", 0);
  expected->AddField<UShortValue>("Vector", 129);
  expected->AddField<UCharValue>("Reserved", 0);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("ISR", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoThreadesDPC32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoThreadedDPCOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPerfInfoThreadedDPCPayload32bitsV2[0]),
          sizeof(kPerfInfoThreadedDPCPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x000002AB91FD4D0A);
  expected->AddField<UIntValue>("Routine", 0x82837107);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("ThreadedDPC", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoDPC32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoDPCOpcode, k32bit,
          reinterpret_cast<const char*>(&kPerfInfoDPCPayload32bitsV2[0]),
          sizeof(kPerfInfoDPCPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x000002AB91B1C134);
  expected->AddField<UIntValue>("Routine", 0x900CEB1D);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("DPC", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoDPCV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoDPCOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoDPCPayloadV2[0]),
          sizeof(kPerfInfoDPCPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 4838955609293ULL);
  expected->AddField<ULongValue>("Routine", 18446735279572565220ULL);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("DPC", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoTimerDPC32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoTimerDPCOpcode, k32bit,
          reinterpret_cast<const char*>(&kPerfInfoTimerDPCPayload32bitsV2[0]),
          sizeof(kPerfInfoTimerDPCPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x000002AB91B13BC3);
  expected->AddField<UIntValue>("Routine", 0x93FE27B0);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("TimerDPC", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoTimerDPCV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoTimerDPCOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoTimerDPCPayloadV2[0]),
          sizeof(kPerfInfoTimerDPCPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0x00000466A83C2475);
  expected->AddField<ULongValue>("Routine", 0xFFFFF800031104D8);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("TimerDPC", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionStart32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoCollectionStartOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPerfInfoCollectionStartPayload32bitsV2[0]),
          sizeof(kPerfInfoCollectionStartPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("Source", 0U);
  expected->AddField<UIntValue>("NewInterval", 10000U);
  expected->AddField<UIntValue>("OldInterval", 10000U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion3, kPerfInfoCollectionStartOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kPerfInfoCollectionStartPayloadV3[0]),
          sizeof(kPerfInfoCollectionStartPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("Source", 0U);
  expected->AddField<UIntValue>("NewInterval", 10000U);
  expected->AddField<UIntValue>("OldInterval", 10000U);
  expected->AddField<WStringValue>("SourceName", L"Timer");

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionEnd32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion2, kPerfInfoCollectionEndOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPerfInfoCollectionEndPayload32bitsV2[0]),
          sizeof(kPerfInfoCollectionEndPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("Source", 0U);
  expected->AddField<UIntValue>("NewInterval", 10000U);
  expected->AddField<UIntValue>("OldInterval", 10000U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion3, kPerfInfoCollectionEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kPerfInfoCollectionEndPayloadV3[0]),
          sizeof(kPerfInfoCollectionEndPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("Source", 0U);
  expected->AddField<UIntValue>("NewInterval", 10000U);
  expected->AddField<UIntValue>("OldInterval", 10000U);
  expected->AddField<WStringValue>("SourceName", L"Timer");

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionStartSecondV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion3, kPerfInfoCollectionStartSecondOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kPerfInfoCollectionStartSecondPayloadV3[0]),
          sizeof(kPerfInfoCollectionStartSecondPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("SpinLockSpinThreshold", 1U);
  expected->AddField<UIntValue>("SpinLockContentionSampleRate", 1U);
  expected->AddField<UIntValue>("SpinLockAcquireSampleRate", 1000U);
  expected->AddField<UIntValue>("SpinLockHoldThreshold", 0U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PerfInfoCollectionEndSecondV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPerfInfoProviderId,
          kVersion3, kPerfInfoCollectionEndSecondOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kPerfInfoCollectionEndSecondPayloadV3[0]),
          sizeof(kPerfInfoCollectionEndSecondPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("SpinLockSpinThreshold", 1U);
  expected->AddField<UIntValue>("SpinLockContentionSampleRate", 1U);
  expected->AddField<UIntValue>("SpinLockAcquireSampleRate", 1000U);
  expected->AddField<UIntValue>("SpinLockHoldThreshold", 0U);

  EXPECT_STREQ("PerfInfo", category.c_str());
  EXPECT_STREQ("CollectionEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadStart32bitsV1) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion1, kThreadStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kThreadStartPayload32bitsV1[0]),
          sizeof(kThreadStartPayload32bitsV1),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 4);
  expected->AddField<UIntValue>("TThreadId", 1868);
  expected->AddField<UIntValue>("StackBase", 4088881152);
  expected->AddField<UIntValue>("StackLimit", 4088868864);
  expected->AddField<UIntValue>("UserStackBase", 0);
  expected->AddField<UIntValue>("UserStackLimit", 0);
  expected->AddField<UIntValue>("StartAddr", 4145994629);
  expected->AddField<UIntValue>("Win32StartAddr", 0);
  expected->AddField<CharValue>("WaitMode", -1);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadStart32bitsV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadStartOpcode, k32bit,
          reinterpret_cast<const char*>(&kThreadStartPayload32bitsV3[0]),
          sizeof(kThreadStartPayload32bitsV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 556);
  expected->AddField<UIntValue>("TThreadId", 4908);
  expected->AddField<UIntValue>("StackBase", 0xb1985000);
  expected->AddField<UIntValue>("StackLimit", 0xb1982000);
  expected->AddField<UIntValue>("UserStackBase", 0x00d50000);
  expected->AddField<UIntValue>("UserStackLimit", 0x00d4c000);
  expected->AddField<UIntValue>("Affinity", 3);
  expected->AddField<UIntValue>("Win32StartAddr", 0x77ab03e9);
  expected->AddField<UIntValue>("TebBase", 0x7ffde000);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 9);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 2);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadStartPayloadV3[0]),
          sizeof(kThreadStartPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 8568U);
  expected->AddField<UIntValue>("TThreadId", 5268U);
  expected->AddField<ULongValue>("StackBase", 18446691297806659584ULL);
  expected->AddField<ULongValue>("StackLimit", 18446691297806635008ULL);
  expected->AddField<ULongValue>("UserStackBase", 101449008ULL);
  expected->AddField<ULongValue>("UserStackLimit", 101416960ULL);
  expected->AddField<ULongValue>("Affinity", 255ULL);
  expected->AddField<ULongValue>("Win32StartAddr", 1549335852ULL);
  expected->AddField<ULongValue>("TebBase", 4279418880ULL);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 8);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 2);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("Start", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadEnd32bitsV1) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion1, kThreadEndOpcode, k32bit,
          reinterpret_cast<const char*>(&kThreadEndPayload32bitsV1[0]),
          sizeof(kThreadEndPayload32bitsV1),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0x00000004);
  expected->AddField<UIntValue>("TThreadId", 0x000000B4);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("End", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadEnd32bitsV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadEndOpcode, k32bit,
          reinterpret_cast<const char*>(&kThreadEndPayload32bitsV3[0]),
          sizeof(kThreadEndPayload32bitsV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0x000012c4);
  expected->AddField<UIntValue>("TThreadId", 0x00001364);
  expected->AddField<UIntValue>("StackBase", 0xaa555000);
  expected->AddField<UIntValue>("StackLimit", 0xaa552000);
  expected->AddField<UIntValue>("UserStackBase", 0x009c0000);
  expected->AddField<UIntValue>("UserStackLimit", 0x009be000);
  expected->AddField<UIntValue>("Affinity", 3);
  expected->AddField<UIntValue>("Win32StartAddr", 0x77ab03e9);
  expected->AddField<UIntValue>("TebBase", 0x7ffdd000);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 8);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 2);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("End", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadEndPayloadV3[0]),
          sizeof(kThreadEndPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 2040U);
  expected->AddField<UIntValue>("TThreadId", 3288U);
  expected->AddField<ULongValue>("StackBase", 18446691297848487936ULL);
  expected->AddField<ULongValue>("StackLimit", 18446691297848463360ULL);
  expected->AddField<ULongValue>("UserStackBase", 903052263424ULL);
  expected->AddField<ULongValue>("UserStackLimit", 903052255232ULL);
  expected->AddField<ULongValue>("Affinity", 255ULL);
  expected->AddField<ULongValue>("Win32StartAddr", 140723235226928ULL);
  expected->AddField<ULongValue>("TebBase", 140699801714688ULL);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 8);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 2);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("End", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadDCStartV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadDCStartPayloadV2[0]),
          sizeof(kThreadDCStartPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("TThreadId", 0U);
  expected->AddField<ULongValue>("StackBase", 18446735277666164736ULL);
  expected->AddField<ULongValue>("StackLimit", 18446735277666140160ULL);
  expected->AddField<ULongValue>("UserStackBase", 0ULL);
  expected->AddField<ULongValue>("UserStackLimit", 0ULL);
  expected->AddField<ULongValue>("StartAddr", 18446735277646357888ULL);
  expected->AddField<ULongValue>("Win32StartAddr", 18446735277646357888ULL);
  expected->AddField<ULongValue>("TebBase", 0ULL);
  expected->AddField<UIntValue>("SubProcessTag", 0U);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadDCStartV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadDCStartOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadDCStartPayloadV3[0]),
          sizeof(kThreadDCStartPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("TThreadId", 0U);
  expected->AddField<ULongValue>("StackBase", 18446735279600988160ULL);
  expected->AddField<ULongValue>("StackLimit", 18446735279600963584ULL);
  expected->AddField<ULongValue>("UserStackBase", 0ULL);
  expected->AddField<ULongValue>("UserStackLimit", 0ULL);
  expected->AddField<ULongValue>("Affinity", 1ULL);
  expected->AddField<ULongValue>("Win32StartAddr", 18446735279572912016ULL);
  expected->AddField<ULongValue>("TebBase", 0ULL);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 0);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 0);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("DCStart", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadDCEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadDCEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadDCEndPayloadV3[0]),
          sizeof(kThreadDCEndPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ProcessId", 0U);
  expected->AddField<UIntValue>("TThreadId", 0U);
  expected->AddField<ULongValue>("StackBase", 18446735279600988160ULL);
  expected->AddField<ULongValue>("StackLimit", 18446735279600963584ULL);
  expected->AddField<ULongValue>("UserStackBase", 0ULL);
  expected->AddField<ULongValue>("UserStackLimit", 0ULL);
  expected->AddField<ULongValue>("Affinity", 1ULL);
  expected->AddField<ULongValue>("Win32StartAddr", 18446735279572912016ULL);
  expected->AddField<ULongValue>("TebBase", 0ULL);
  expected->AddField<UIntValue>("SubProcessTag", 0U);
  expected->AddField<UCharValue>("BasePriority", 0);
  expected->AddField<UCharValue>("PagePriority", 5);
  expected->AddField<UCharValue>("IoPriority", 0);
  expected->AddField<UCharValue>("ThreadFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("DCEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadCSwitch32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadCSwitchOpcode, k32bit,
          reinterpret_cast<const char*>(&kThreadCSwitchPayload32bitsV2[0]),
          sizeof(kThreadCSwitchPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("NewThreadId", 0);
  expected->AddField<UIntValue>("OldThreadId", 4396);
  expected->AddField<CharValue>("NewThreadPriority", 0);
  expected->AddField<CharValue>("OldThreadPriority", 9);
  expected->AddField<UCharValue>("PreviousCState", 0);
  expected->AddField<CharValue>("SpareByte", 0);
  expected->AddField<CharValue>("OldThreadWaitReason", 23);
  expected->AddField<CharValue>("OldThreadWaitMode", 0);
  expected->AddField<CharValue>("OldThreadState", 1);
  expected->AddField<CharValue>("OldThreadWaitIdealProcessor", 0);
  expected->AddField<UIntValue>("NewThreadWaitTime", 18);
  expected->AddField<UIntValue>("Reserved", 18470);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("CSwitch", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadCSwitchV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadCSwitchOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadCSwitchPayloadV2[0]),
          sizeof(kThreadCSwitchPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("NewThreadId", 2252U);
  expected->AddField<UIntValue>("OldThreadId", 0U);
  expected->AddField<CharValue>("NewThreadPriority", 8);
  expected->AddField<CharValue>("OldThreadPriority", 0);
  expected->AddField<UCharValue>("PreviousCState", 1);
  expected->AddField<CharValue>("SpareByte", 0);
  expected->AddField<CharValue>("OldThreadWaitReason", 0);
  expected->AddField<CharValue>("OldThreadWaitMode", 0);
  expected->AddField<CharValue>("OldThreadState", 2);
  expected->AddField<CharValue>("OldThreadWaitIdealProcessor", 4);
  expected->AddField<UIntValue>("NewThreadWaitTime", 1U);
  expected->AddField<UIntValue>("Reserved", 881356167U);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("CSwitch", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadSpinLockV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadSpinLockOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadSpinLockPayloadV2[0]),
          sizeof(kThreadSpinLockPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("SpinLockAddress", 18446708889382682976ULL);
  expected->AddField<ULongValue>("CallerAddress", 18446735279573042192ULL);
  expected->AddField<ULongValue>("AcquireTime", 2104105494612894ULL);
  expected->AddField<ULongValue>("ReleaseTime", 2104105494613543ULL);
  expected->AddField<UIntValue>("WaitTimeInCycles", 1681U);
  expected->AddField<UIntValue>("SpinCount", 11U);
  expected->AddField<UIntValue>("ThreadId", 0U);
  expected->AddField<UIntValue>("InterruptCount", 0U);
  expected->AddField<UCharValue>("Irql", 0);
  expected->AddField<UCharValue>("AcquireDepth", 1);
  expected->AddField<UCharValue>("Flag", 0);

  std::unique_ptr<ArrayValue> reserved_array(new ArrayValue());
  for (int i = 0; i < 5; ++i)
    reserved_array->Append<UCharValue>(0);
  expected->AddField("Reserved", std::move(reserved_array));

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("SpinLock", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadSetPriorityV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadSetPriorityOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadSetPriorityPayloadV3[0]),
          sizeof(kThreadSetPriorityPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ThreadId", 544U);
  expected->AddField<UCharValue>("OldPriority", 15);
  expected->AddField<UCharValue>("NewPriority", 16);
  expected->AddField<UShortValue>("Reserved", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("SetPriority", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadSetBasePriorityV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadSetBasePriorityOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadSetBasePriorityPayloadV3[0]),
          sizeof(kThreadSetBasePriorityPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ThreadId", 6896U);
  expected->AddField<UCharValue>("OldPriority", 4);
  expected->AddField<UCharValue>("NewPriority", 7);
  expected->AddField<UShortValue>("Reserved", 7);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("SetBasePriority", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadReadyThreadV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadReadyThreadOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadReadyThreadPayloadV2[0]),
          sizeof(kThreadReadyThreadPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("TThreadId", 2252U);
  expected->AddField<CharValue>("AdjustReason", 1);
  expected->AddField<CharValue>("AdjustIncrement", 0);
  expected->AddField<CharValue>("Flag", 1);
  expected->AddField<CharValue>("Reserved", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("ReadyThread", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadSetPagePriorityV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadSetPagePriorityOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadSetPagePriorityPayloadV3[0]),
          sizeof(kThreadSetPagePriorityPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ThreadId", 6764U);
  expected->AddField<UCharValue>("OldPriority", 5);
  expected->AddField<UCharValue>("NewPriority", 6);
  expected->AddField<UShortValue>("Reserved", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("SetPagePriority", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadSetIoPriorityV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion3, kThreadSetIoPriorityOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadSetIoPriorityPayloadV3[0]),
          sizeof(kThreadSetIoPriorityPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("ThreadId", 188U);
  expected->AddField<UCharValue>("OldPriority", 2);
  expected->AddField<UCharValue>("NewPriority", 0);
  expected->AddField<UShortValue>("Reserved", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("SetIoPriority", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadAutoBoostSetFloorV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadAutoBoostSetFloorOpcode, k64bit,
          reinterpret_cast<const char*>(&kThreadAutoBoostSetFloorPayloadV2[0]),
          sizeof(kThreadAutoBoostSetFloorPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Lock", 18446708889355637112ULL);
  expected->AddField<UIntValue>("ThreadId", 6896U);
  expected->AddField<UCharValue>("NewCpuPriorityFloor", 11);
  expected->AddField<UCharValue>("OldCpuPriority", 7);
  expected->AddField<UCharValue>("IoPriorities", 32);
  expected->AddField<UCharValue>("BoostFlags", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("AutoBoostSetFloor", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadAutoBoostClearFloorV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadAutoBoostClearFloorOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kThreadAutoBoostClearFloorPayloadV2[0]),
          sizeof(kThreadAutoBoostClearFloorPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("LockAddress", 18446708889355637112ULL);
  expected->AddField<UIntValue>("ThreadId", 6896U);
  expected->AddField<UShortValue>("BoostBitmap", 2048);
  expected->AddField<UShortValue>("Reserved", 0);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("AutoBoostClearFloor", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, ThreadAutoBoostEntryExhaustionV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kThreadProviderId,
          kVersion2, kThreadAutoBoostEntryExhaustionOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kThreadAutoBoostEntryExhaustionPayloadV2[0]),
          sizeof(kThreadAutoBoostEntryExhaustionPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("LockAddress", 18446708889482441968ULL);
  expected->AddField<UIntValue>("ThreadId", 3004U);

  EXPECT_STREQ("Thread", category.c_str());
  EXPECT_STREQ("AutoBoostEntryExhaustion", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpSendIPV432bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpSendIPV4Opcode, k32bit,
          reinterpret_cast<const char*>(&kTcplpSendIPV4Payload32bitsV2[0]),
          sizeof(kTcplpSendIPV4Payload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 3768);
  expected->AddField<UIntValue>("size", 516);
  expected->AddField<UIntValue>("daddr", 420152384);
  expected->AddField<UIntValue>("saddr", 2064391596);
  expected->AddField<UShortValue>("dport", 20480);
  expected->AddField<UShortValue>("sport", 23037);
  expected->AddField<UIntValue>("startime", 12557505);
  expected->AddField<UIntValue>("endtime", 12557505);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<UIntValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("SendIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpSendIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpSendIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpSendIPV4PayloadV2[0]),
          sizeof(kTcplpSendIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 8500U);
  expected->AddField<UIntValue>("size", 26U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UIntValue>("startime", 3483307U);
  expected->AddField<UIntValue>("endtime", 3483307U);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("SendIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpTCPCopyIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpTCPCopyIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpTCPCopyIPV4PayloadV2[0]),
          sizeof(kTcplpTCPCopyIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 6784U);
  expected->AddField<UIntValue>("size", 85U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("TCPCopyIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpRecvIPV432bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpRecvIPV4Opcode, k32bit,
          reinterpret_cast<const char*>(&kTcplpRecvIPV4Payload32bitsV2[0]),
          sizeof(kTcplpRecvIPV4Payload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 3768);
  expected->AddField<UIntValue>("size", 450);
  expected->AddField<UIntValue>("daddr", 420152384);
  expected->AddField<UIntValue>("saddr", 2064391596);
  expected->AddField<UShortValue>("dport", 20480);
  expected->AddField<UShortValue>("sport", 23037);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<UIntValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("RecvIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpRecvIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpRecvIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpRecvIPV4PayloadV2[0]),
          sizeof(kTcplpRecvIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 6784U);
  expected->AddField<UIntValue>("size", 85U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("RecvIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpConnectIPV432bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpConnectIPV4Opcode, k32bit,
          reinterpret_cast<const char*>(&kTcplpConnectIPV4Payload32bitsV2[0]),
          sizeof(kTcplpConnectIPV4Payload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 3768);
  expected->AddField<UIntValue>("size", 0U);
  expected->AddField<UIntValue>("daddr", 353238403);
  expected->AddField<UIntValue>("saddr", 2064391596);
  expected->AddField<UShortValue>("dport", 20480);
  expected->AddField<UShortValue>("sport", 23293);
  expected->AddField<UShortValue>("mss", 1440);
  expected->AddField<UShortValue>("sackopt", 1);
  expected->AddField<UShortValue>("tsopt", 0);
  expected->AddField<UShortValue>("wsopt", 1);
  expected->AddField<UIntValue>("rcvwin", 66240);
  expected->AddField<ShortValue>("rcvwinscale", 8);
  expected->AddField<ShortValue>("sndwinscale", 8);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<UIntValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("ConnectIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpConnectIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpConnectIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpConnectIPV4PayloadV2[0]),
          sizeof(kTcplpConnectIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 6784U);
  expected->AddField<UIntValue>("size", 0U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UShortValue>("mss", 1430);
  expected->AddField<UShortValue>("sackopt", 1);
  expected->AddField<UShortValue>("tsopt", 0);
  expected->AddField<UShortValue>("wsopt", 1);
  expected->AddField<UIntValue>("rcvwin", 65780U);
  expected->AddField<ShortValue>("rcvwinscale", 8);
  expected->AddField<ShortValue>("sndwinscale", 6);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("ConnectIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpDisconnectIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpDisconnectIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpDisconnectIPV4PayloadV2[0]),
          sizeof(kTcplpDisconnectIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 6784U);
  expected->AddField<UIntValue>("size", 0U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("DisconnectIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, TcplpRetransmitIPV4V2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kTcplpProviderId,
          kVersion2, kTcplpRetransmitIPV4Opcode, k64bit,
          reinterpret_cast<const char*>(&kTcplpRetransmitIPV4PayloadV2[0]),
          sizeof(kTcplpRetransmitIPV4PayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("PID", 6784U);
  expected->AddField<UIntValue>("size", 0U);
  expected->AddField<UIntValue>("daddr", 2U);
  expected->AddField<UIntValue>("saddr", 3U);
  expected->AddField<UShortValue>("dport", 8);
  expected->AddField<UShortValue>("sport", 9);
  expected->AddField<UIntValue>("seqnum", 0U);
  expected->AddField<ULongValue>("connid", 0ULL);

  EXPECT_STREQ("Tcplp", category.c_str());
  EXPECT_STREQ("RetransmitIPV4", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryCounters32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryCountersOpcode, k32bit,
          reinterpret_cast<const char*>(&kRegistryCountersPayload32bitsV2[0]),
          sizeof(kRegistryCountersPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Counter1", 3444ULL);
  expected->AddField<ULongValue>("Counter2", 1558ULL);
  expected->AddField<ULongValue>("Counter3", 343ULL);
  expected->AddField<ULongValue>("Counter4", 5131ULL);
  expected->AddField<ULongValue>("Counter5", 3444ULL);
  expected->AddField<ULongValue>("Counter6", 7150820ULL);
  expected->AddField<ULongValue>("Counter7", 850068ULL);
  expected->AddField<ULongValue>("Counter8", 1298338ULL);
  expected->AddField<ULongValue>("Counter9", 0ULL);
  expected->AddField<ULongValue>("Counter10", 0ULL);
  expected->AddField<ULongValue>("Counter11", 0ULL);

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Counters", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryCountersV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryCountersOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryCountersPayloadV2[0]),
          sizeof(kRegistryCountersPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Counter1", 4774ULL);
  expected->AddField<ULongValue>("Counter2", 2043ULL);
  expected->AddField<ULongValue>("Counter3", 631ULL);
  expected->AddField<ULongValue>("Counter4", 3429ULL);
  expected->AddField<ULongValue>("Counter5", 4774ULL);
  expected->AddField<ULongValue>("Counter6", 44167160ULL);
  expected->AddField<ULongValue>("Counter7", 7830828ULL);
  expected->AddField<ULongValue>("Counter8", 3438528ULL);
  expected->AddField<ULongValue>("Counter9", 0ULL);
  expected->AddField<ULongValue>("Counter10", 0ULL);
  expected->AddField<ULongValue>("Counter11", 0ULL);

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Counters", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryCloseV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryCloseOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryClosePayloadV2[0]),
          sizeof(kRegistryClosePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156575559766LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673704982924480ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Close", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryOpen32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryOpenOpcode, k32bit,
          reinterpret_cast<const char*>(&kRegistryOpenPayload32bitsV2[0]),
          sizeof(kRegistryOpenPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 2935907034356LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<UIntValue>("KeyHandle", 0ULL);
  expected->AddField<WStringValue>("KeyName",
      L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\"
      L"CurrentVersion\\GRE_Initialize");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Open", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryOpenV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryOpenOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryOpenPayloadV2[0]),
          sizeof(kRegistryOpenPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156575563809LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 0ULL);
  expected->AddField<WStringValue>("KeyName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized st");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Open", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryQueryValueV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryQueryValueOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryQueryValuePayloadV2[0]),
          sizeof(kRegistryQueryValuePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156575563864LL);
  expected->AddField<UIntValue>("Status", 3221225524U);
  expected->AddField<UIntValue>("Index", 2U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705101222488ULL);
  expected->AddField<WStringValue>("KeyName", L"Anonymized strin");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("QueryValue", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryQueryV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryQueryOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryQueryPayloadV2[0]),
          sizeof(kRegistryQueryPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156576149040LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 7U);
  expected->AddField<ULongValue>("KeyHandle", 18446673704987402840ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Query", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryKCBDeleteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryKCBDeleteOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryKCBDeletePayloadV2[0]),
          sizeof(kRegistryKCBDeletePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 0LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705265649400ULL);
  expected->AddField<WStringValue>("KeyName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content. False value. Fake cha");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("KCBDelete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryKCBCreate32bitsV1) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion1, kRegistryKCBCreateOpcode, k32bit,
          reinterpret_cast<const char*>(&kRegistryKCBCreatePayload32bitsV1[0]),
          sizeof(kRegistryKCBCreatePayload32bitsV1),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("KeyHandle", 3814704792);
  expected->AddField<LongValue>("ElapsedTime", 0LL);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<WStringValue>("KeyName",
      L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Enum\\PCI\\"
      L"VEN_8086&DEV_2C22&SUBSYS_00000000&REV_05\\3&36cb97a3&0&22");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("KCBCreate", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryKCBCreateV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryKCBCreateOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryKCBCreatePayloadV2[0]),
          sizeof(kRegistryKCBCreatePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 0LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705105261736ULL);
  expected->AddField<WStringValue>("KeyName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content. Fa");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("KCBCreate", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistrySetInformationV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistrySetInformationOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistrySetInformationPayloadV2[0]),
          sizeof(kRegistrySetInformationPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156576862229LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705105261736ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("SetInformation", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryEnumerateValueKeyV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryEnumerateValueKeyOpcode, k64bit,
          reinterpret_cast<const char*>(
              &kRegistryEnumerateValueKeyPayloadV2[0]),
          sizeof(kRegistryEnumerateValueKeyPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156576862359LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705105261736ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("EnumerateValueKey", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryEnumerateKeyV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryEnumerateKeyOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryEnumerateKeyPayloadV2[0]),
          sizeof(kRegistryEnumerateKeyPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156576863273LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705105261736ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("EnumerateKey", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistrySetValue32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistrySetValueOpcode, k32bit,
          reinterpret_cast<const char*>(&kRegistrySetValuePayload32bitsV2[0]),
          sizeof(kRegistrySetValuePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 2935917025169LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<UIntValue>("KeyHandle", 2573103112);
  expected->AddField<WStringValue>("KeyName",
      L"{Q65231O0-O2S1-4857-N4PR-N8R7P6RN7Q27}\\pzq.rkr");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("SetValue", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistrySetValueV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistrySetValueOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistrySetValuePayloadV2[0]),
          sizeof(kRegistrySetValuePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156580683338LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705117816864ULL);
  expected->AddField<WStringValue>("KeyName",
      L"Anonymized string. Dummy content. False value.");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("SetValue", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryCreate32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryCreateOpcode, k32bit,
          reinterpret_cast<const char*>(&kRegistryCreatePayload32bitsV2[0]),
          sizeof(kRegistryCreatePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 2935928835756LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<UIntValue>("KeyHandle", 2354816104);
  expected->AddField<WStringValue>("KeyName",
      L"Software\\Microsoft\\Internet Explorer\\SQM");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Create", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryCreateV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryCreateOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryCreatePayloadV2[0]),
          sizeof(kRegistryCreatePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156580973646LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705024425152ULL);
  expected->AddField<WStringValue>("KeyName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymi");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Create", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryQuerySecurityV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryQuerySecurityOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryQuerySecurityPayloadV2[0]),
          sizeof(kRegistryQuerySecurityPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156608798503LL);
  expected->AddField<UIntValue>("Status", 3221225507U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705265383160ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("QuerySecurity", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistrySetSecurityV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistrySetSecurityOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistrySetSecurityPayloadV2[0]),
          sizeof(kRegistrySetSecurityPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 1156608798701LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673705265666080ULL);
  expected->AddField<WStringValue>("KeyName", L"");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("SetSecurity", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryKCBRundownEndV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryKCBRundownEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryKCBRundownEndPayloadV2[0]),
          sizeof(kRegistryKCBRundownEndPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<LongValue>("InitialTime", 0LL);
  expected->AddField<UIntValue>("Status", 0U);
  expected->AddField<UIntValue>("Index", 0U);
  expected->AddField<ULongValue>("KeyHandle", 18446673704965529608ULL);
  expected->AddField<WStringValue>("KeyName", L"Anonymize");

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("KCBRundownEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, RegistryConfigV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kRegistryProviderId,
          kVersion2, kRegistryConfigOpcode, k64bit,
          reinterpret_cast<const char*>(&kRegistryConfigPayloadV2[0]),
          sizeof(kRegistryConfigPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("CurrentControlSet", 1U);

  EXPECT_STREQ("Registry", category.c_str());
  EXPECT_STREQ("Config", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileCreate32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileCreateOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOFileCreatePayload32bitsV2[0]),
          sizeof(kFileIOFileCreatePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("FileObject", 2928799992U);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized s");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileCreate", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileCreateV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileCreateOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFileCreatePayloadV2[0]),
          sizeof(kFileIOFileCreatePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("FileObject", 18446673705054964784ULL);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anony");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileCreate", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileDelete32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileDeleteOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOFileDeletePayload32bitsV2[0]),
          sizeof(kFileIOFileDeletePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("FileObject", 2978713848U);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content.");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileDelete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileDeleteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileDeleteOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFileDeletePayloadV2[0]),
          sizeof(kFileIOFileDeletePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("FileObject", 18446673705333632048ULL);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized str");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileDelete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileRundown32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileRundownOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOFileRundownPayload32bitsV2[0]),
          sizeof(kFileIOFileRundownPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("FileObject", 2310563480U);
  expected->AddField<WStringValue>("FileName", L"Anonymized string. Dummy");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileRundown", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFileRundownV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFileRundownOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFileRundownPayloadV2[0]),
          sizeof(kFileIOFileRundownPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("FileObject", 18446673704981525952ULL);
  expected->AddField<WStringValue>("FileName", L"Anonymized string. Dummy");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FileRundown", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCreateV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCreateOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOCreatePayloadV2[0]),
          sizeof(kFileIOCreatePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026435767392ULL);
  expected->AddField<ULongValue>("TTID", 1592ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026464273584ULL);
  expected->AddField<UIntValue>("CreateOptions", 16777312U);
  expected->AddField<UIntValue>("FileAttributes", 0U);
  expected->AddField<UIntValue>("ShareAccess", 1U);
  expected->AddField<WStringValue>(
      "OpenPath",
      L"Anonymized string. Dummy content. False value. Fake characters");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Create", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCreate32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCreateOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOCreatePayload32bitsV2[0]),
          sizeof(kFileIOCreatePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229521984U);
  expected->AddField<UIntValue>("TTID", 2612U);
  expected->AddField<UIntValue>("FileObject", 2228830616U);
  expected->AddField<UIntValue>("CreateOptions", 18874368U);
  expected->AddField<UIntValue>("FileAttributes", 0U);
  expected->AddField<UIntValue>("ShareAccess", 7U);
  expected->AddField<WStringValue>(
      "OpenPath",
      L"Anonymized string. Dummy content. False value. Fake");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Create", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCreateV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOCreateOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOCreatePayloadV3[0]),
          sizeof(kFileIOCreatePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889463167384ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889421029152ULL);
  expected->AddField<UIntValue>("TTID", 6592U);
  expected->AddField<UIntValue>("CreateOptions", 16908384U);
  expected->AddField<UIntValue>("FileAttributes", 128U);
  expected->AddField<UIntValue>("ShareAccess", 3U);
  expected->AddField<WStringValue>(
      "OpenPath",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized st");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Create", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCleanupV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCleanupOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOCleanupPayloadV2[0]),
          sizeof(kFileIOCleanupPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026421882464ULL);
  expected->AddField<ULongValue>("TTID", 2844ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026463889744ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964834310304ULL);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Cleanup", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCleanup32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCleanupOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOCleanupPayload32bitsV2[0]),
          sizeof(kFileIOCleanupPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229521984U);
  expected->AddField<UIntValue>("TTID", 2612U);
  expected->AddField<UIntValue>("FileObject", 2228830616U);
  expected->AddField<UIntValue>("FileKey", 2978882848U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Cleanup", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCleanupV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOCleanupOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOCleanupPayloadV3[0]),
          sizeof(kFileIOCleanupPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889441474104ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889468267536ULL);
  expected->AddField<ULongValue>("FileKey", 18446673704999469856ULL);
  expected->AddField<UIntValue>("TTID", 3480U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Cleanup", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCloseV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCloseOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOClosePayloadV2[0]),
          sizeof(kFileIOClosePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026421882464ULL);
  expected->AddField<ULongValue>("TTID", 2844ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026463889744ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964834310304ULL);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Close", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOClose32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOCloseOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOClosePayload32bitsV2[0]),
          sizeof(kFileIOClosePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229521984U);
  expected->AddField<UIntValue>("TTID", 2612U);
  expected->AddField<UIntValue>("FileObject", 2228830616U);
  expected->AddField<UIntValue>("FileKey", 2978882848U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Close", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOCloseV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOCloseOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOClosePayloadV3[0]),
          sizeof(kFileIOClosePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889441474104ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889468267536ULL);
  expected->AddField<ULongValue>("FileKey", 18446673704999469856ULL);
  expected->AddField<UIntValue>("TTID", 3480U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Close", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOReadV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOReadOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOReadPayloadV2[0]),
          sizeof(kFileIOReadPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 258ULL);
  expected->AddField<ULongValue>("IrpPtr", 18446738026430539952ULL);
  expected->AddField<ULongValue>("TTID", 3580ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026463889744ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964915212608ULL);
  expected->AddField<UIntValue>("IoSize", 8191U);
  expected->AddField<UIntValue>("IoFlags", 395520U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Read", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIORead32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOReadOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOReadPayload32bitsV2[0]),
          sizeof(kFileIOReadPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 9984ULL);
  expected->AddField<UIntValue>("IrpPtr", 2228365648U);
  expected->AddField<UIntValue>("TTID", 2924U);
  expected->AddField<UIntValue>("FileObject", 2229119216U);
  expected->AddField<UIntValue>("FileKey", 2719720864U);
  expected->AddField<UIntValue>("IoSize", 256U);
  expected->AddField<UIntValue>("IoFlags", 0U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Read", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOReadV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOReadOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOReadPayloadV3[0]),
          sizeof(kFileIOReadPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 736ULL);
  expected->AddField<ULongValue>("IrpPtr", 18446708889463167384ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889421029152ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705375292464ULL);
  expected->AddField<UIntValue>("TTID", 6592U);
  expected->AddField<UIntValue>("IoSize", 8191U);
  expected->AddField<UIntValue>("IoFlags", 395520U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Read", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOWriteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOWriteOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOWritePayloadV2[0]),
          sizeof(kFileIOWritePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 0ULL);
  expected->AddField<ULongValue>("IrpPtr", 18446738026421882464ULL);
  expected->AddField<ULongValue>("TTID", 1592ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026464273584ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964923425088ULL);
  expected->AddField<UIntValue>("IoSize", 331074U);
  expected->AddField<UIntValue>("IoFlags", 395776U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Write", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOWrite32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOWriteOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOWritePayload32bitsV2[0]),
          sizeof(kFileIOWritePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 225956ULL);
  expected->AddField<UIntValue>("IrpPtr", 2230303248U);
  expected->AddField<UIntValue>("TTID", 2924U);
  expected->AddField<UIntValue>("FileObject", 2228936920U);
  expected->AddField<UIntValue>("FileKey", 2677720112U);
  expected->AddField<UIntValue>("IoSize", 36U);
  expected->AddField<UIntValue>("IoFlags", 0U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Write", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOWriteV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOWriteOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOWritePayloadV3[0]),
          sizeof(kFileIOWritePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Offset", 0ULL);
  expected->AddField<ULongValue>("IrpPtr", 18446708889468543848ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889442318784ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705429320000ULL);
  expected->AddField<UIntValue>("TTID", 1804U);
  expected->AddField<UIntValue>("IoSize", 722U);
  expected->AddField<UIntValue>("IoFlags", 395776U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Write", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOSetInfoV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOSetInfoOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOSetInfoPayloadV2[0]),
          sizeof(kFileIOSetInfoPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026421882464ULL);
  expected->AddField<ULongValue>("TTID", 4676ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026439430256ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964812580464ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("InfoClass", 4U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("SetInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOSetInfo32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOSetInfoOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOSetInfoPayload32bitsV2[0]),
          sizeof(kFileIOSetInfoPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229278008U);
  expected->AddField<UIntValue>("TTID", 716U);
  expected->AddField<UIntValue>("FileObject", 2245283192U);
  expected->AddField<UIntValue>("FileKey", 2327829880U);
  expected->AddField<UIntValue>("ExtraInfo", 524288U);
  expected->AddField<UIntValue>("InfoClass", 20U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("SetInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOSetInfoV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOSetInfoOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOSetInfoPayloadV3[0]),
          sizeof(kFileIOSetInfoPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889351416760ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889444373312ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705429320000ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 1708U);
  expected->AddField<UIntValue>("InfoClass", 4U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("SetInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODeleteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODeleteOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODeletePayloadV2[0]),
          sizeof(kFileIODeletePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026455966864ULL);
  expected->AddField<ULongValue>("TTID", 2524ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026430805520ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964915447104ULL);
  expected->AddField<ULongValue>("ExtraInfo", 1ULL);
  expected->AddField<UIntValue>("InfoClass", 13U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Delete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODelete32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODeleteOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIODeletePayload32bitsV2[0]),
          sizeof(kFileIODeletePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229278008U);
  expected->AddField<UIntValue>("TTID", 2924U);
  expected->AddField<UIntValue>("FileObject", 2245543696U);
  expected->AddField<UIntValue>("FileKey", 2978713848U);
  expected->AddField<UIntValue>("ExtraInfo", 1U);
  expected->AddField<UIntValue>("InfoClass", 13U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Delete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODeleteV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIODeleteOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODeletePayloadV3[0]),
          sizeof(kFileIODeletePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889352747960ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889505544320ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705429320000ULL);
  expected->AddField<ULongValue>("ExtraInfo", 1ULL);
  expected->AddField<UIntValue>("TTID", 1804U);
  expected->AddField<UIntValue>("InfoClass", 13U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Delete", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIORenameV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIORenameOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIORenamePayloadV2[0]),
          sizeof(kFileIORenamePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026435767392ULL);
  expected->AddField<ULongValue>("TTID", 404ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026444779632ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964927413360ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("InfoClass", 10U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Rename", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIORename32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIORenameOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIORenamePayload32bitsV2[0]),
          sizeof(kFileIORenamePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2230303248U);
  expected->AddField<UIntValue>("TTID", 3092U);
  expected->AddField<UIntValue>("FileObject", 2273110328U);
  expected->AddField<UIntValue>("FileKey", 2617259296U);
  expected->AddField<UIntValue>("ExtraInfo", 0U);
  expected->AddField<UIntValue>("InfoClass", 10U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Rename", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIORenameV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIORenameOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIORenamePayloadV3[0]),
          sizeof(kFileIORenamePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889463167384ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889442619504ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705292653728ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 7700U);
  expected->AddField<UIntValue>("InfoClass", 10U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Rename", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirEnumV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODirEnumOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODirEnumPayloadV2[0]),
          sizeof(kFileIODirEnumPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026429591744ULL);
  expected->AddField<ULongValue>("TTID", 2112ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026464819664ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964813193536ULL);
  expected->AddField<UIntValue>("Length", 632U);
  expected->AddField<UIntValue>("InfoClass", 37U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>("FileName", L"Anony");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirEnum", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirEnum32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODirEnumOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIODirEnumPayload32bitsV2[0]),
          sizeof(kFileIODirEnumPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2228365648U);
  expected->AddField<UIntValue>("TTID", 2612U);
  expected->AddField<UIntValue>("FileObject", 2228830616U);
  expected->AddField<UIntValue>("FileKey", 2978882848U);
  expected->AddField<UIntValue>("Length", 616U);
  expected->AddField<UIntValue>("InfoClass", 3U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. ");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirEnum", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirEnumV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIODirEnumOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODirEnumPayloadV3[0]),
          sizeof(kFileIODirEnumPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889354247384ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889434820384ULL);
  expected->AddField<ULongValue>("FileKey", 18446673704981525952ULL);
  expected->AddField<UIntValue>("TTID", 1856U);
  expected->AddField<UIntValue>("Length", 632U);
  expected->AddField<UIntValue>("InfoClass", 37U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>("FileName", L"Anony");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirEnum", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFlushV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFlushOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFlushPayloadV2[0]),
          sizeof(kFileIOFlushPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026421882464ULL);
  expected->AddField<ULongValue>("TTID", 48ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026421593136ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964820929296ULL);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Flush", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFlush32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFlushOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOFlushPayload32bitsV2[0]),
          sizeof(kFileIOFlushPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2261535752U);
  expected->AddField<UIntValue>("TTID", 2856U);
  expected->AddField<UIntValue>("FileObject", 2229003904U);
  expected->AddField<UIntValue>("FileKey", 2741681528U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Flush", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFlushV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOFlushOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFlushPayloadV3[0]),
          sizeof(kFileIOFlushPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889351396104ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889348433504ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705442971968ULL);
  expected->AddField<UIntValue>("TTID", 3436U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("Flush", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOQueryInfoV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOQueryInfoOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOQueryInfoPayloadV2[0]),
          sizeof(kFileIOQueryInfoPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026435767392ULL);
  expected->AddField<ULongValue>("TTID", 1592ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026464273584ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964923425088ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("InfoClass", 5U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("QueryInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOQueryInfo32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOQueryInfoOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOQueryInfoPayload32bitsV2[0]),
          sizeof(kFileIOQueryInfoPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229521984U);
  expected->AddField<UIntValue>("TTID", 2612U);
  expected->AddField<UIntValue>("FileObject", 2228830616U);
  expected->AddField<UIntValue>("FileKey", 2677009672U);
  expected->AddField<UIntValue>("ExtraInfo", 0U);
  expected->AddField<UIntValue>("InfoClass", 4U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("QueryInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOQueryInfoV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOQueryInfoOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOQueryInfoPayloadV3[0]),
          sizeof(kFileIOQueryInfoPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889441474104ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889382979552ULL);
  expected->AddField<ULongValue>("FileKey", 18446673704977933824ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 3480U);
  expected->AddField<UIntValue>("InfoClass", 9U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("QueryInfo", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFSControlV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFSControlOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFSControlPayloadV2[0]),
          sizeof(kFileIOFSControlPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026429591744ULL);
  expected->AddField<ULongValue>("TTID", 2404ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026458665072ULL);
  expected->AddField<ULongValue>("FileKey", 18446738026438512656ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("InfoClass", 590068U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FSControl", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFSControl32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOFSControlOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOFSControlPayload32bitsV2[0]),
          sizeof(kFileIOFSControlPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229521984U);
  expected->AddField<UIntValue>("TTID", 3816U);
  expected->AddField<UIntValue>("FileObject", 2272674216U);
  expected->AddField<UIntValue>("FileKey", 2242878872U);
  expected->AddField<UIntValue>("ExtraInfo", 0U);
  expected->AddField<UIntValue>("InfoClass", 590068U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FSControl", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOFSControlV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOFSControlOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOFSControlPayloadV3[0]),
          sizeof(kFileIOFSControlPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889356233944ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889414324000ULL);
  expected->AddField<ULongValue>("FileKey", 18446708889381955568ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 940U);
  expected->AddField<UIntValue>("InfoClass", 590011U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("FSControl", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOOperationEnd32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIOOperationEndOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIOOperationEndPayload32bitsV2[0]),
          sizeof(kFileIOOperationEndPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2228365648U);
  expected->AddField<UIntValue>("ExtraInfo", 224U);
  expected->AddField<UIntValue>("NtStatus", 0U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("OperationEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIOOperationEndV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIOOperationEndOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIOOperationEndPayloadV3[0]),
          sizeof(kFileIOOperationEndPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889441474104ULL);
  expected->AddField<ULongValue>("ExtraInfo", 58ULL);
  expected->AddField<UIntValue>("NtStatus", 0U);

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("OperationEnd", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirNotifyV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODirNotifyOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODirNotifyPayloadV2[0]),
          sizeof(kFileIODirNotifyPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446738026434152288ULL);
  expected->AddField<ULongValue>("TTID", 2112ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026432933664ULL);
  expected->AddField<ULongValue>("FileKey", 18446735964918094736ULL);
  expected->AddField<UIntValue>("Length", 2048U);
  expected->AddField<UIntValue>("InfoClass", 2U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>("FileName", L"");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirNotify", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirNotify32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion2, kFileIODirNotifyOpcode, k32bit,
          reinterpret_cast<const char*>(&kFileIODirNotifyPayload32bitsV2[0]),
          sizeof(kFileIODirNotifyPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("IrpPtr", 2229757472U);
  expected->AddField<UIntValue>("TTID", 5528U);
  expected->AddField<UIntValue>("FileObject", 2230090792U);
  expected->AddField<UIntValue>("FileKey", 2627465464U);
  expected->AddField<UIntValue>("Length", 32U);
  expected->AddField<UIntValue>("InfoClass", 27U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>("FileName", L"");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirNotify", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODirNotifyV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIODirNotifyOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODirNotifyPayloadV3[0]),
          sizeof(kFileIODirNotifyPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889360288168ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889436228640ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705003707264ULL);
  expected->AddField<UIntValue>("TTID", 188U);
  expected->AddField<UIntValue>("Length", 32U);
  expected->AddField<UIntValue>("InfoClass", 17U);
  expected->AddField<UIntValue>("FileIndex", 0U);
  expected->AddField<WStringValue>("FileName", L"");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DirNotify", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIODletePathV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIODletePathOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIODletePathPayloadV3[0]),
          sizeof(kFileIODletePathPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889352747960ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889505544320ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705429320000ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 1804U);
  expected->AddField<UIntValue>("InfoClass", 13U);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake char");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("DeletePath", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, FileIORenamePathV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kFileIOProviderId,
          kVersion3, kFileIORenamePathOpcode, k64bit,
          reinterpret_cast<const char*>(&kFileIORenamePathPayloadV3[0]),
          sizeof(kFileIORenamePathPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("IrpPtr", 18446708889354247384ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889420710640ULL);
  expected->AddField<ULongValue>("FileKey", 18446673705066228784ULL);
  expected->AddField<ULongValue>("ExtraInfo", 0ULL);
  expected->AddField<UIntValue>("TTID", 7700U);
  expected->AddField<UIntValue>("InfoClass", 10U);
  expected->AddField<WStringValue>(
      "FileName",
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized string. Dummy content. False value. Fake characters. "
      L"Anonymized str");

  EXPECT_STREQ("FileIO", category.c_str());
  EXPECT_STREQ("RenamePath", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOReadV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOReadOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOReadPayloadV2[0]),
          sizeof(kDiskIOReadPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 0U);
  expected->AddField<UIntValue>("IrpFlags", 393283U);
  expected->AddField<UIntValue>("TransferSize", 32768U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("ByteOffset", 1134870528ULL);
  expected->AddField<ULongValue>("FileObject", 18446735964947782768ULL);
  expected->AddField<ULongValue>("Irp", 18446738026433680656ULL);
  expected->AddField<ULongValue>("HighResResponseTime", 96928ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("Read", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOReadV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOReadOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOReadPayloadV3[0]),
          sizeof(kDiskIOReadPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 1U);
  expected->AddField<UIntValue>("IrpFlags", 393283U);
  expected->AddField<UIntValue>("TransferSize", 4096U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("ByteOffset", 1841837375488ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889442809920ULL);
  expected->AddField<ULongValue>("Irp", 18446708889436113680ULL);
  expected->AddField<ULongValue>("HighResResponseTime", 36525ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 7056U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("Read", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOWriteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOWriteOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOWritePayloadV2[0]),
          sizeof(kDiskIOWritePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 0U);
  expected->AddField<UIntValue>("IrpFlags", 393283U);
  expected->AddField<UIntValue>("TransferSize", 12800U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("ByteOffset", 108986368ULL);
  expected->AddField<ULongValue>("FileObject", 18446735964860446544ULL);
  expected->AddField<ULongValue>("Irp", 18446738026434317152ULL);
  expected->AddField<ULongValue>("HighResResponseTime", 969ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("Write", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOWriteV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOWriteOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOWritePayloadV3[0]),
          sizeof(kDiskIOWritePayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 0U);
  expected->AddField<UIntValue>("IrpFlags", 393283U);
  expected->AddField<UIntValue>("TransferSize", 8192U);
  expected->AddField<UIntValue>("Reserved", 0U);
  expected->AddField<ULongValue>("ByteOffset", 4120666112ULL);
  expected->AddField<ULongValue>("FileObject", 18446708889381719024ULL);
  expected->AddField<ULongValue>("Irp", 18446708889462370320ULL);
  expected->AddField<ULongValue>("HighResResponseTime", 429ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 6896U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("Write", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOReadInitV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOReadInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOReadInitPayloadV2[0]),
          sizeof(kDiskIOReadInitPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446738026433680656ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("ReadInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOReadInitV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOReadInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOReadInitPayloadV3[0]),
          sizeof(kDiskIOReadInitPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446708889436113680ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 7056U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("ReadInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOWriteInitV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOWriteInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOWriteInitPayloadV2[0]),
          sizeof(kDiskIOWriteInitPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446738026434317152ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("WriteInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOWriteInitV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOWriteInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOWriteInitPayloadV3[0]),
          sizeof(kDiskIOWriteInitPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446708889462370320ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 6896U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("WriteInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOFlushBuffersV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOFlushBuffersOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOFlushBuffersPayloadV2[0]),
          sizeof(kDiskIOFlushBuffersPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 0U);
  expected->AddField<UIntValue>("IrpFlags", 393216U);
  expected->AddField<ULongValue>("HighResResponseTime", 45238ULL);
  expected->AddField<ULongValue>("Irp", 18446738026432981120ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("FlushBuffers", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOFlushBuffersV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOFlushBuffersOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOFlushBuffersPayloadV3[0]),
          sizeof(kDiskIOFlushBuffersPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("DiskNumber", 0U);
  expected->AddField<UIntValue>("IrpFlags", 393216U);
  expected->AddField<ULongValue>("HighResResponseTime", 1881ULL);
  expected->AddField<ULongValue>("Irp", 18446708889460512592ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 6896U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("FlushBuffers", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOFlushInitV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion2, kDiskIOFlushInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOFlushInitPayloadV2[0]),
          sizeof(kDiskIOFlushInitPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446738026432981120ULL);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("FlushInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, DiskIOFlushInitV3) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kDiskIOProviderId,
          kVersion3, kDiskIOFlushInitOpcode, k64bit,
          reinterpret_cast<const char*>(&kDiskIOFlushInitPayloadV3[0]),
          sizeof(kDiskIOFlushInitPayloadV3),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("Irp", 18446708889460512592ULL);
  expected->AddField<UIntValue>("IssuingThreadId", 6896U);

  EXPECT_STREQ("DiskIO", category.c_str());
  EXPECT_STREQ("FlushInit", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, StackWalkStackV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kStackWalkProviderId,
          kVersion2, kStackWalkStackOpcode, k64bit,
          reinterpret_cast<const char*>(&kStackWalkStackPayloadV2[0]),
          sizeof(kStackWalkStackPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("EventTimeStamp", 1198356524732ULL);
  expected->AddField<UIntValue>("StackProcess", 7828U);
  expected->AddField<UIntValue>("StackThread", 1404U);

  const uint64_t kStackValues[] = {
      18446735285893805867ULL,
      140718042587290ULL,
      140718042589835ULL,
      140717494394206ULL,
      140717495106052ULL,
      140717541396037ULL,
      140717541395385ULL,
      140717541395351ULL,
      140717541311121ULL,
      140717625823603ULL,
      140717625823278ULL,
      140717626448659ULL,
      140717627685449ULL,
      140717625855001ULL,
      140717625854880ULL,
      140717625854737ULL,
      140717625855059ULL,
      140717627685154ULL,
      140717625832418ULL,
      140718065718733ULL,
      140718076806097ULL
  };
  std::unique_ptr<ArrayValue> stack(new ArrayValue());
  stack->AppendAll<ULongValue>(&kStackValues[0],
                               sizeof(kStackValues) / sizeof(uint64_t));
  expected->AddField("Stack", std::move(stack));

  EXPECT_STREQ("StackWalk", category.c_str());
  EXPECT_STREQ("Stack", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultTransitionFault32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultTransitionFaultOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPageFaultTransitionFaultPayload32bitsV2[0]),
          sizeof(kPageFaultTransitionFaultPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("VirtualAddress", 0x77388E2D);
  expected->AddField<UIntValue>("ProgramCounter", 0x77388E2D);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("TransitionFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultTransitionFaultV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultTransitionFaultOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultTransitionFaultPayloadV2[0]),
          sizeof(kPageFaultTransitionFaultPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("VirtualAddress", 0x000007FEFDE62C26ULL);
  expected->AddField<ULongValue>("ProgramCounter", 0x000007FEFDE62C26ULL);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("TransitionFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultDemandZeroFaultV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultDemandZeroFaultOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultDemandZeroFaultPayloadV2[0]),
          sizeof(kPageFaultDemandZeroFaultPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("VirtualAddress", 0x000007FFFFFAE020ULL);
  expected->AddField<ULongValue>("ProgramCounter", 0xFFFFF8000317FED6ULL);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("DemandZeroFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultCopyOnWriteV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultCopyOnWriteOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultCopyOnWritePayloadV2[0]),
          sizeof(kPageFaultCopyOnWritePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("VirtualAddress", 0x000007FEFDFFB228ULL);
  expected->AddField<ULongValue>("ProgramCounter", 0x00000000775D5469ULL);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("CopyOnWrite", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultAccessViolationV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultAccessViolationOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultAccessViolationPayloadV2[0]),
          sizeof(kPageFaultAccessViolationPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("VirtualAddress", 0x000007FFFFFF0000ULL);
  expected->AddField<ULongValue>("ProgramCounter", 0xFFFFF9600022CD8AULL);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("AccessViolation", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultHardPageFaultV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultHardPageFaultOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultHardPageFaultPayloadV2[0]),
          sizeof(kPageFaultHardPageFaultPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("VirtualAddress", 0xFFFFF9804966C000ULL);
  expected->AddField<ULongValue>("ProgramCounter", 0);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("HardPageFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultHardFault32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultHardFaultOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPageFaultHardFaultPayload32bitsV2[0]),
          sizeof(kPageFaultHardFaultPayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 0);
  expected->AddField<ULongValue>("ReadOffset", 0x00000000026b4000ULL);
  expected->AddField<UIntValue>("VirtualAddress", 0xa55b4000);
  expected->AddField<UIntValue>("FileObject", 0x85b1b008);
  expected->AddField<UIntValue>("TThreadId", 5008);
  expected->AddField<UIntValue>("ByteCount", 0x1000);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("HardFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultHardFaultV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultHardFaultOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultHardFaultPayloadV2[0]),
          sizeof(kPageFaultHardFaultPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("InitialTime", 107701904733ULL);
  expected->AddField<ULongValue>("ReadOffset", 150687744ULL);
  expected->AddField<ULongValue>("VirtualAddress", 408352ULL);
  expected->AddField<ULongValue>("FileObject", 18446738026691582464ULL);
  expected->AddField<UIntValue>("TThreadId", 10012U);
  expected->AddField<UIntValue>("ByteCount", 16384U);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("HardFault", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultVirtualAllocV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultVirtualAllocOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultVirtualAllocPayloadV2[0]),
          sizeof(kPageFaultVirtualAllocPayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 0x003B4000ULL);
  expected->AddField<ULongValue>("RegionSize", 0x6000ULL);
  expected->AddField<UIntValue>("ProcessId", 0x1804);
  expected->AddField<UIntValue>("Flags", 0x1000);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("VirtualAlloc", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultVirtualFree32bitsV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultVirtualFreeOpcode, k32bit,
          reinterpret_cast<const char*>(
              &kPageFaultVirtualFreePayload32bitsV2[0]),
          sizeof(kPageFaultVirtualFreePayload32bitsV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<UIntValue>("BaseAddress", 0x01420000);
  expected->AddField<UIntValue>("RegionSize", 0x00040000);
  expected->AddField<UIntValue>("ProcessId", 0x00000dd8);
  expected->AddField<UIntValue>("Flags", 0x00008000);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("VirtualFree", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

TEST(EtwRawDecoderTest, PageFaultVirtualFreeV2) {
  std::string operation;
  std::string category;
  std::unique_ptr<Value> fields;
  EXPECT_TRUE(
      DecodeRawETWKernelPayload(kPageFaultProviderId,
          kVersion2, kPageFaultVirtualFreeOpcode, k64bit,
          reinterpret_cast<const char*>(&kPageFaultVirtualFreePayloadV2[0]),
          sizeof(kPageFaultVirtualFreePayloadV2),
          &operation, &category, &fields));

  std::unique_ptr<StructValue> expected(new StructValue());
  expected->AddField<ULongValue>("BaseAddress", 0x003B4000ULL);
  expected->AddField<ULongValue>("RegionSize", 0x0000F000ULL);
  expected->AddField<UIntValue>("ProcessId", 0x1804);
  expected->AddField<UIntValue>("Flags", 0x4000);

  EXPECT_STREQ("PageFault", category.c_str());
  EXPECT_STREQ("VirtualFree", operation.c_str());
  EXPECT_TRUE(expected->Equals(fields.get()));
}

}  // namespace etw
}  // namespace parser

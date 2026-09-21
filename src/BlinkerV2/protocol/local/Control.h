#ifndef BLINKER_PROTOCOL_LOCAL_CONTROL_H
#define BLINKER_PROTOCOL_LOCAL_CONTROL_H

#include "Access.h"

namespace blinker {
namespace local_access {

enum class ControlKind : uint8_t { Hello = 3U, Command = 4U, Ack = 5U };
enum class ControlOperation : uint8_t { None = 0U, Enable = 1U, Disable = 2U, Retire = 3U };
enum : size_t { kControlMaximumEncodedSize = 112U };

// One exact shape for hello, commands and success-only ACKs. Kind is included
// in the MAC, so reflecting a command can never acknowledge its application.
struct ControlView {
    ControlKind kind;
    ByteView recipientDeviceInstanceId;
    uint32_t deviceKeyVersion;
    ByteView controlSessionId;
    uint32_t sequence;
    uint32_t authorityRevision;
    ControlOperation operation;
    ByteView authorizationId;
    ByteView authenticator;

    ControlView()
        : kind(ControlKind::Hello), recipientDeviceInstanceId(),
          deviceKeyVersion(0U), controlSessionId(), sequence(0U),
          authorityRevision(0U), operation(ControlOperation::None),
          authorizationId(), authenticator() {}
};

Result validateControlUnsigned(const ControlView& value);
Result validateControl(const ControlView& value);
Result encodeControl(const ControlView& value, MutableByteSpan output, size_t& written);
Result decodeControl(ByteView encoded, ControlView& value);
Result streamControlTranscript(const ControlView& value, cbor::WriteSink sink,
                               void* context, size_t& written);

} // namespace local_access
} // namespace blinker
#endif

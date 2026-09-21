#ifndef BLINKER_PROTOCOL_LOCAL_PROGRESS_H
#define BLINKER_PROTOCOL_LOCAL_PROGRESS_H

#include "Control.h"

namespace blinker { namespace local_access {

enum class ProgressKind : uint8_t { Query = 6U, Status = 7U };
enum class ProgressState : uint8_t { Query = 0U, Synchronized = 1U, Pending = 2U, Refresh = 3U };

// A fresh observation, NOT a command, ACK, grant or durable delivery item.
// Uses the same bounded 0x36 carrier; nonce and proof bind both directions.
struct ProgressView {
    uint8_t version;
    SecurityProfile securityProfile;
    ProgressKind kind;
    ByteView recipientDeviceInstanceId;
    uint32_t deviceKeyVersion;
    ByteView controlSessionId;
    ByteView requestNonce;
    uint32_t sequence;
    uint32_t authorityRevision;
    ProgressState state;
    ByteView authenticator;
    ProgressView() : version(1U), securityProfile(SecurityProfile::NoiseNnPsk0AesGcmSha256),
        kind(ProgressKind::Query), recipientDeviceInstanceId(), deviceKeyVersion(0U),
        controlSessionId(), requestNonce(), sequence(0U), authorityRevision(0U),
        state(ProgressState::Query), authenticator() {}
};

Result validateProgressUnsigned(const ProgressView& value);
Result encodeProgress(const ProgressView& value, MutableByteSpan output, size_t& written);
Result decodeProgress(ByteView encoded, ProgressView& value);
Result streamProgressTranscript(const ProgressView& value, cbor::WriteSink sink, void* context, size_t& written);

} } // namespace blinker::local_access
#endif

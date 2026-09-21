#ifndef BLINKER_SECURITY_LOCAL_ACCESS_NOISE_SESSION_H
#define BLINKER_SECURITY_LOCAL_ACCESS_NOISE_SESSION_H

#include "NoiseNn.h"
#include "../protocol/noise/LocalSecureRecord.h"
#include "../protocol/local/Access.h"

namespace blinker {
namespace security {

// Optional profile 1 adapter. Owns only existing Noise/record processing;
// authorization, queues, physical lifetime and BBP all belong to the caller.
class LocalAccessNoiseSession {
public:
    static constexpr local_access::SecurityProfile profile = local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256;
    static constexpr bool encrypted = true;
    enum : size_t { overhead = noise::kLocalSecureRecordHeaderSize + kNoiseTagSize,
        maximumMessage = noise::kLocalSecureRecordHeaderSize + noise::kLocalSecureRecordMaxBodySize };
    LocalAccessNoiseSession(INoiseCryptoProvider& crypto, IRandom& random) : noise_(crypto, random) {}
    Result begin(ByteView key, ByteView) {
        return noise_.begin(NoiseRole::Responder, noise::NoiseNnPattern::NnPsk0,
            key, noise::localAccessNoisePrologue());
    }
    Result reply(ByteView input, MutableByteSpan scratch, MutableByteSpan output, size_t& written) {
        written = 0U;
        noise::LocalSecureRecordView record;
        Result result = noise::parseLocalSecureRecord(input, record);
        if (result && (record.type != noise::LocalSecureRecordType::InitiatorHandshake ||
            record.body.size != kNoiseMaxHandshakeOverhead)) result = Result::failure(ErrorCode::ProtocolError);
        ByteView plaintext, response;
        if (result) result = noise_.readHandshake(record.body, scratch, plaintext);
        if (result && !plaintext.empty()) result = Result::failure(ErrorCode::ProtocolError);
        if (result) result = noise_.writeHandshake(ByteView(), bodyOutput(output), response);
        if (result) result = wrap(noise::LocalSecureRecordType::ResponderHandshake, response, output, written);
        return result;
    }
    Result encode(ByteView frame, MutableByteSpan output, size_t& written) {
        written = 0U; ByteView ciphertext;
        Result result = noise_.encryptTransport(frame, bodyOutput(output), ciphertext);
        if (result) result = wrap(noise::LocalSecureRecordType::Transport, ciphertext, output, written);
        return result;
    }
    Result decode(ByteView input, MutableByteSpan output, ByteView& frame) {
        frame = ByteView();
        noise::LocalSecureRecordView record;
        Result result = noise::parseLocalSecureRecord(input, record);
        if (result && (record.type != noise::LocalSecureRecordType::Transport || record.body.size <= kNoiseTagSize))
            result = Result::failure(ErrorCode::ProtocolError);
        if (result) result = noise_.decryptTransport(record.body, output, frame);
        return result;
    }
    void clear() { noise_.clear(); }
private:
    static MutableByteSpan bodyOutput(MutableByteSpan output) {
        return output.data && output.size >= noise::kLocalSecureRecordHeaderSize
            ? MutableByteSpan(output.data + noise::kLocalSecureRecordHeaderSize, output.size - noise::kLocalSecureRecordHeaderSize)
            : MutableByteSpan();
    }
    static Result wrap(noise::LocalSecureRecordType type, ByteView body, MutableByteSpan output, size_t& written) {
        ByteView encoded;
        const Result result = noise::encodeLocalSecureRecord(type, body, output, encoded);
        if (result) written = encoded.size;
        return result;
    }
    NoiseNnSession noise_;
};

} // namespace security
} // namespace blinker
#endif

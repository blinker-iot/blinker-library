#ifndef BLINKER_SECURITY_LOCAL_ACCESS_PLAIN_SESSION_H
#define BLINKER_SECURITY_LOCAL_ACCESS_PLAIN_SESSION_H

#include "LocalAccessSecurity.h"
#include "../core/SecureMemory.h"
#include <string.h>

namespace blinker {
namespace security {

// Profile 2: finite session proof, then unencrypted and unauthenticated BBP
// records. This is NOT a secure channel against an active network attacker.
// Only temporary expected proofs are retained, not the session key.
class LocalAccessPlainSession {
public:
    static constexpr local_access::SecurityProfile profile = local_access::SecurityProfile::PlainHmacSha256;
    static constexpr bool encrypted = false;
    enum : size_t { overhead = 0U, maximumMessage = 1024U };

    LocalAccessPlainSession() : caller_(), recipient_(), state_(State::Empty) {}
    ~LocalAccessPlainSession() { clear(); }
    LocalAccessPlainSession(const LocalAccessPlainSession&) = delete;
    LocalAccessPlainSession& operator=(const LocalAccessPlainSession&) = delete;

    Result begin(ByteView key, ByteView grantAuthenticator) {
        clear();
        Result result = computeLocalAccessPlainProof(key, grantAuthenticator, local_access::kCallerProofKind,
            MutableByteSpan(caller_, sizeof(caller_)));
        if (result) result = computeLocalAccessPlainProof(key, grantAuthenticator, local_access::kRecipientProofKind,
            MutableByteSpan(recipient_, sizeof(recipient_)));
        if (result) state_ = State::Proof;
        else clear();
        return result;
    }
    Result reply(ByteView input, MutableByteSpan, MutableByteSpan output, size_t& written) {
        written = 0U;
        ByteView proof;
        Result result = local_access::decodePlainProof(input, local_access::kCallerProofKind, proof);
        if (result && (state_ != State::Proof || !constantTimeEqual(proof, ByteView(caller_, sizeof(caller_)))))
            result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result) result = local_access::encodePlainProof(local_access::kRecipientProofKind,
            ByteView(recipient_, sizeof(recipient_)), output, written);
        clear();
        if (result) state_ = State::Ready;
        return result;
    }
    Result encode(ByteView frame, MutableByteSpan output, size_t& written) {
        written = 0U;
        if (state_ != State::Ready) return Result::failure(ErrorCode::NotConnected);
        if (frame.data == nullptr || frame.empty()) return Result::failure(ErrorCode::InvalidArgument);
        if (output.data == nullptr || output.size < frame.size) return Result::failure(ErrorCode::BufferTooSmall);
        memmove(output.data, frame.data, frame.size); written = frame.size;
        return Result::success();
    }
    Result decode(ByteView input, MutableByteSpan output, ByteView& frame) {
        frame = ByteView();
        size_t written = 0U;
        const Result result = encode(input, output, written);
        if (result) frame = ByteView(output.data, written);
        return result;
    }
    void clear() {
        secureZero(MutableByteSpan(caller_, sizeof(caller_)));
        secureZero(MutableByteSpan(recipient_, sizeof(recipient_)));
        state_ = State::Empty;
    }
private:
    enum class State : uint8_t { Empty, Proof, Ready };
    uint8_t caller_[local_access::kAuthenticatorSize], recipient_[local_access::kAuthenticatorSize];
    State state_;
};

} // namespace security
} // namespace blinker
#endif

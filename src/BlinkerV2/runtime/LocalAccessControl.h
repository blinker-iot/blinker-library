#ifndef BLINKER_RUNTIME_LOCALACCESSCONTROL_H
#define BLINKER_RUNTIME_LOCALACCESSCONTROL_H

#include "LocalAccessLease.h"

namespace blinker {

// One receiver per product, not per MQTT connection or child. Lives through
// network reconnects so a cloud outage does not reset finite local leases.
// No socket, scheduler, persistent secret, retry queue or platform SDK here.
template <size_t Capacity, local_access::SecurityProfile Profile = local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256>
class LocalAccessControlReceiver {
public:
    LocalAccessControlReceiver(LocalAccessAuthority<Capacity>& authority,
                               IDeviceKeySource& keys, IRandom& random)
        : authority_(authority), keys_(keys), random_(random), recipient_(),
          session_(), lastAuthenticator_(), keyVersion_(0U), sequence_(0U) {}
    ~LocalAccessControlReceiver() { clear(); }

    // Only at product boot or an explicit identity/Key reset; ordinary MQTT
    // reconnect sends hello() again. A live receiver cannot silently restart.
    Result begin(ByteView instance, uint32_t keyVersion) {
        if (keyVersion_ != 0U) return Result::failure(ErrorCode::StateConflict);
        local_access::ControlView value;
        value.recipientDeviceInstanceId = instance;
        value.deviceKeyVersion = keyVersion;
        Result result = random_.fill(MutableByteSpan(session_, sizeof(session_)));
        value.controlSessionId = ByteView(session_, sizeof(session_));
        if (result) result = local_access::validateControlUnsigned(value);
        if (!result) { clear(); return result; }
        memcpy(recipient_, instance.data, sizeof(recipient_));
        authority_.clear();
        keyVersion_ = keyVersion;
        return Result::success();
    }

    Result hello(ByteView proofSessionId, MutableByteSpan output, size_t& written) {
        written = 0U;
        if (keyVersion_ == 0U) return Result::failure(ErrorCode::NotConfigured);
        local_access::ControlView value;
        value.recipientDeviceInstanceId = ByteView(recipient_, sizeof(recipient_));
        value.deviceKeyVersion = keyVersion_;
        value.controlSessionId = ByteView(session_, sizeof(session_));
        value.sequence = sequence_;
        value.authorityRevision = authority_.authorityRevision();
        DeviceKey key;
        Result result = keys_.load(key);
        if (result) result = signAndEncode(key, value, output, written, proofSessionId);
        clearDeviceKey(key);
        return result;
    }

    Result query(ByteView nonce, ByteView proof, MutableByteSpan output, size_t& written) {
        written = 0U;
        if (keyVersion_ == 0U) return Result::failure(ErrorCode::NotConfigured);
        local_access::ProgressView value;
        value.version = 2U; value.securityProfile = Profile;
        value.recipientDeviceInstanceId = ByteView(recipient_, sizeof(recipient_));
        value.deviceKeyVersion = keyVersion_; value.controlSessionId = ByteView(session_, sizeof(session_));
        value.requestNonce = nonce; value.sequence = sequence_; value.authorityRevision = authority_.authorityRevision();
        DeviceKey key; uint8_t mac[local_access::kAuthenticatorSize] = {};
        Result result = keys_.load(key);
        if (result) result = security::computeLocalAccessProgressAuthenticator(key, value, proof, MutableByteSpan(mac, sizeof(mac)));
        value.authenticator = ByteView(mac, sizeof(mac));
        if (result) result = local_access::encodeProgress(value, output, written);
        clearDeviceKey(key); secureZero(MutableByteSpan(mac, sizeof(mac)));
        return result;
    }

    // Caller owns one fresh nonce and its deadline. No lease mutation here.
    Result progress(ByteView encoded, ByteView nonce, ByteView proof, local_access::ProgressView& value) {
        Result result = local_access::decodeProgress(encoded, value);
        if (!result) return result;
        if (keyVersion_ == 0U || value.version != 2U || value.securityProfile != Profile ||
            value.kind != local_access::ProgressKind::Status || value.deviceKeyVersion != keyVersion_ ||
            !constantTimeEqual(value.requestNonce, nonce) ||
            !constantTimeEqual(value.recipientDeviceInstanceId, ByteView(recipient_, sizeof(recipient_))) ||
            !constantTimeEqual(value.controlSessionId, ByteView(session_, sizeof(session_))) || value.sequence > sequence_ ||
            value.authorityRevision > authority_.authorityRevision() ||
            (value.state == local_access::ProgressState::Synchronized &&
                (value.sequence != sequence_ || value.authorityRevision != authority_.authorityRevision())))
            return Result::failure(ErrorCode::AuthenticationRequired);
        DeviceKey key;
        result = keys_.load(key);
        if (result) result = security::verifyLocalAccessProgressAuthenticator(key, value, proof);
        clearDeviceKey(key);
        return result;
    }

    Result accept(ByteView encoded, MutableByteSpan ack, size_t& written) {
        written = 0U;
        const uintptr_t inputAddress = reinterpret_cast<uintptr_t>(encoded.data);
        const uintptr_t outputAddress = reinterpret_cast<uintptr_t>(ack.data);
        if (encoded.data != nullptr && ack.data != nullptr &&
            (inputAddress <= outputAddress ? outputAddress - inputAddress < encoded.size
                                           : inputAddress - outputAddress < ack.size)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        local_access::ControlView command;
        Result result = local_access::decodeControl(encoded, command);
        if (!result) return result;
        if (keyVersion_ == 0U || command.kind != local_access::ControlKind::Command ||
            command.deviceKeyVersion != keyVersion_ ||
            memcmp(command.recipientDeviceInstanceId.data, recipient_, sizeof(recipient_)) != 0 ||
            memcmp(command.controlSessionId.data, session_, sizeof(session_)) != 0) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        DeviceKey key;
        result = keys_.load(key);
        if (result) result = security::verifyLocalAccessControlAuthenticator(key, command);
        const bool duplicate = command.sequence == sequence_ &&
            constantTimeEqual(command.authenticator, ByteView(lastAuthenticator_, sizeof(lastAuthenticator_)));
        if (result && !duplicate && (sequence_ == UINT32_MAX || command.sequence != sequence_ + 1U)) {
            result = Result::failure(ErrorCode::StateConflict);
        }
        // Precompute the exact success ACK before mutation. A too-small output
        // buffer must neither consume the sequence nor retire a live lease.
        local_access::ControlView response = command;
        response.kind = local_access::ControlKind::Ack;
        if (result) result = signAndEncode(key, response, ack, written);
        clearDeviceKey(key);
        if (result && !duplicate) {
            result = apply(command);
            if (result) {
                sequence_ = command.sequence;
                memcpy(lastAuthenticator_, command.authenticator.data, sizeof(lastAuthenticator_));
            }
        }
        if (!result) { secureZero(MutableByteSpan(ack.data, written)); written = 0U; }
        return result;
    }

    void clear() {
        authority_.clear();
        secureZero(MutableByteSpan(recipient_, sizeof(recipient_)));
        secureZero(MutableByteSpan(session_, sizeof(session_)));
        secureZero(MutableByteSpan(lastAuthenticator_, sizeof(lastAuthenticator_)));
        keyVersion_ = 0U;
        sequence_ = 0U;
    }

private:
    static Result signAndEncode(const DeviceKey& key, local_access::ControlView& value,
                                MutableByteSpan output, size_t& written, ByteView proofSessionId = ByteView()) {
        uint8_t mac[local_access::kAuthenticatorSize] = {};
        Result result = security::computeLocalAccessControlAuthenticator(key, value, MutableByteSpan(mac, sizeof(mac)), proofSessionId);
        value.authenticator = ByteView(mac, sizeof(mac));
        if (result) result = local_access::encodeControl(value, output, written);
        secureZero(MutableByteSpan(mac, sizeof(mac)));
        return result;
    }

    Result apply(const local_access::ControlView& command) {
        if (command.operation == local_access::ControlOperation::Retire) {
            if (!authority_.configured() || command.authorityRevision != authority_.authorityRevision()) {
                return Result::failure(ErrorCode::StateConflict);
            }
            return authority_.retireAuthorization(command.authorizationId);
        }
        // Disable at an unchanged revision is allowed for an exhausted server
        // generation; enabling it again requires a strictly newer revision.
        Result result = Result::success();
        if (command.operation != local_access::ControlOperation::Disable ||
            !authority_.configured() || command.authorityRevision != authority_.authorityRevision()) {
            result = authority_.configure(command.recipientDeviceInstanceId,
                command.deviceKeyVersion, command.authorityRevision);
        }
        if (result && command.operation == local_access::ControlOperation::Disable) authority_.disable();
        return result;
    }

    LocalAccessAuthority<Capacity>& authority_;
    IDeviceKeySource& keys_;
    IRandom& random_;
    uint8_t recipient_[local_access::kIdSize];
    uint8_t session_[local_access::kIdSize];
    uint8_t lastAuthenticator_[local_access::kAuthenticatorSize];
    uint32_t keyVersion_;
    uint32_t sequence_;

    LocalAccessControlReceiver(const LocalAccessControlReceiver&);
    LocalAccessControlReceiver& operator=(const LocalAccessControlReceiver&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(LocalAccessControlReceiver<2U>) <= 84U,
              "LocalAccess control receiver exceeds its 32-bit ABI gate");
#endif
} // namespace blinker
#endif

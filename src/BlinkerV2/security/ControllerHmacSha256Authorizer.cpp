#include "ControllerHmacSha256Authorizer.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {
namespace security {

namespace {

const uint8_t kDeviceProofDomain[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'C', 'O', 'N', 'T', 'R', 'O',
    'L', 'L', 'E', 'R', '-', 'D', 'E', 'V', 'I', 'C', 'E', '-', 'P', 'R',
    'O', 'O', 'F', '/', '1'
};
const uint8_t kAppProofDomain[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'C', 'O', 'N', 'T', 'R', 'O',
    'L', 'L', 'E', 'R', '-', 'A', 'P', 'P', '-', 'P', 'R', 'O', 'O', 'F',
    '/', '1'
};

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool validDomainGeneration(
    ControllerCredentialDomain domain,
    uint32_t generation) {
    return (domain == ControllerCredentialDomain::Local &&
            generation == 0U) ||
           (domain == ControllerCredentialDomain::Ownership &&
            generation != 0U);
}

bool sameChallenge(
    const ControllerAuthSession& state,
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t generation,
    uint32_t version,
    ByteView clientNonce) {
    return state.challengeActive && state.domain == domain &&
           state.ownershipGeneration == generation &&
           state.credentialVersion == version &&
           constantTimeEqual(
               ByteView(state.controllerId, sizeof(state.controllerId)),
               controllerId) &&
           constantTimeEqual(
               ByteView(state.clientNonce, sizeof(state.clientNonce)),
               clientNonce);
}

} // namespace

ControllerAuthSession::ControllerAuthSession()
    : sessionId(0U),
      ownershipGeneration(0U),
      credentialVersion(0U),
      permissions(0U),
      controllerId(),
      clientNonce(),
      deviceNonce(),
      transportId(0U),
      domain(ControllerCredentialDomain::Local),
      challengeActive(false),
      occupied(false) {}

void ControllerAuthSession::clear() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(this),
        sizeof(*this)));
    domain = ControllerCredentialDomain::Local;
}

Result computeControllerAuthProof(
    ByteView secret,
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration,
    uint32_t credentialVersion,
    uint32_t permissions,
    ByteView clientNonce,
    ByteView deviceNonce,
    ControllerAuthProofRole role,
    MutableByteSpan output) {
    if (secret.size != kControllerCredentialSecretSize || !nonZero(secret) ||
        controllerId.size != kControllerIdSize || !nonZero(controllerId) ||
        !validDomainGeneration(domain, ownershipGeneration) ||
        credentialVersion == 0U ||
        !validAuthorizationPermissions(permissions) ||
        clientNonce.size != kControllerAuthNonceSize ||
        !nonZero(clientNonce) ||
        deviceNonce.size != kControllerAuthNonceSize ||
        !nonZero(deviceNonce) || output.data == nullptr ||
        (role != ControllerAuthProofRole::Device &&
         role != ControllerAuthProofRole::App)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size < kControllerAuthProofSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    const ByteView roleDomain =
        role == ControllerAuthProofRole::Device
            ? ByteView(kDeviceProofDomain, sizeof(kDeviceProofDomain))
            : ByteView(kAppProofDomain, sizeof(kAppProofDomain));
    const uint8_t prefix[] = {
        kControllerAuthTranscriptVersion,
        static_cast<uint8_t>(kControllerHmacSha256Method >> 8U),
        static_cast<uint8_t>(kControllerHmacSha256Method),
        static_cast<uint8_t>(ControllerCredentialSuite::HmacSha256_32)
    };
    uint8_t metadata[13U];
    metadata[0] = static_cast<uint8_t>(domain);
    writeU32(metadata + 1U, ownershipGeneration);
    writeU32(metadata + 5U, credentialVersion);
    writeU32(metadata + 9U, permissions);

    HmacSha256 hmac;
    Result result = hmac.begin(secret);
    if (result) result = hmac.update(roleDomain);
    if (result) result = hmac.update(ByteView(prefix, sizeof(prefix)));
    if (result) result = hmac.update(controllerId);
    if (result) result = hmac.update(ByteView(metadata, sizeof(metadata)));
    if (result) result = hmac.update(clientNonce);
    if (result) result = hmac.update(deviceNonce);
    if (result) result = hmac.finish(output);
    secureZero(MutableByteSpan(metadata, sizeof(metadata)));
    return result;
}

ControllerHmacSha256Authorizer::ControllerHmacSha256Authorizer(
    IControllerCredentialSource& credentials,
    IRandom& random,
    ControllerAuthSession* sessions,
    size_t sessionCapacity)
    : credentials_(credentials),
      random_(random),
      sessions_(sessions),
      sessionCapacity_(sessionCapacity),
      response_() {
    resetSessions();
}

ControllerHmacSha256Authorizer::~ControllerHmacSha256Authorizer() {
    resetSessions();
}

size_t ControllerHmacSha256Authorizer::methodCount() const {
    return sessions_ != nullptr && sessionCapacity_ != 0U ? 1U : 0U;
}

uint16_t ControllerHmacSha256Authorizer::methodAt(size_t index) const {
    return methodCount() != 0U && index == 0U
               ? kControllerHmacSha256Method
               : 0U;
}

Result ControllerHmacSha256Authorizer::loadCredential(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration,
    uint32_t credentialVersion,
    ControllerCredential& credential,
    bool& found) {
    found = false;
    Result result = credentials_.loadActive(
        controllerId,
        domain,
        ownershipGeneration,
        credential);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    if (!result) return result;
    result = validateControllerCredential(credential);
    if (!result) return result;
    found = credential.credentialVersion == credentialVersion &&
            credential.suite == ControllerCredentialSuite::HmacSha256_32;
    return Result::success();
}

Result ControllerHmacSha256Authorizer::writeChallenge(
    const ControllerCredential& credential,
    ControllerAuthSession& session,
    AuthorizationDecision& decision) {
    response_[0] = ControllerAuthPayloadChallenge;
    memcpy(
        response_ + 1U,
        session.deviceNonce,
        sizeof(session.deviceNonce));
    writeU32(
        response_ + 1U + kControllerAuthNonceSize,
        credential.permissions);
    Result result = computeControllerAuthProof(
        credential.authenticationSecret(),
        credential.id(),
        credential.domain,
        credential.ownershipGeneration,
        credential.credentialVersion,
        credential.permissions,
        ByteView(session.clientNonce, sizeof(session.clientNonce)),
        ByteView(session.deviceNonce, sizeof(session.deviceNonce)),
        ControllerAuthProofRole::Device,
        MutableByteSpan(
            response_ + 1U + kControllerAuthNonceSize + 4U,
            kControllerAuthProofSize));
    if (!result) {
        secureZero(MutableByteSpan(response_, sizeof(response_)));
        return result;
    }
    decision.status = bbp2::AuthStatus::Continue;
    decision.responsePayload = ByteView(response_, sizeof(response_));
    return Result::success();
}

Result ControllerHmacSha256Authorizer::authorize(
    uint16_t method,
    ByteView requestPayload,
    const AuthorizationSessionContext& session,
    AuthorizationDecision& decision) {
    decision = AuthorizationDecision();
    secureZero(MutableByteSpan(response_, sizeof(response_)));
    if (methodCount() == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (method != kControllerHmacSha256Method) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!session.encrypted || session.sessionId == 0U) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (requestPayload.data == nullptr || requestPayload.empty()) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    if (requestPayload.data[0] == ControllerAuthPayloadInit) {
        if (requestPayload.size != kControllerAuthInitPayloadSize) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        const ByteView controllerId(requestPayload.data + 1U, kControllerIdSize);
        const ControllerCredentialDomain domain =
            static_cast<ControllerCredentialDomain>(
                requestPayload.data[1U + kControllerIdSize]);
        const uint32_t generation = readU32(
            requestPayload.data + 1U + kControllerIdSize + 1U);
        const uint32_t version = readU32(
            requestPayload.data + 1U + kControllerIdSize + 1U + 4U);
        const ByteView clientNonce(
            requestPayload.data + 1U + kControllerIdSize + 1U + 8U,
            kControllerAuthNonceSize);
        if (!nonZero(controllerId) ||
            !validDomainGeneration(domain, generation) || version == 0U ||
            !nonZero(clientNonce)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }

        ControllerAuthSession* state = findSession(
            session.transportId,
            session.sessionId);
        if (state != nullptr && state->challengeActive &&
            !sameChallenge(
                *state,
                controllerId,
                domain,
                generation,
                version,
                clientNonce)) {
            return Result::success();
        }

        ControllerCredential credential;
        bool found = false;
        Result result = loadCredential(
            controllerId,
            domain,
            generation,
            version,
            credential,
            found);
        if (!result || !found) {
            // Revocation, rotation or suite replacement invalidates an
            // already-issued challenge immediately instead of pinning a
            // stale session slot until proof or disconnect.
            if (result && state != nullptr) state->clear();
            clearControllerCredential(credential);
            return result;
        }

        const bool existing = state != nullptr && state->challengeActive;
        if (state == nullptr) {
            state = allocateSession(session.transportId, session.sessionId);
        }
        if (state == nullptr) {
            clearControllerCredential(credential);
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        if (!existing) {
            state->domain = domain;
            state->ownershipGeneration = generation;
            state->credentialVersion = version;
            state->permissions = credential.permissions;
            memcpy(state->controllerId, controllerId.data, kControllerIdSize);
            memcpy(
                state->clientNonce,
                clientNonce.data,
                kControllerAuthNonceSize);
            result = random_.fill(MutableByteSpan(
                state->deviceNonce,
                sizeof(state->deviceNonce)));
            if (!result || !nonZero(ByteView(
                               state->deviceNonce,
                               sizeof(state->deviceNonce)))) {
                state->clear();
                clearControllerCredential(credential);
                return result
                           ? Result::failure(ErrorCode::ProtocolError)
                           : result;
            }
            state->challengeActive = true;
        }
        if (state->permissions != credential.permissions) {
            state->clear();
            clearControllerCredential(credential);
            return Result::success();
        }
        result = writeChallenge(credential, *state, decision);
        if (!result) state->clear();
        clearControllerCredential(credential);
        return result;
    }

    if (requestPayload.data[0] == ControllerAuthPayloadProof) {
        if (requestPayload.size != kControllerAuthProofPayloadSize) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        ControllerAuthSession* state = findSession(
            session.transportId,
            session.sessionId);
        if (state == nullptr || !state->challengeActive) {
            return Result::success();
        }

        ControllerCredential credential;
        bool found = false;
        Result result = loadCredential(
            ByteView(state->controllerId, sizeof(state->controllerId)),
            state->domain,
            state->ownershipGeneration,
            state->credentialVersion,
            credential,
            found);
        if (!result || !found ||
            credential.permissions != state->permissions) {
            state->clear();
            clearControllerCredential(credential);
            return result;
        }

        uint8_t expected[kControllerAuthProofSize];
        result = computeControllerAuthProof(
            credential.authenticationSecret(),
            credential.id(),
            credential.domain,
            credential.ownershipGeneration,
            credential.credentialVersion,
            credential.permissions,
            ByteView(state->clientNonce, sizeof(state->clientNonce)),
            ByteView(state->deviceNonce, sizeof(state->deviceNonce)),
            ControllerAuthProofRole::App,
            MutableByteSpan(expected, sizeof(expected)));
        const bool matches = result && constantTimeEqual(
            ByteView(expected, sizeof(expected)),
            ByteView(requestPayload.data + 1U, kControllerAuthProofSize));
        secureZero(MutableByteSpan(expected, sizeof(expected)));
        const uint32_t permissions = credential.permissions;
        clearControllerCredential(credential);
        state->clear();
        if (!result) return result;
        if (matches) {
            decision.status = bbp2::AuthStatus::Authorized;
            decision.permissions = permissions;
        }
        return Result::success();
    }

    return Result::failure(ErrorCode::InvalidEncoding);
}

void ControllerHmacSha256Authorizer::sessionClosed(
    const AuthorizationSessionContext& session) {
    ControllerAuthSession* state = findSession(
        session.transportId,
        session.sessionId);
    if (state != nullptr) state->clear();
}

void ControllerHmacSha256Authorizer::resetSessions() {
    if (sessions_ != nullptr) {
        for (size_t index = 0U; index < sessionCapacity_; ++index) {
            sessions_[index].clear();
        }
    }
    secureZero(MutableByteSpan(response_, sizeof(response_)));
}

ControllerAuthSession* ControllerHmacSha256Authorizer::findSession(
    uint8_t transportId,
    uint32_t sessionId) {
    if (sessions_ == nullptr) return nullptr;
    for (size_t index = 0U; index < sessionCapacity_; ++index) {
        ControllerAuthSession& state = sessions_[index];
        if (state.occupied && state.transportId == transportId &&
            state.sessionId == sessionId) {
            return &state;
        }
    }
    return nullptr;
}

ControllerAuthSession* ControllerHmacSha256Authorizer::allocateSession(
    uint8_t transportId,
    uint32_t sessionId) {
    if (sessions_ == nullptr) return nullptr;
    for (size_t index = 0U; index < sessionCapacity_; ++index) {
        ControllerAuthSession& state = sessions_[index];
        if (state.occupied) continue;
        state.clear();
        state.occupied = true;
        state.transportId = transportId;
        state.sessionId = sessionId;
        return &state;
    }
    return nullptr;
}

} // namespace security
} // namespace blinker

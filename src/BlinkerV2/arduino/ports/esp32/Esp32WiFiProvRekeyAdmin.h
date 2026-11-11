#ifndef BLINKER_ESP32_WIFIPROV_REKEY_ADMIN_H
#define BLINKER_ESP32_WIFIPROV_REKEY_ADMIN_H

#include "Esp32WiFiProvSecurity1Owner.h"
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/interface/IRandom.h>
#include <BlinkerV2/provisioning/WifiprovRekeyEndpoint.h>

namespace blinker {

// Narrow, read-only local Admin authentication for a verified native setup
// owner. Neither 08 nor 09 reserves an attempt, writes the root, or permits
// Wi-Fi; those ports still enforce the journal and product admission.
class Esp32WiFiProvRekeyAdmin {
public:
    enum : uint8_t { nonceOperation = 8U, authorizeOperation = 9U };
    enum : size_t {
        authorizeRequestSize = 2U + security::kWifiprovRekeyOldBindingSize + kControllerIdSize + 4U +
            security::kWifiprovRekeyOwnerNonceSize + security::kWifiprovRekeyMacSize,
        successResponseSize = 3U + security::kWifiprovRekeyOwnerNonceSize
    };

    Esp32WiFiProvRekeyAdmin(Esp32WiFiProvSecurity1Owner& nativeOwner, const DeviceInstanceId& instance,
        IControllerCredentialStore& credentials, IRandom& random)
        : nativeOwner_(nativeOwner), instance_(instance), credentials_(credentials), random_(random) {}

    Result handle(uint32_t session, ByteView request, MutableByteSpan response, size_t& written) {
        written = 0U;
        if (response.data == nullptr || response.size < 3U) return Result::failure(ErrorCode::BufferTooSmall);
        const uint8_t op = request.data != nullptr && request.size >= 2U ? request.data[1] : 0U;
        const uint8_t version = request.data != nullptr && request.size >= 2U ? request.data[0] : 0U;
        response.data[0] = kDeviceKeyProvisioningVersion;
        response.data[1] = op;
        const auto status = [&](DeviceKeyProvisioningStatus value) {
            response.data[2] = static_cast<uint8_t>(value);
            written = 3U;
            return Result::success();
        };
        using Status = DeviceKeyProvisioningStatus;
        if (request.data == nullptr || request.size < 2U) return status(Status::Malformed);
        if (version != kDeviceKeyProvisioningVersion) return status(Status::UnsupportedVersion);
        if (op != nonceOperation && op != authorizeOperation) return status(Status::UnsupportedOperation);
        if (request.size != (op == nonceOperation ? 2U : static_cast<size_t>(authorizeRequestSize)))
            return status(Status::Malformed);
        if (response.size < successResponseSize) return Result::failure(ErrorCode::BufferTooSmall);
        Esp32WiFiProvRekeyNativeOwner native{};
        if (!nativeOwner_.current(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session, native))
            return status(Status::AccessRequired);
        if (!matches(native)) {
            current_ = native;
            hasNonce_ = false;
            authorized_ = false;
            secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
        }

        if (op == nonceOperation) {
            if (!hasNonce_) {
                const Result result = random_.fill(MutableByteSpan(nonce_, sizeof(nonce_)));
                uint8_t combined = 0U;
                for (uint8_t byte : nonce_) combined = static_cast<uint8_t>(combined | byte);
                if (!result || combined == 0U) {
                    secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
                    return status(Status::StorageFailure);
                }
                hasNonce_ = true;
            }
            (void)status(Status::Success);
            memcpy(response.data + 3U, nonce_, sizeof(nonce_));
        } else {
            if (!hasNonce_) return status(Status::AccessRequired);
            security::WifiprovRekeyOldBinding incoming;
            if (!security::decodeWifiprovRekeyOldBinding(request.subspan(2U, security::kWifiprovRekeyOldBindingSize), incoming))
                return status(Status::Malformed);
            if (!sameDeviceInstanceId(instance_, incoming.instance)) return status(Status::AccessRequired);
            const size_t adminOffset = 2U + security::kWifiprovRekeyOldBindingSize;
            const ByteView id = request.subspan(adminOffset, kControllerIdSize);
            const size_t versionOffset = adminOffset + kControllerIdSize;
            uint32_t adminVersion = 0U;
            for (size_t i = 0U; i < 4U; ++i) adminVersion = (adminVersion << 8U) | request.data[versionOffset + i];
            const size_t nonceOffset = versionOffset + 4U;
            const ByteView nonce(nonce_, sizeof(nonce_));
            if (adminVersion == 0U || !constantTimeEqual(nonce, request.subspan(nonceOffset, nonce.size)))
                return status(Status::AccessRequired);
            if (authorized_ && (!sameOldBinding(previous_, incoming) || !constantTimeEqual(admin_.id(), id) || admin_.version != adminVersion))
                return status(Status::Conflict);
            ControllerCredential admin;
            Result result = credentials_.loadActive(id, ControllerCredentialDomain::PlatformAccess, incoming.accessEpoch, admin);
            if (result && (!constantTimeEqual(id, admin.id()) || admin.credentialVersion != adminVersion))
                result = Result::failure(ErrorCode::AuthenticationRequired);
            if (result) result = security::verifyWifiprovRekeySetupAdminProof(admin, incoming, nonce,
                request.subspan(nonceOffset + nonce.size, security::kWifiprovRekeyMacSize));
            clearControllerCredential(admin);
            if (!result) return status(Status::AccessRequired);
            memcpy(admin_.controllerId, id.data, sizeof(admin_.controllerId));
            admin_.version = adminVersion;
            previous_ = incoming;
            authorized_ = true;
            (void)status(Status::Success);
            memcpy(response.data + 3U, incoming.operationId, sizeof(incoming.operationId));
        }
        written = successResponseSize;
        return Result::success();
    }

    bool allows(Esp32WiFiProvRekeyEndpointKind endpoint, uint32_t session,
        const security::WifiprovRekeyOldBinding& previous, WifiprovRekeyAdmin& admin) const {
        Esp32WiFiProvRekeyNativeOwner native{};
        if (!authorized_ || !nativeOwner_.current(endpoint, session, native) || !matches(native) || !sameOldBinding(previous_, previous))
            return false;
        admin = admin_;
        return true;
    }

private:
    bool matches(const Esp32WiFiProvRekeyNativeOwner& owner) const {
        return current_.generation == owner.generation && current_.session == owner.session;
    }
    static bool sameOldBinding(const security::WifiprovRekeyOldBinding& a, const security::WifiprovRekeyOldBinding& b) {
        return a.accessEpoch == b.accessEpoch && a.fromVersion == b.fromVersion && sameDeviceInstanceId(a.instance, b.instance) &&
            memcmp(a.operationId, b.operationId, sizeof(a.operationId)) == 0 && memcmp(a.challenge, b.challenge, sizeof(a.challenge)) == 0 &&
            memcmp(a.oldLocator.bytes, b.oldLocator.bytes, sizeof(a.oldLocator.bytes)) == 0;
    }

    Esp32WiFiProvSecurity1Owner& nativeOwner_;
    const DeviceInstanceId& instance_;
    IControllerCredentialStore& credentials_;
    IRandom& random_;
    Esp32WiFiProvRekeyNativeOwner current_{};
    security::WifiprovRekeyOldBinding previous_{};
    WifiprovRekeyAdmin admin_{};
    uint8_t nonce_[security::kWifiprovRekeyOwnerNonceSize] = {};
    bool hasNonce_ = false;
    bool authorized_ = false;
};

static_assert(Esp32WiFiProvRekeyAdmin::authorizeRequestSize == 142U &&
    Esp32WiFiProvRekeyAdmin::successResponseSize == 19U, "WiFiProv setup Admin frame sizes");

} // namespace blinker

#endif

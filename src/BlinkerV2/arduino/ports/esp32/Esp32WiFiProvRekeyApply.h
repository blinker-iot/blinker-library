#ifndef BLINKER_ESP32_WIFIPROV_REKEY_APPLY_H
#define BLINKER_ESP32_WIFIPROV_REKEY_APPLY_H

#include "Esp32WiFiProvConfigFence.h"
#include "Esp32WiFiProvRekeyAdmin.h"
#include <BlinkerV2/provisioning/WifiprovRekeyJournal.h>

namespace blinker {

// Candidate only. Wire 07 can open a single original SDK Set/Apply on the
// SAME native owner as the local write. Recovery on a new owner is read-only;
// the Gateway MAC does not carry a replacement owner nonce. The composition
// must pass a separately opened durable read-only handle, never the writer.
class Esp32WiFiProvRekeyApply final : public IEsp32WiFiProvConfigFence {
public:
    enum : uint8_t { permitOperation = 7U };
    enum : size_t {
        requestSize = 2U + security::kWifiprovRekeyBindingSize + security::kWifiprovRekeyMacSize,
        responseSize = 3U + security::kWifiprovRekeyOperationSize
    };

    Esp32WiFiProvRekeyApply(Esp32WiFiProvSecurity1Owner& nativeOwner, Esp32WiFiProvRekeyAdmin& auth,
        DeviceAccessStore& access, IAtomicBlobStore& durableRoot, WifiprovRekeyJournal& journal)
        : nativeOwner_(nativeOwner), auth_(auth), access_(access), durableRoot_(durableRoot), journal_(journal) {}

    // Called by the guarded 05 composition BEFORE its device write, after
    // its own request validation. No Apply is possible until 05 then succeeds.
    bool beginInstall(uint32_t session, const security::WifiprovRekeyBinding& binding) {
        if (phase_ != Phase::Idle) return false;
        const auto endpoint = Esp32WiFiProvRekeyEndpointKind::BlinkerConfig;
        if (!nativeOwner_.current(endpoint, session, installer_) || !auth_.allows(endpoint, session, binding.previous, admin_))
            return false;
        WifiprovRekeyJournalPhase persisted;
        if (journal_.inspect(binding, admin_, persisted).code() != ErrorCode::NotFound) return false;
        binding_ = binding;
        phase_ = Phase::Installing;
        return true;
    }

    // Success means the exact new root was committed and independently read
    // by the 05 handler, not an old ACK/ready or a successful later 06.
    void finishInstall(uint32_t session, bool success) {
        if (phase_ != Phase::Installing) return;
        WifiprovRekeyJournalPhase persisted;
        phase_ = success && live(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session) &&
            journal_.inspect(binding_, admin_, persisted) && persisted == WifiprovRekeyJournalPhase::AttemptReserved
            ? Phase::Installed : Phase::Closed;
    }

    Result handle(uint32_t session, ByteView request, MutableByteSpan response, size_t& written) {
        written = 0U;
        if (response.data == nullptr || response.size < responseSize) return Result::failure(ErrorCode::BufferTooSmall);
        using Status = DeviceKeyProvisioningStatus;
        const uint8_t version = request.data != nullptr && request.size >= 2U ? request.data[0] : 0U;
        const uint8_t operation = request.data != nullptr && request.size >= 2U ? request.data[1] : 0U;
        const auto status = [&](Status value) {
            response.data[0] = kDeviceKeyProvisioningVersion;
            response.data[1] = permitOperation;
            response.data[2] = static_cast<uint8_t>(value);
            written = 3U;
            return Result::success();
        };
        if (request.data == nullptr || request.size != requestSize) return status(Status::Malformed);
        if (version != kDeviceKeyProvisioningVersion) return status(Status::UnsupportedVersion);
        if (operation != permitOperation) return status(Status::UnsupportedOperation);
        if (phase_ != Phase::Installed || !live(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session))
            return status(Status::AccessRequired);

        uint8_t expected[security::kWifiprovRekeyBindingSize];
        if (!security::encodeWifiprovRekeyBinding(binding_, MutableByteSpan(expected, sizeof(expected))) ||
            !constantTimeEqual(ByteView(expected, sizeof(expected)), request.subspan(2U, sizeof(expected))))
            return status(Status::Conflict);
        DeviceKey key;
        Result result = access_.readDurableWifiProvisioningKeyVerified(durableRoot_, admin_.id(),
            binding_.previous.accessEpoch, admin_.version, binding_.newLocator, key);
        if (result) result = security::verifyWifiprovRekeyWifiPermit(key, binding_,
            request.subspan(2U + sizeof(expected), security::kWifiprovRekeyMacSize));
        clearDeviceKey(key);
        if (!result) return status(Status::AccessRequired);
        phase_ = Phase::Permitted;
        (void)status(Status::Success);
        memcpy(response.data + 3U, binding_.previous.operationId, sizeof(binding_.previous.operationId));
        written = responseSize;
        return Result::success();
    }

    bool allowStatus(uint32_t session) override {
        Esp32WiFiProvRekeyNativeOwner owner{};
        return nativeOwner_.current(Esp32WiFiProvRekeyEndpointKind::SdkConfig, session, owner);
    }
    bool beginSet(uint32_t session) override {
        if (phase_ != Phase::Permitted || !live(Esp32WiFiProvRekeyEndpointKind::SdkConfig, session)) return false;
        phase_ = Phase::Setting;
        return true;
    }
    void finishSet(uint32_t session, bool success) override {
        if (phase_ == Phase::Setting) phase_ = success && live(Esp32WiFiProvRekeyEndpointKind::SdkConfig, session)
            ? Phase::Set : Phase::Closed;
    }
    bool beginApply(uint32_t session) override {
        if (phase_ != Phase::Set || !live(Esp32WiFiProvRekeyEndpointKind::SdkConfig, session)) return false;
        phase_ = Phase::Closed;
        const bool consumed = journal_.consumeApply(binding_, admin_).ok();
        if (consumed) consumed_.store(true, std::memory_order_release);
        return consumed;
    }
    bool consumedThisWindow() const { return consumed_.load(std::memory_order_acquire); }

private:
    enum class Phase : uint8_t { Idle, Installing, Installed, Permitted, Setting, Set, Closed };

    bool live(Esp32WiFiProvRekeyEndpointKind endpoint, uint32_t session) const {
        Esp32WiFiProvRekeyNativeOwner owner{};
        WifiprovRekeyAdmin admin;
        return nativeOwner_.current(endpoint, session, owner) && owner.session == installer_.session &&
            owner.generation == installer_.generation && auth_.allows(endpoint, session, binding_.previous, admin) &&
            admin.version == admin_.version && constantTimeEqual(admin.id(), admin_.id());
    }

    Esp32WiFiProvSecurity1Owner& nativeOwner_;
    Esp32WiFiProvRekeyAdmin& auth_;
    DeviceAccessStore& access_;
    IAtomicBlobStore& durableRoot_;
    WifiprovRekeyJournal& journal_;
    Phase phase_ = Phase::Idle;
    std::atomic<bool> consumed_{false};
    Esp32WiFiProvRekeyNativeOwner installer_{};
    WifiprovRekeyAdmin admin_{};
    security::WifiprovRekeyBinding binding_{};
};

static_assert(Esp32WiFiProvRekeyApply::requestSize == 122U && Esp32WiFiProvRekeyApply::responseSize == 19U,
    "WiFiProv permit frame sizes");

} // namespace blinker

#endif

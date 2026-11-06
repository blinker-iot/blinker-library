#ifndef BLINKER_ESP32_WIFIPROV_REKEY_CANDIDATE_H
#define BLINKER_ESP32_WIFIPROV_REKEY_CANDIDATE_H

#include "Esp32Storage.h"
#include "Esp32WiFiProvRekeyApply.h"
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/runtime/TaskRootOwnerGate.h>

namespace blinker {

inline Esp32NvsBlobStoreConfig esp32RekeyNvsRecord(const char* name, const char* key, size_t size) {
    Esp32NvsBlobStoreConfig config;
    config.namespaceName = name;
    config.key = key;
    config.maximumSize = size;
    return config;
}

// A cold preflight, GetInfo, or a scan never creates the rekey namespace.
class Esp32WiFiProvRekeyJournalBlob final : public IAtomicBlobStore {
public:
    Esp32WiFiProvRekeyJournalBlob()
        : blob_(esp32RekeyNvsRecord("bl_v2_rekey", "attempt", WifiprovRekeyJournal::serializedSize)) {}
    Result load(MutableByteSpan output, size_t& written) override {
        written = 0U;
        Result result = blob_.beginReadOnly();
        if (result) result = blob_.load(output, written);
        blob_.end();
        return result;
    }
    Result replace(ByteView value) override {
        Result result = blob_.begin();
        if (result) result = blob_.replace(value);
        blob_.end();
        return result;
    }
    Result clear() override { return Result::failure(ErrorCode::UnsupportedFeature); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }

private:
    Esp32NvsAtomicBlobStore blob_;
};

// One opt-in BLE product window; ordinary WiFiProv never constructs this.
// The stack's Direct/Cloud owners are stopped until SDK shutdown completes.
// The restricted composition has no running task graph and additionally checks
// the original task NVS on every admitted authorization/write window.
class Esp32WiFiProvRekeyCandidate final : private IWifiprovRekeyOwnerGuard {
public:
    Esp32WiFiProvRekeyCandidate(const DeviceInstanceId& instance, DeviceAccessStore& access,
        DeviceKeyProvisioningEndpoint& legacy, WifiCredentialStore& network, IRandom& random,
        IX25519AesGcmCryptoProvider& crypto, const Esp32NvsBlobStoreConfig& rootConfig)
        : instance_(instance), access_(access), legacy_(legacy), network_(network), durable_(rootConfig),
          journal_(journalBlob_), auth_(native_, instance, access, random),
          endpoint_(instance, access, crypto, this), apply_(native_, auth_, access, durable_, journal_) {}

    static std::atomic<Esp32WiFiProvRekeyCandidate*>& active() {
        static std::atomic<Esp32WiFiProvRekeyCandidate*> value{nullptr};
        return value;
    }
    Esp32WiFiProvSecurity1Owner& native() { return native_; }
    IEsp32WiFiProvConfigFence& configFence() { return apply_; }

    static Result requireExistingInstance() {
        Esp32NvsAtomicBlobStore readOnly(esp32RekeyNvsRecord("bl_v2_id", "instance", DeviceInstanceIdStore::serializedSize));
        Result result = readOnly.beginReadOnly();
        DeviceInstanceId instance;
        if (result) result = DeviceInstanceIdStore(readOnly).load(instance);
        return result;
    }

    Result bootMayStartCloud() {
        WifiprovRekeyJournalRecord record;
        Result result = journal_.read(record);
        if (!result) return result.code() == ErrorCode::NotFound ? Result::success() : result;
        if (record.phase != WifiprovRekeyJournalPhase::Complete ||
            !sameDeviceInstanceId(instance_, record.binding.previous.instance)) return denied();
        result = durable_.beginReadOnly();
        if (result) result = verifyInstalled(record);
        durable_.end();
        return result && confirmedNetwork(record.networkGeneration) ? Result::success() : denied();
    }

    Result beginWindow() {
        if (used_ || !isValidDeviceInstanceId(instance_) || !rootAllowed()) return denied();
        WifiprovRekeyJournalRecord record;
        const Result read = journal_.read(record);
        if ((!read && read.code() != ErrorCode::NotFound) ||
            (read && !sameDeviceInstanceId(instance_, record.binding.previous.instance))) return denied();
        Result result = durable_.beginReadOnly();
        if (!result) return result;
        if (access_.hasChangeObserver(nullptr) == false) { durable_.end(); return denied(); }
        previousNetwork_ = network_.activeGeneration();
        access_.setChangeObserver(&rootGate_);
        native_.arm();
        Esp32WiFiProvRekeyCandidate* expected = nullptr;
        if (!active().compare_exchange_strong(expected, this, std::memory_order_release, std::memory_order_relaxed)) {
            native_.disarm();
            access_.setChangeObserver(nullptr);
            durable_.end();
            return denied();
        }
        used_ = true;
        return Result::success();
    }

    // Only after real SDK manager deinit, never on a timeout or ready flag.
    void endWindow() {
        Esp32WiFiProvRekeyCandidate* expected = this;
        if (!active().compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel, std::memory_order_relaxed)) return;
        native_.disarm();
        access_.setChangeObserver(nullptr);
        durable_.end();
    }

    bool acceptCredentials() {
        return apply_.consumedThisWindow() && !receivedCredentials_.test_and_set(std::memory_order_acq_rel);
    }
    bool consumedApply() const { return apply_.consumedThisWindow(); }

    // Product calls this only when the original SDK reported success and its
    // actual manager has exited; consumed Apply alone is never enough.
    Result finishSuccessfulWindow() {
        if (!apply_.consumedThisWindow()) return denied();
        WifiprovRekeyJournalRecord record;
        Result result = journal_.read(record);
        if (!result || record.phase != WifiprovRekeyJournalPhase::ApplyConsumed) return denied();
        result = verifyInstalled(record);
        const uint32_t generation = network_.activeGeneration();
        if (!result || generation == 0U || generation == previousNetwork_) return denied();
        // Confirm before starting Cloud: the ordinary Wi-Fi lifecycle must
        // not roll back to the old network and make this rotation look online.
        result = network_.confirmActive();
        if (!result || !confirmedNetwork(generation)) return denied();
        return journal_.complete(record.binding, record.admin, generation);
    }

    Result handle(uint32_t session, ByteView request, MutableByteSpan response, size_t& written) {
        written = 0U;
        const uint8_t op = request.data != nullptr && request.size >= 2U ? request.data[1] : 0U;
        const auto fail = [&](DeviceKeyProvisioningStatus status) {
            if (response.data == nullptr || response.size < kDeviceKeyProvisioningStatusResponseSize)
                return Result::failure(ErrorCode::BufferTooSmall);
            response.data[0] = kDeviceKeyProvisioningVersion;
            response.data[1] = op;
            response.data[2] = static_cast<uint8_t>(status);
            written = kDeviceKeyProvisioningStatusResponseSize;
            return Result::success();
        };
        using Status = DeviceKeyProvisioningStatus;
        if (request.data == nullptr || request.size < 2U) return fail(Status::Malformed);
        if (request.data[0] != kDeviceKeyProvisioningVersion) return fail(Status::UnsupportedVersion);
        Esp32WiFiProvRekeyNativeOwner owner{};
        if (!native_.current(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session, owner)) return fail(Status::AccessRequired);
        if (op == static_cast<uint8_t>(DeviceKeyProvisioningOperation::GetInfo)) {
            const Result result = legacy_.handle(request, response, written);
            if (result && written == kDeviceKeyProvisioningInfoResponseSize &&
                response.data[2] == static_cast<uint8_t>(Status::Success) &&
                (response.data[3] & (kDeviceKeyProvisioningHasDeviceKey | kDeviceKeyProvisioningHasAccessState)) ==
                    (kDeviceKeyProvisioningHasDeviceKey | kDeviceKeyProvisioningHasAccessState)) response.data[3] |= 0x08U;
            return result; // No legacy installed()/ready path in this window.
        }
        if (op == Esp32WiFiProvRekeyAdmin::nonceOperation || op == Esp32WiFiProvRekeyAdmin::authorizeOperation)
            return auth_.handle(session, request, response, written);
        if (op == Esp32WiFiProvRekeyApply::permitOperation) return apply_.handle(session, request, response, written);
        if (op < static_cast<uint8_t>(WifiprovRekeyOperation::Challenge) ||
            op > static_cast<uint8_t>(WifiprovRekeyOperation::ReadReceipt)) return fail(Status::UnsupportedOperation);

        session_ = session;
        const bool install = op == static_cast<uint8_t>(WifiprovRekeyOperation::Install);
        if (install && request.size == kWifiprovRekeyInstallRequestSize && response.size >= kWifiprovRekeySuccessResponseSize) {
            security::WifiprovRekeyBinding binding;
            if (!security::decodeWifiprovRekeyBinding(request.subspan(2U, security::kWifiprovRekeyBindingSize), binding))
                return fail(Status::Malformed);
            if (!apply_.beginInstall(session, binding)) return fail(Status::Conflict);
            const Result result = endpoint_.handle(request, response, written);
            apply_.finishInstall(session, result && written == kWifiprovRekeySuccessResponseSize && response.data[2] == 0U);
            return result;
        }
        return endpoint_.handle(request, response, written);
    }

private:
    static Result denied() { return Result::failure(ErrorCode::StateConflict); }
    static bool originalTaskStoreAbsent() {
        nvs_handle_t nvs = 0;
        const esp_err_t opened = nvs_open("bl_v2_tasks", NVS_READONLY, &nvs);
        if (opened == ESP_ERR_NVS_NOT_FOUND) return true;
        if (opened != ESP_OK) return false;
        size_t length = 0U;
        const esp_err_t observed = nvs_get_blob(nvs, "snapshot", nullptr, &length);
        nvs_close(nvs);
        return observed == ESP_ERR_NVS_NOT_FOUND;
    }
    bool rootAllowed() {
        size_t count = 0U;
        DeviceAccessAdminSelector admin;
        DeviceKey current;
        const bool good = originalTaskStoreAbsent() && access_.count(count) && count == 1U &&
            access_.loadAdminSelector(admin) && admin.accessEpoch != 0U && access_.load(current);
        clearDeviceKey(current);
        return good;
    }
    bool rootCertain() {
        if (!rootGate_.enterOwner()) return false;
        const bool certain = !rootGate_.uncertain();
        rootGate_.leaveOwner();
        return certain;
    }
    Result verifyInstalled(const WifiprovRekeyJournalRecord& record) {
        if (!rootCertain()) return denied();
        DeviceKey key;
        const Result result = access_.readDurableWifiProvisioningKeyVerified(durable_, record.admin.id(),
            record.binding.previous.accessEpoch, record.admin.version, record.binding.newLocator, key);
        clearDeviceKey(key);
        return result;
    }
    bool confirmedNetwork(uint32_t generation) {
        WifiCredentialProfile profile;
        const bool matched = network_.load(profile) && profile.generation == generation && profile.confirmed();
        clearWifiCredentialProfile(profile);
        return matched;
    }

    Result begin(WifiprovRekeyOperation operation, const security::WifiprovRekeyOldBinding& previous,
        const DeviceKeyLocator* locator, WifiprovRekeyAdmin& admin) override {
        const bool read = operation == WifiprovRekeyOperation::ReadReceipt;
        if (guardHeld_ || (!read && (!rootCertain() || !rootAllowed())) ||
            !auth_.allows(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session_, previous, admin)) return denied();
        guardedBinding_.previous = previous;
        if (locator != nullptr) guardedBinding_.newLocator = *locator;
        WifiprovRekeyJournalRecord record;
        const Result available = journal_.read(record);
        if (read) {
            WifiprovRekeyJournalPhase phase;
            if (locator == nullptr || !available || !journal_.inspect(guardedBinding_, admin, phase)) return denied();
        } else if (available.code() != ErrorCode::NotFound) return denied();
        guardedAdmin_ = admin;
        guardHeld_ = true;
        reserved_ = false;
        return Result::success();
    }
    Result reserveAttempt() override {
        if (!guardHeld_ || reserved_) return denied();
        const Result result = journal_.reserveAttempt(guardedBinding_, guardedAdmin_);
        reserved_ = result.ok();
        return result;
    }
    Result validateWifiprovRekeyCommit() override {
        WifiprovRekeyAdmin admin;
        return guardHeld_ && reserved_ &&
            auth_.allows(Esp32WiFiProvRekeyEndpointKind::BlinkerConfig, session_, guardedBinding_.previous, admin) &&
            admin.version == guardedAdmin_.version && constantTimeEqual(admin.id(), guardedAdmin_.id())
            ? Result::success() : denied();
    }
    Result openDurableReadback(IAtomicBlobStore*& storage) override {
        WifiprovRekeyJournalPhase phase;
        if (!guardHeld_ || !rootCertain() || !durable_.opened() ||
            !journal_.inspect(guardedBinding_, guardedAdmin_, phase)) return denied();
        storage = &durable_;
        return Result::success();
    }
    void end() override { guardHeld_ = false; reserved_ = false; }

    const DeviceInstanceId& instance_;
    DeviceAccessStore& access_;
    DeviceKeyProvisioningEndpoint& legacy_;
    WifiCredentialStore& network_;
    Esp32NvsAtomicBlobStore durable_;
    Esp32WiFiProvRekeyJournalBlob journalBlob_;
    WifiprovRekeyJournal journal_;
    TaskRootOwnerGate rootGate_;
    Esp32WiFiProvSecurity1Owner native_;
    Esp32WiFiProvRekeyAdmin auth_;
    WifiprovRekeyEndpoint endpoint_;
    Esp32WiFiProvRekeyApply apply_;
    security::WifiprovRekeyBinding guardedBinding_;
    WifiprovRekeyAdmin guardedAdmin_;
    std::atomic_flag receivedCredentials_ = ATOMIC_FLAG_INIT;
    uint32_t previousNetwork_ = 0U;
    uint32_t session_ = 0U;
    bool used_ = false;
    bool guardHeld_ = false;
    bool reserved_ = false;
};

} // namespace blinker

#endif

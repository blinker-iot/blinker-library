#include "WifiprovRekeyEndpoint.h"

#include <string.h>

namespace blinker {
namespace {

static_assert(static_cast<uint8_t>(WifiprovRekeyOperation::Challenge) >
    static_cast<uint8_t>(DeviceKeyProvisioningOperation::Bootstrap), "Dedicated rekey opcodes");
static_assert(kWifiprovRekeyChallengeRequestSize == 74U && kWifiprovRekeyInstallRequestSize == 150U &&
    kWifiprovRekeyReceiptRequestSize == 90U && kWifiprovRekeySuccessResponseSize == 51U,
    "WiFiProv rekey envelope sizes");

struct RekeyRequestLock {
    explicit RekeyRequestLock(std::atomic_flag& flag)
        : busy(flag), owned(!busy.test_and_set(std::memory_order_acquire)) {}
    ~RekeyRequestLock() { if (owned) busy.clear(std::memory_order_release); }
    std::atomic_flag& busy;
    const bool owned;
};

struct RekeyOwnerScope {
    explicit RekeyOwnerScope(IWifiprovRekeyOwnerGuard& guard) : owner(guard) {}
    ~RekeyOwnerScope() { owner.end(); }
    IWifiprovRekeyOwnerGuard& owner;
};

using RekeyStatus = DeviceKeyProvisioningStatus;
constexpr uint8_t statusByte(RekeyStatus status) { return static_cast<uint8_t>(status); }

Result writeRekeyStatus(uint8_t operation, uint8_t status, MutableByteSpan response, size_t& written) {
    response.data[0] = kDeviceKeyProvisioningVersion;
    response.data[1] = operation;
    response.data[2] = status;
    written = kDeviceKeyProvisioningStatusResponseSize;
    return Result::success();
}

size_t rekeyRequestSize(WifiprovRekeyOperation operation) {
    switch (operation) {
        case WifiprovRekeyOperation::Challenge: return kWifiprovRekeyChallengeRequestSize;
        case WifiprovRekeyOperation::Install: return kWifiprovRekeyInstallRequestSize;
        case WifiprovRekeyOperation::ReadReceipt: return kWifiprovRekeyReceiptRequestSize;
    }
    return 0U;
}

} // namespace

Result WifiprovRekeyEndpoint::readReceipt(const WifiprovRekeyAdmin& admin,
    const security::WifiprovRekeyBinding& binding, security::WifiprovRekeyMac& receipt) {
    IAtomicBlobStore* fresh = nullptr;
    Result result = guard_->openDurableReadback(fresh);
    if (!result) return result;
    if (fresh == nullptr) return Result::failure(ErrorCode::NotConfigured);
    DeviceKey current;
    result = access_.readDurableWifiProvisioningKeyVerified(*fresh, admin.id(), binding.previous.accessEpoch,
        admin.version, binding.newLocator, current);
    if (result) result = security::createWifiprovRekeyInstalledReceipt(current, binding, receipt);
    clearDeviceKey(current);
    return result;
}

Result WifiprovRekeyEndpoint::handle(ByteView request, MutableByteSpan response, size_t& written) {
    written = 0U;
    if (response.data == nullptr || response.size < kDeviceKeyProvisioningStatusResponseSize)
        return Result::failure(ErrorCode::BufferTooSmall);
    const uint8_t op = (request.data != nullptr && request.size >= 2U) ? request.data[1] : 0U;
    const auto status = [&](uint8_t code) { return writeRekeyStatus(op, code, response, written); };
    if (request.data == nullptr || request.size < 2U) return status(statusByte(RekeyStatus::Malformed));
    if (request.data[0] != kDeviceKeyProvisioningVersion) return status(statusByte(RekeyStatus::UnsupportedVersion));
    const auto operation = static_cast<WifiprovRekeyOperation>(op);
    const size_t expectedSize = rekeyRequestSize(operation);
    if (expectedSize == 0U) return status(statusByte(RekeyStatus::UnsupportedOperation));
    if (request.size != expectedSize) return status(statusByte(RekeyStatus::Malformed));
    if (guard_ == nullptr) return status(statusByte(RekeyStatus::AccessRequired));
    if (response.size < kWifiprovRekeySuccessResponseSize) return Result::failure(ErrorCode::BufferTooSmall);

    const bool challenge = operation == WifiprovRekeyOperation::Challenge;
    security::WifiprovRekeyBinding binding;
    Result result = challenge
        ? security::decodeWifiprovRekeyOldBinding(request.subspan(2U, security::kWifiprovRekeyOldBindingSize), binding.previous)
        : security::decodeWifiprovRekeyBinding(request.subspan(2U, security::kWifiprovRekeyBindingSize), binding);
    if (!result) return status(statusByte(RekeyStatus::Malformed));
    const auto& previous = binding.previous;
    if (!sameDeviceInstanceId(instance_, previous.instance)) return status(statusByte(RekeyStatus::AccessRequired));

    RekeyRequestLock lock(busy_);
    if (!lock.owned || (attempted_ && operation != WifiprovRekeyOperation::ReadReceipt))
        return status(statusByte(RekeyStatus::Conflict));
    WifiprovRekeyAdmin admin;
    result = guard_->begin(operation, previous, challenge ? nullptr : &binding.newLocator, admin);
    if (!result) return status(statusByte(RekeyStatus::AccessRequired));
    RekeyOwnerScope owner(*guard_);
    security::WifiprovRekeyMac proof;
    if (operation == WifiprovRekeyOperation::ReadReceipt) {
        if (!readReceipt(admin, binding, proof)) return status(kWifiprovRekeyOutcomeUnknown);
    } else {
        DeviceKey current;
        result = access_.loadWifiProvisioningKeyVerified(admin.id(), previous.accessEpoch, admin.version,
            previous.oldLocator, current);
        if (!result) return status(statusByte(RekeyStatus::AccessRequired));
        if (challenge) {
            result = security::createWifiprovRekeyChallengeProof(current, previous, proof);
            clearDeviceKey(current);
            if (!result) return status(statusByte(RekeyStatus::AccessRequired));
        } else {
            DeviceKey candidate;
            result = security::openWifiprovRekeyCandidate(current, binding,
                request.subspan(kWifiprovRekeyReceiptRequestSize, kAesGcmNonceSize),
                request.subspan(static_cast<size_t>(kWifiprovRekeyReceiptRequestSize) + kAesGcmNonceSize, security::kWifiprovRekeyBoxSize),
                crypto_, candidate);
            clearDeviceKey(current);
            if (!result) return status(statusByte(RekeyStatus::InvalidDeviceKey));
            attempted_ = true;
            result = guard_->reserveAttempt();
            if (!result) {
                clearDeviceKey(candidate);
                return status(result.code() == ErrorCode::StateConflict ? statusByte(RekeyStatus::Conflict) : kWifiprovRekeyOutcomeUnknown);
            }
            result = access_.replaceWifiProvisioningKeyVerified(admin.id(), previous.accessEpoch, admin.version,
                previous.oldLocator, candidate, *guard_);
            clearDeviceKey(candidate);
            if (!result || !readReceipt(admin, binding, proof)) return status(kWifiprovRekeyOutcomeUnknown);
        }
    }
    (void)status(statusByte(RekeyStatus::Success));
    memcpy(response.data + kDeviceKeyProvisioningStatusResponseSize, previous.operationId, sizeof(previous.operationId));
    memcpy(response.data + kDeviceKeyProvisioningStatusResponseSize + sizeof(previous.operationId), proof.bytes, sizeof(proof.bytes));
    written = kWifiprovRekeySuccessResponseSize;
    return Result::success();
}

} // namespace blinker

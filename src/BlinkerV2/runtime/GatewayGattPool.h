#ifndef BLINKER_RUNTIME_GATEWAYGATTPOOL_H
#define BLINKER_RUNTIME_GATEWAYGATTPOOL_H
#include "GatewayGattRadio.h"
#include "GatewayPresenceMatcher.h"
#include "../identity/GatewayChildRecords.h"
#include "../interface/IGattPermitJoinPortLease.h"
#include "../protocol/ble/Mode.h"

namespace blinker {

// Caller-owned bounded slots; no SDK, MQTT, credentials cache or work queue.
// A permit has a physical port and, if already bound, a separate target child.
template <typename Radio, typename Port>
class BasicGatewayGattPool final : public IGattPermitJoinPortLease, public IGatewayManagementAdmission {
public:
    struct Slot { Radio& radio; Port& port; GatewayChildRecords& records; };
    BasicGatewayGattPool(Slot* slots, size_t count, GatewayGattBootGuard& boot, IClock& clock)
        : slots_(slots), count_(count), boot_(boot), clock_(clock), borrowed_(count), target_(count),
          next_(0), grant_(count), scanSince_(0), online_(false), scanTimed_(false) {}

    void prepare(bool online) {
        online_ = online;
        (void)boot_.ready();
        if (grant_ < count_) {
            const auto state = slots_[grant_].port.state();
            if (state != BleCentralPortState::Stopped && state != BleCentralPortState::Idle) next_ = (grant_ + 1U) % count_;
        }
        bool initiating = false;
        for (size_t i = 0; i < count_; ++i) {
            slots_[i].radio.prepare(online, false);
            const auto state = slots_[i].port.state();
            if (state == BleCentralPortState::Scanning || state == BleCentralPortState::Connecting) initiating = true;
        }
        const bool scanning = borrowed_ < count_ && slots_[borrowed_].port.state() == BleCentralPortState::Scanning;
        if (scanning && !scanTimed_) { scanSince_ = clock_.monotonicMillis(); scanTimed_ = true; }
        if (!scanning) scanTimed_ = false;
        grant_ = count_;
        if (online && !initiating) {
            for (size_t n = 0; n < count_; ++n) {
                const size_t i = (next_ + n) % count_;
                if ((i == borrowed_ && slots_[i].port.state() == BleCentralPortState::Idle) || slots_[i].radio.wantsInitiation()) {
                    grant_ = i; break;
                }
            }
        }
        for (size_t i = 0; i < count_; ++i) slots_[i].radio.prepare(online, i == grant_);
    }

    void poll(bool online, uint32_t budgetMicros) {
        prepare(online);
        const uint32_t perSlot = count_ != 0U ? budgetMicros / static_cast<uint32_t>(count_) : 0U;
        for (size_t i = 0; i < count_; ++i) slots_[i].radio.poll(online, perSlot);
        if (grant_ < count_ && grant_ != borrowed_) next_ = (grant_ + 1U) % count_;
    }
    void stop() { online_ = false; for (size_t i = 0; i < count_; ++i) slots_[i].radio.stop(); }
    bool stopped() const {
        if (borrowed_ < count_) return false;
        for (size_t i = 0; i < count_; ++i) if (!slots_[i].radio.stopped()) return false;
        return true;
    }

    bool allowNewManagementWork() override { return online_ && boot_.ready(); }
    Result acquirePermitJoinPort(IBleCentralPort*& output) override {
        output = nullptr;
        if (!allowNewManagementWork()) return Result::failure(ErrorCode::WouldBlock);
        if (borrowed_ < count_) return Result::failure(ErrorCode::AlreadyExists);
        for (size_t n = 0; n < count_; ++n) {
            const size_t i = (next_ + n) % count_;
            if (slots_[i].radio.holdForPermit()) {
                borrowed_ = i; target_ = count_; output = &slots_[i].port;
                return Result::success();
            }
        }
        // Full means Busy, not an Opening waiter that evicts an active child.
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    void releasePermitJoinPort(IBleCentralPort& port) override {
        if (borrowed_ == count_ || &port != &slots_[borrowed_].port || port.state() != BleCentralPortState::Stopped) return;
        slots_[borrowed_].radio.returnPermit();
        if (target_ < count_ && target_ != borrowed_) slots_[target_].radio.returnPermit();
        borrowed_ = target_ = count_; scanTimed_ = false;
    }
    bool allowPermitJoinScan(IBleCentralPort& port) override {
        if (!online_ || borrowed_ == count_ || &port != &slots_[borrowed_].port) return false;
        if (port.state() == BleCentralPortState::Scanning) {
            // Time-slice only unselected discovery, never a live relay/ACL.
            if (!scanTimed_ || static_cast<uint32_t>(clock_.monotonicMillis() - scanSince_) < 2000U) return true;
            for (size_t i = 0; i < count_; ++i) if (i != borrowed_ && slots_[i].radio.wantsInitiation()) return false;
            return true;
        }
        return grant_ == borrowed_;
    }

    bool allowPermitJoinTarget(const BleCentralAdvertisement& advertisement) override {
        if (!online_ || borrowed_ == count_) return false;
        ble::ModeProfile profile;
        if (!ble::decodeModeServiceData(advertisement.modeServiceData, profile)) return false;
        if (profile.mode == ble::ApplicationMode::Provisioning) return target_ == count_;
        if (profile.mode != ble::ApplicationMode::Direct) return false;
        size_t matched = count_;
        for (size_t i = 0; i < count_; ++i) {
            const GatewayChildIdentity* identity = nullptr;
            if (!slots_[i].records.identity(identity)) return false;
            if (identity->state != GatewayChildRecordState::Active) {
                if (identity->hasRenewal || slots_[i].radio.directPromiseHolding()) return false;
                continue;
            }
            GatewayAccessRecord access;
            Result result = slots_[i].records.access.load(access);
            GatewayPresenceMatcher matcher;
            if (result) result = matcher.configure(access);
            clearGatewayAccessRecord(access);
            if (!result) return false;
            if (matcher.match(advertisement)) {
                if (matched != count_) return false;
                matched = i;
            }
        }
        if (target_ < count_) return matched == target_; // No re-targeting a borrowed native session.
        if (matched == count_) return true; // New child: App still proves its grant/receipt/identity.
        if (!slots_[matched].radio.permitTargetAllowed()) return false;
        if (matched != borrowed_ && !slots_[matched].radio.holdForPermit()) return false;
        target_ = matched;
        return true;
    }
private:
    Slot* slots_;
    size_t count_;
    GatewayGattBootGuard& boot_;
    IClock& clock_;
    size_t borrowed_, target_, next_, grant_;
    uint32_t scanSince_;
    bool online_, scanTimed_;
    BasicGatewayGattPool(const BasicGatewayGattPool&) = delete;
    BasicGatewayGattPool& operator=(const BasicGatewayGattPool&) = delete;
};
} // namespace blinker
#endif

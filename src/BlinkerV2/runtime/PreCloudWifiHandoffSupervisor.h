#ifndef BLINKER_RUNTIME_PRECLOUDWIFIHANDOFFSUPERVISOR_H
#define BLINKER_RUNTIME_PRECLOUDWIFIHANDOFFSUPERVISOR_H

#include "../identity/DeviceAccessStore.h"
#include "../identity/DeviceInstanceIdStore.h"
#include "../provisioning/InitialCloudWifiOnly.h"
#include "../provisioning/InitialCloudNetworkBaseline.h"
#include "../security/InitialCloudWifiHandoff.h"

namespace blinker {

class IPreCloudDirectRetirement {
public:
    virtual ~IPreCloudDirectRetirement() {}
    // Called by the original cooperative owner, after taking the tuple and
    // outside a BLE callback. First call retires the Runtime. Later calls may
    // only advance the same native retirement after a known WouldBlock.
    virtual Result retireDirect() = 0;
    // A native deinitialization proof, not a link/GATT state or App close.
    virtual bool shutdownComplete() const = 0;
};

class IPreCloudWifiOnlyReceiver {
public:
    virtual ~IPreCloudWifiOnlyReceiver() {}
    // One call, no credential write inside begin. Session binding is borrowed
    // only during begin; the guard lives until native cleanup completes.
    virtual Result beginWifiOnly(const security::InitialCloudKeyHandoff& handoff,
        IInitialCloudWifiWriteGuard& guard) = 0;
};

// Candidate-only cooperative owner. No manager, no ordinary client, no
// persistence or implicit restart. The product must keep one exclusive root
// owner across the instance/root reads and receiver journal call, and retire
// receiver callbacks before destroying the supervisor or its stores.
class PreCloudWifiHandoffSupervisor final : private IInitialCloudWifiWriteGuard {
public:
    PreCloudWifiHandoffSupervisor(security::IInitialCloudWifiHandoffSource& directEndpoint,
        IPreCloudDirectRetirement& direct, IPreCloudWifiOnlyReceiver& wifi,
        DeviceInstanceIdStore& instances, DeviceAccessStore& access, IClock& clock,
        uint32_t setupOpenedAtMillis, uint32_t setupWindowMillis,
        IInitialCloudNetworkBaseline* installedNetworkBaseline = nullptr)
        : directEndpoint_(directEndpoint), direct_(direct), wifi_(wifi), instances_(instances),
          access_(access), clock_(clock), setup_(setupOpenedAtMillis, setupWindowMillis),
          handoff_(), state_(State::Waiting),
          advancing_(false), writeChecked_(false), networkBaseline_(installedNetworkBaseline) {}
    ~PreCloudWifiHandoffSupervisor() { clearSessionBinding(); }
    PreCloudWifiHandoffSupervisor(const PreCloudWifiHandoffSupervisor&) = delete;
    PreCloudWifiHandoffSupervisor& operator=(const PreCloudWifiHandoffSupervisor&) = delete;

    Result advance() {
        if (advancing_) return Result::failure(ErrorCode::WouldBlock);
        if (state_ == State::WifiOnly)
            return handoffWindowLive() ? Result::success() : close(ErrorCode::StateConflict);
        if (state_ == State::Closed) return Result::failure(ErrorCode::StateConflict);
        ScopedAdvance advancing(advancing_);
        if (!setupWindowLive()) return close(ErrorCode::StateConflict);
        if (state_ == State::Waiting) {
            if (!directEndpoint_.takeAuthorizedHandoff(handoff_)) return Result::failure(ErrorCode::WouldBlock);
            state_ = State::RetiringDirect;
        }
        if (handoff_.purpose != security::InitialCloudWifiHandoffPurpose::KeyInstallation &&
            handoff_.purpose != security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork)
            return close(ErrorCode::UnsupportedFeature);
        if (handoff_.purpose == security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork && networkBaseline_ == nullptr)
            return close(ErrorCode::NotConfigured);
        if (!handoffWindowLive()) return close(ErrorCode::StateConflict);
        const Result retired = direct_.retireDirect();
        if (state_ != State::RetiringDirect || !handoffWindowLive()) return close(ErrorCode::StateConflict);
        if (!retired) {
            if (retired.code() == ErrorCode::WouldBlock) return retired;
            return close(retired.code());
        }
        if (!direct_.shutdownComplete()) return close(ErrorCode::StateConflict);

        const Result checked = validateRootAndWindow(State::RetiringDirect);
        if (!checked) return close(checked.code());
        if (!direct_.shutdownComplete()) return close(ErrorCode::StateConflict);
        state_ = State::StartingWifi;
        const Result started = wifi_.beginWifiOnly(handoff_, *this);
        clearSessionBinding();
        if (state_ != State::StartingWifi || !handoffWindowLive()) return close(ErrorCode::StateConflict);
        if (!started) return close(started.code());
        state_ = State::WifiOnly;
        return Result::success();
    }

    // Cancels authorization, not an SDK shutdown proof. A product keeps the
    // explicit responsibility for any native retirement/cleanup still needed.
    void cancel() { (void)close(ErrorCode::StateConflict); }
    bool waitingForDirectHandoff() const { return state_ == State::Waiting; }
    bool windowLive() const {
        return state_ == State::Waiting ? setupWindowLive() : state_ != State::Closed && handoffWindowLive();
    }

private:
    enum class State : uint8_t { Waiting, RetiringDirect, StartingWifi, WifiOnly, Closed };
    struct ScopedAdvance {
        explicit ScopedAdvance(bool& flag) : flag_(flag) { flag_ = true; }
        ~ScopedAdvance() { flag_ = false; }
        bool& flag_;
    };

    bool setupWindowLive() const {
        return setup_.live(clock_.monotonicMillis());
    }
    bool handoffWindowLive() const {
        return setupWindowLive() && clock_.monotonicMillis() - handoff_.authority.committedAtMillis <
            security::kInitialCloudKeyRegistrationWindowMillis;
    }
    Result validateRootAndWindow(State expected) {
        if (state_ != expected || !handoffWindowLive()) return Result::failure(ErrorCode::StateConflict);
        const security::InitialCloudKeyHandoffAuthority& original = handoff_.authority;
        DeviceInstanceId actual;
        Result result = instances_.load(actual);
        if (result && !sameDeviceInstanceId(actual, original.instance)) result = Result::failure(ErrorCode::StateConflict);
        if (!result) return result;
        if (state_ != expected || !handoffWindowLive()) return Result::failure(ErrorCode::StateConflict);
        result = access_.verifyInitialCloudWifiRoot(ByteView(original.adminId, sizeof(original.adminId)),
            original.epoch, original.credentialVersion, handoff_.locator);
        if (!result) return result;
        if (state_ != expected || !handoffWindowLive()) return Result::failure(ErrorCode::StateConflict);
        if (handoff_.purpose == security::InitialCloudWifiHandoffPurpose::InstalledKeyNetwork) {
            if (networkBaseline_ == nullptr) return Result::failure(ErrorCode::NotConfigured);
            result = networkBaseline_->validateEmptyNetwork();
            if (!result) return result;
        }
        return state_ == expected && handoffWindowLive() ? Result::success() : Result::failure(ErrorCode::StateConflict);
    }
    Result validateWifiWrite() override {
        if (state_ != State::WifiOnly || writeChecked_) return Result::failure(ErrorCode::StateConflict);
        writeChecked_ = true; // First attempted validation, even if its root read fails.
        if (!direct_.shutdownComplete()) return close(ErrorCode::StateConflict);
        const Result checked = validateRootAndWindow(State::WifiOnly);
        if (!checked) return close(checked.code());
        return Result::success();
    }
    Result close(ErrorCode error) {
        if (state_ != State::Closed) {
            state_ = State::Closed;
            clearSessionBinding();
            directEndpoint_.retire();
        }
        return Result::failure(error);
    }
    void clearSessionBinding() {
        secureZero(MutableByteSpan(handoff_.sessionBinding, sizeof(handoff_.sessionBinding)));
    }

    security::IInitialCloudWifiHandoffSource& directEndpoint_;
    IPreCloudDirectRetirement& direct_;
    IPreCloudWifiOnlyReceiver& wifi_;
    DeviceInstanceIdStore& instances_;
    DeviceAccessStore& access_;
    IClock& clock_;
    const security::InitialCloudKeySetupWindow setup_;
    security::InitialCloudKeyHandoff handoff_;
    State state_;
    bool advancing_, writeChecked_;
    IInitialCloudNetworkBaseline* networkBaseline_;
};

} // namespace blinker
#endif

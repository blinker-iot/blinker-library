#ifndef BLINKER_V2_ESP32_PRECLOUDWIFIHANDOFFPORTS_H
#define BLINKER_V2_ESP32_PRECLOUDWIFIHANDOFFPORTS_H

#include "Esp32NimBle.h"
#include <BlinkerV2/runtime/DeviceRuntime.h>
#include <BlinkerV2/runtime/PreCloudWifiHandoffSupervisor.h>

namespace blinker {

// For the dedicated Runtime only. Its first stop clears protocol state; a
// subsequent poll may continue the same known-pending native host release.
class Esp32PreCloudDirectRetirement final : public IPreCloudDirectRetirement {
public:
    Esp32PreCloudDirectRetirement(DeviceRuntime& runtime, Esp32NimBleLink& link)
        : runtime_(runtime), link_(link) {}

    Result retireDirect() override {
        if (stopping_) return link_.tryStop();
        stopping_ = true;
        runtime_.stop();
        if (link_.shutdownComplete()) return Result::success();
        const ErrorCode error = link_.lastError();
        return Result::failure(error == ErrorCode::Ok ? ErrorCode::StateConflict : error);
    }
    bool shutdownComplete() const override { return stopping_ && link_.shutdownComplete(); }

private:
    DeviceRuntime& runtime_;
    Esp32NimBleLink& link_;
    bool stopping_ = false;
};

} // namespace blinker
#endif

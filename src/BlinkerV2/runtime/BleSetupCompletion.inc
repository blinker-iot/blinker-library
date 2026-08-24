#include "BleSetupCompletion.h"

namespace blinker {

Result DeviceAccessSetupCompletion::load(bool& complete) {
    complete = false;
    uint32_t accessEpoch = 0U;
    Result result = access_.loadAccessEpoch(accessEpoch);
    if (result) {
        complete = accessEpoch != 0U;
    } else if (result.code() == ErrorCode::NotFound) {
        result = Result::success();
    }
    return result;
}

} // namespace blinker

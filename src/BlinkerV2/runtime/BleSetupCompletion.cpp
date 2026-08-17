#include "BleSetupCompletion.h"

namespace blinker {

Result LocalSetupSagaCompletion::load(bool& complete) {
    complete = false;
    LocalSetupSagaRecord record;
    Result result = store_.load(record);
    if (result) {
        complete = record.state == LocalSetupSagaState::Complete;
    } else if (result.code() == ErrorCode::NotFound) {
        result = Result::success();
    }
    clearLocalSetupSagaRecord(record);
    return result;
}

Result LocalControllerCredentialCompletion::load(bool& complete) {
    complete = false;
    size_t count = 0U;
    Result result = store_.count(count);
    for (size_t index = 0U; result && index < count; ++index) {
        ControllerCredential credential;
        result = store_.loadAt(index, credential);
        if (result &&
            credential.domain == ControllerCredentialDomain::Local &&
            credential.ownershipGeneration == 0U) {
            complete = true;
        }
        clearControllerCredential(credential);
        if (complete) break;
    }
    return result;
}

} // namespace blinker

#ifndef BLINKER_TEST_TASKAUTHORITYFIXTURE_H
#define BLINKER_TEST_TASKAUTHORITYFIXTURE_H
#include "BlinkerV2/runtime/TaskAuthority.h"

// Isolated test values, not an epoch conversion or production identity source.
inline blinker::TaskAuthorityBinding testAuthority() {
    blinker::TaskAuthorityBinding value;
    for (size_t i = 0U; i < 16U; ++i) {
        value.deviceInstanceId[i] = static_cast<uint8_t>(i + 1U);
        value.taskScope[i] = static_cast<uint8_t>(i + 33U);
    }
    for (size_t i = 0U; i < 32U; ++i) value.rootDigest[i] = static_cast<uint8_t>(i + 65U);
    return value;
}
inline blinker::TaskAuthorityBinding otherAuthority() {
    auto value = testAuthority();
    value.taskScope[15] ^= 1U; // Same first 120 bits; truncating scope must fail.
    return value;
}
#endif

#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTBUDGET_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTBUDGET_H

#include <stdint.h>

namespace blinker {

// Pin an authenticated command's remaining wall-clock lifetime once. The
// Proof/revoke/renewal phases default to 300 s; permit-join supplies its 900 s
// Service cap. Retries and SNTP rollback must not extend it; wall-clock expiry
// is still checked by the coordinator.
class GatewayManagementBudget {
public:
    GatewayManagementBudget() : started_(0U), duration_(0U) {}
    bool begin(uint32_t monotonic, uint64_t now, uint64_t expires,
               uint32_t maximumSeconds = 300U) {
        clear();
        if (now == 0U || expires <= now || expires - now > maximumSeconds ||
            expires - now > UINT32_MAX / 1000U) return false;
        started_ = monotonic;
        duration_ = static_cast<uint32_t>(expires - now) * 1000U;
        return true;
    }
    bool active(uint32_t now) const {
        return duration_ != 0U && static_cast<uint32_t>(now - started_) < duration_;
    }
    void clear() { started_ = duration_ = 0U; }
private:
    uint32_t started_;
    uint32_t duration_;
};

static_assert(sizeof(GatewayManagementBudget) == 8U, "Management budget must remain compact");

} // namespace blinker
#endif

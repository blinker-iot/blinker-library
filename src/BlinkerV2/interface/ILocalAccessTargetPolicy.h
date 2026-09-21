#ifndef BLINKER_INTERFACE_ILOCALACCESSTARGETPOLICY_H
#define BLINKER_INTERFACE_ILOCALACCESSTARGETPOLICY_H

#include "../protocol/local/Access.h"

namespace blinker {

// Product-owned scope check. Ordinary WiFi admits self only; a Hub adapter
// additionally matches one current child identity/epoch/topology. Discovery,
// IP addresses and BLE addresses never participate in this decision.
class ILocalAccessTargetPolicy {
public:
    virtual ~ILocalAccessTargetPolicy() {}
    virtual Result admit(const local_access::GrantView& grant) = 0;
};

} // namespace blinker

#endif

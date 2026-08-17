#ifndef BLINKER_INTERFACE_ISESSIONPROVIDER_H
#define BLINKER_INTERFACE_ISESSIONPROVIDER_H

#include "../control/Session.h"
#include "../core/Result.h"

namespace blinker {

enum class SessionProviderState : uint8_t {
    Stopped = 0,
    Idle,
    Requesting,
    Backoff,
    Ready,
    Failed
};

// Supplies short-lived MQTT session credentials without exposing how the
// device authenticates. start() may only stage work; DNS/TCP/TLS belongs in
// poll(). Credential views remain owned by the provider until the next
// successful generation or stop().
class ISessionProvider {
public:
    virtual ~ISessionProvider() {}

    virtual Result start() = 0;
    virtual Result requestRefresh() = 0;
    virtual void setNetworkAvailable(bool available) = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual SessionProviderState state() const = 0;
    virtual ErrorCode lastError() const = 0;
    virtual uint32_t credentialGeneration() const = 0;
    virtual Result credentials(SessionCredentials& output) const = 0;
    virtual void stop() = 0;
};

} // namespace blinker

#endif

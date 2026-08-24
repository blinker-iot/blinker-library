#ifndef BLINKER_INTERFACE_IAUTHORIZATIONPROVIDER_H
#define BLINKER_INTERFACE_IAUTHORIZATIONPROVIDER_H

#include "../core/Authorization.h"
#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/bbp2/Messages.h"

namespace blinker {

struct AuthorizationSessionContext {
    uint8_t transportId;
    uint32_t sessionId;
    bool encrypted;
    bool bonded;
    bool transportAuthenticated;

    AuthorizationSessionContext()
        : transportId(0),
          sessionId(0),
          encrypted(false),
          bonded(false),
          transportAuthenticated(false) {}
};

struct AuthorizationDecision {
    bbp2::AuthStatus status;
    uint32_t permissions;
    ByteView responsePayload;

    AuthorizationDecision()
        : status(bbp2::AuthStatus::Rejected),
          permissions(0U),
          responsePayload() {}
};

class IAuthorizationProvider {
public:
    virtual ~IAuthorizationProvider() {}

    virtual size_t methodCount() const = 0;
    virtual uint16_t methodAt(size_t index) const = 0;

    // decision.responsePayload is allowed only with Continue and must
    // reference provider-owned storage until the enclosing receive call
    // completes. Permissions are accepted only with Authorized; Runtime owns
    // attempt locking.
    virtual Result authorize(
        uint16_t method,
        ByteView requestPayload,
        const AuthorizationSessionContext& session,
        AuthorizationDecision& decision) = 0;
    // Called only after an Authorized result has been queued successfully.
    // Providers may use this non-fallible boundary to activate prepared
    // data-plane state. The default preserves providers without such state.
    virtual void authorizationResultQueued(
        const AuthorizationSessionContext& session) {
        (void)session;
    }
    virtual void sessionClosed(
        const AuthorizationSessionContext& session) = 0;
    virtual void resetSessions() = 0;
};

} // namespace blinker

#endif

#ifndef BLINKER_RUNTIME_GATEWAYGATTCHILDCONTEXT_H
#define BLINKER_RUNTIME_GATEWAYGATTCHILDCONTEXT_H

#include "GatewayGattChildSession.h"
#include "GatewayChildRouteBridge.h"
#include "GatewayProofCoordinator.h"
#include "GatewayRevocationCoordinator.h"
#include "GatewayCredentialRenewalCoordinator.h"
#include "GatewayRouteBinding.h"
#include "GatewayManagementControlMux.h"

namespace blinker {

// Internal composition for one active GATT child, not a product/lifecycle or
// SDK owner. The product supplies one slot port and durable record stores;
// WiFi/MQTT, clock, random and crypto stay shared. No duplicate Blinker Client.
// The owner must finish native retirement before destroying/reusing this context.
template <typename RouteEndpoint = GatewayRouteEndpoint,
          size_t FrameSize = 512U, size_t PacketSize = 20U>
struct BasicGatewayGattChildContext {
    enum : size_t { RecordSize = FrameSize + security::kDirectSecureOverhead };
    typedef BasicGatewayChildExecutor<GatewayGattChildSession> Executor;
    typedef BasicGatewayChildRouteBridge<GatewayGattChildSession, RouteEndpoint> Bridge;
    typedef BasicGatewayProofCoordinator<GatewayGattChildSession> Proof;
    typedef BasicGatewayRevocationCoordinator<Executor> Revocation;
    typedef BasicGatewayCredentialRenewalCoordinator<Executor> Renewal;

    GatewayAccessStore& access;
    GatewayCredentialRenewalStore& renewal;
    RouteEndpoint route;

private:
    uint8_t rx_[RecordSize], tx_[RecordSize], packet_[PacketSize];
    uint8_t handshake_[FrameSize], plaintext_[FrameSize], secureRecord_[RecordSize];

public:
    GattDirectChildLink link;
    GatewayGattChildSession session;
    BasicGatewayChildConnection<GatewayGattChildSession> connection;
    Executor executor;
    Proof proof;
    Bridge bridge;
    Revocation revocation;
    Renewal renewalCoordinator;
    GatewayManagementControlMux management;

    BasicGatewayGattChildContext(
        IBleCentralPort& port, GatewayChildRecords& records,
        GatewayRouteClient& routes, IClock& clock,
        IRandom& random, IX25519AesGcmCryptoProvider& crypto)
        : access(records.access), renewal(records.renewal),
          route(routes, &matchesRoute, &records),
          rx_(), tx_(), packet_(), handshake_(), plaintext_(), secureRecord_(),
          link(port, clock, MutableByteSpan(rx_, sizeof(rx_)), MutableByteSpan(tx_, sizeof(tx_)),
               MutableByteSpan(packet_, sizeof(packet_))),
          session(link, access, renewal, clock, random, crypto,
                  MutableByteSpan(handshake_, sizeof(handshake_)),
                  MutableByteSpan(plaintext_, sizeof(plaintext_)),
                  MutableByteSpan(secureRecord_, sizeof(secureRecord_))),
          connection(session), executor(session, connection),
          proof(session, connection, access, clock),
          bridge(executor, route, access, clock, random),
          revocation(executor, access, clock),
          renewalCoordinator(executor, access, renewal, clock),
          management(proof, revocation, renewalCoordinator) {
        executor.setControllerControlReceiver(
            &IGatewayManagementControl::controllerControlResponseThunk, &management);
    }

    ~BasicGatewayGattChildContext() {
        executor.setControllerControlReceiver(nullptr, nullptr);
    }

    BasicGatewayGattChildContext(const BasicGatewayGattChildContext&) = delete;
    BasicGatewayGattChildContext& operator=(const BasicGatewayGattChildContext&) = delete;

private:
    static bool matchesRoute(void* context, ByteView bytes) {
        return gatewayRouteMatchesChild(bytes, *static_cast<GatewayChildRecords*>(context));
    }
};

} // namespace blinker
#endif

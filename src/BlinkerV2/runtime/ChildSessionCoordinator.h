#ifndef BLINKER_RUNTIME_CHILDSESSIONCOORDINATOR_H
#define BLINKER_RUNTIME_CHILDSESSIONCOORDINATOR_H

#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "../security/ControllerHmacSha256Initiator.h"

namespace blinker {

enum : uint32_t {
    kChildSessionRequiredFeatures =
        bbp2::FeatureManifest |
        bbp2::FeatureEndpointIds |
        bbp2::FeatureAuthentication |
        bbp2::FeatureReliableDelivery |
        bbp2::FeatureStateRevision,
    kChildSessionOptionalFeatures =
        bbp2::FeatureTelemetry |
        bbp2::FeatureControllerControl,
    kChildSessionKnownFeatures =
        kChildSessionRequiredFeatures | kChildSessionOptionalFeatures
};

enum : size_t {
    // BBP/2 header + canonical AuthResult envelope + 53-byte challenge.
    kChildSessionMinimumFrameSize =
        bbp2::kBaseHeaderSize + 8U +
        security::kControllerAuthChallengePayloadSize
};

struct ChildSessionConfig {
    uint32_t features;
    uint32_t authenticationTimeoutMillis;
    uint16_t maxFrameSize;
    uint8_t reliableReceiveWindow;

    ChildSessionConfig()
        : features(kChildSessionRequiredFeatures),
          authenticationTimeoutMillis(15000U),
          maxFrameSize(512U),
          reliableReceiveWindow(4U) {}
};

enum class ChildSessionState : uint8_t {
    Idle = 0U,
    AwaitingHello,
    AwaitingChallenge,
    AwaitingAuthorization,
    Secure
};

// One transport-independent outbound child handshake. GATT/LAN code moves
// complete BBP/2 records only; this coordinator owns Hello negotiation,
// Method 2 ordering, the total authentication deadline and DirectSecure.
class ChildSessionCoordinator {
public:
    ChildSessionCoordinator(
        IRandom& random,
        const ChildSessionConfig& config = ChildSessionConfig());
    ~ChildSessionCoordinator();
    ChildSessionCoordinator(const ChildSessionCoordinator&) = delete;
    ChildSessionCoordinator& operator=(
        const ChildSessionCoordinator&) = delete;

    Result begin(
        const ControllerCredential& credential,
        uint32_t nowMillis,
        MutableByteSpan output,
        ByteView& frame);
    Result handleFrame(
        ByteView input,
        uint32_t nowMillis,
        MutableByteSpan output,
        ByteView& responseFrame);
    Result poll(uint32_t nowMillis);
    void reset();

    ChildSessionState state() const { return state_; }
    bool secure() const {
        return state_ == ChildSessionState::Secure && initiator_.authorized();
    }
    uint32_t negotiatedFeatures() const { return negotiatedFeatures_; }
    uint16_t negotiatedMaxFrameSize() const {
        return negotiatedMaxFrameSize_;
    }
    uint8_t remoteReliableReceiveWindow() const {
        return remoteReliableReceiveWindow_;
    }
    uint32_t manifestRevision() const { return manifestRevision_; }
    ByteView manifestFingerprint() const {
        return ByteView(
            manifestFingerprint_,
            hasManifest_ ? sizeof(manifestFingerprint_) : 0U);
    }
    security::DirectSecureSession& secureSession() {
        return initiator_.secureSession();
    }
    const security::DirectSecureSession& secureSession() const {
        return initiator_.secureSession();
    }

private:
    Result fail(ErrorCode error);
    Result validateConfig() const;
    Result writeHello(MutableByteSpan output, ByteView& frame);
    Result writeAuthenticate(
        ByteView payload,
        MutableByteSpan output,
        ByteView& frame);
    Result writeFrameHeader(
        bbp2::MessageKind kind,
        uint8_t flags,
        uint16_t sequence,
        size_t bodySize,
        MutableByteSpan output,
        ByteView& frame) const;
    Result handleHello(
        const bbp2::FrameView& frame,
        MutableByteSpan output,
        ByteView& responseFrame);
    Result handleChallenge(
        const bbp2::FrameView& frame,
        MutableByteSpan output,
        ByteView& responseFrame);
    Result handleAuthorization(const bbp2::FrameView& frame);
    bool deadlineReached(uint32_t nowMillis) const;

    ChildSessionConfig config_;
    security::ControllerHmacSha256Initiator initiator_;
    uint8_t authPayload_[security::kControllerAuthInitPayloadSize];
    uint8_t manifestFingerprint_[bbp2::kManifestFingerprintSize];
    uint32_t deadlineMillis_;
    uint32_t negotiatedFeatures_;
    uint32_t manifestRevision_;
    uint16_t negotiatedMaxFrameSize_;
    uint16_t sequence_;
    uint8_t remoteReliableReceiveWindow_;
    bool hasManifest_;
    ChildSessionState state_;
};

} // namespace blinker

#endif

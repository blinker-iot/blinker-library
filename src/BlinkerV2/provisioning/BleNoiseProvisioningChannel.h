#ifndef BLINKER_PROVISIONING_BLENOISEPROVISIONINGCHANNEL_H
#define BLINKER_PROVISIONING_BLENOISEPROVISIONINGCHANNEL_H

#include "../interface/IBleLink.h"
#include "../interface/IClock.h"
#include "../interface/INoiseCryptoProvider.h"
#include "../interface/IRandom.h"
#include "../interface/ITransportLifecycle.h"
#include "../protocol/noise/LocalSecureRecord.h"
#include "../security/NoiseNn.h"
#include "../transport/BleRecordTransport.h"

namespace blinker {

static const size_t kBleNoiseRecordSize =
    noise::kLocalSecureRecordHeaderSize + noise::kLocalSecureRecordMaxBodySize;
static const size_t kBleNoiseMaxHandshakePayloadSize =
    noise::kLocalSecureRecordMaxBodySize - security::kNoiseMaxHandshakeOverhead;
static const size_t kBleNoiseMaxTransportPayloadSize =
    noise::kLocalSecureRecordMaxBodySize - security::kNoiseTagSize;

enum class BleNoisePayloadKind : uint8_t {
    InitiatorHandshake = 1U,
    Transport = 2U
};

struct BleNoiseProvisioningConfig {
    noise::NoiseNnPattern pattern;
    uint32_t reassemblyTimeoutMillis;
    uint32_t secureSessionTimeoutMillis;
    uint8_t maxPacketsPerPoll;

    BleNoiseProvisioningConfig()
        // Gateway permit-join relays one ATT20 packet at a time through an
        // acknowledged cloud mailbox. A 2 s direct-BLE timeout races normal
        // network jitter; 10 s remains an inter-fragment idle bound while the
        // separate 60 s secure-session deadline limits slow peers.
        : pattern(noise::NoiseNnPattern::Nn), reassemblyTimeoutMillis(10000U),
          secureSessionTimeoutMillis(60000U), maxPacketsPerPoll(4U) {}
};

// Payload is borrowed and remains valid only for the callback. Applications
// must copy anything retained after return.
typedef void (*BleNoisePayloadReceiver)(void* context,
                                        BleNoisePayloadKind kind,
                                        ByteView payload,
                                        const RxContext& rx);

typedef void (*BleNoiseFaultHandler)(void* context,
                                     ErrorCode error,
                                     const RxContext& rx);

// Device/responder side of the local provisioning secure channel:
//
//   BLE 4-byte fragments -> B3 local record -> Noise NN/NNpsk0 -> payload
//
// The object accepts one BLE session at a time. All large buffers are caller
// owned: rxStorage and txStorage need kBleNoiseRecordSize bytes each;
// plaintextScratch needs kBleNoiseMaxTransportPayloadSize; packetScratch must
// hold one ATT payload. Outbound ciphertext is built directly in the single
// TX slot, avoiding a third 1028-byte record buffer. No raw PSK is retained
// after arm().
class BleNoiseProvisioningChannel : public ITransportLifecycle {
public:
    BleNoiseProvisioningChannel(IBleLink& link,
                                IClock& clock,
                                INoiseCryptoProvider& crypto,
                                IRandom& random,
                                MutableByteSpan rxStorage,
                                MutableByteSpan txStorage,
                                MutableByteSpan packetScratch,
                                MutableByteSpan plaintextScratch,
                                const BleNoiseProvisioningConfig& config =
                                    BleNoiseProvisioningConfig());
    ~BleNoiseProvisioningChannel();
    BleNoiseProvisioningChannel(const BleNoiseProvisioningChannel&) = delete;
    BleNoiseProvisioningChannel&
    operator=(const BleNoiseProvisioningChannel&) = delete;

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override { return records_.state(); }

    // Arms a fresh responder session. Ordinary NN requires an empty PSK;
    // enhanced NNpsk0 requires exactly 32 non-zero bytes. The pattern is
    // fixed in config and cannot be negotiated by a peer at runtime.
    Result arm(ByteView psk);
    void cancel();

    Result sendHandshakeResponse(ByteView payload);
    Result sendTransport(ByteView plaintext);
    Result handshakeHash(ByteView& output) const;

    bool armed() const { return armed_; }
    bool ready() const { return noise_.ready(); }
    noise::NoiseNnPattern pattern() const { return config_.pattern; }
    uint32_t activeSessionId() const { return activeSessionId_; }
    security::NoiseSessionState noiseState() const { return noise_.state(); }

    void setReceiver(BleNoisePayloadReceiver receiver, void* context);
    void setSessionHandlers(FrameSessionHandler connected,
                            FrameSessionHandler disconnected,
                            void* context);
    void setFaultHandler(BleNoiseFaultHandler handler, void* context);

private:
    static BleRecordFormat recordFormat();
    static BleRecordTransportConfig
    recordConfig(const BleNoiseProvisioningConfig& config);
    static void
    recordThunk(void* context, ByteView record, const RxContext& rx);
    static void connectedThunk(void* context, const RxContext& rx);
    static void disconnectedThunk(void* context, const RxContext& rx);
    static void faultThunk(void* context, ErrorCode error, const RxContext& rx);

    void onRecord(ByteView encoded, const RxContext& rx);
    void onConnected(const RxContext& rx);
    void onDisconnected(const RxContext& rx);
    void onRecordFault(ErrorCode error, const RxContext& rx);
    Result targetForActive(SendTarget& target) const;
    Result queueHandshake(ByteView payload);
    Result queueTransport(ByteView plaintext);
    void clearSecureState(bool resetRecord);
    void failActive(ErrorCode error, const RxContext& rx);
    void emitFault(ErrorCode error, const RxContext& rx);
    void wipeWorkspaces();

    IClock& clock_;
    MutableByteSpan rxStorage_;
    MutableByteSpan txStorage_;
    MutableByteSpan plaintextScratch_;
    BleNoiseProvisioningConfig config_;
    BleRecordTransport records_;
    security::NoiseNnSession noise_;
    BleNoisePayloadReceiver receiver_;
    void* receiverContext_;
    FrameSessionHandler sessionConnected_;
    FrameSessionHandler sessionDisconnected_;
    void* sessionContext_;
    BleNoiseFaultHandler faultHandler_;
    void* faultContext_;
    uint32_t activeSessionId_;
    uint32_t lastActivityMillis_;
    bool armed_;
};

} // namespace blinker

#endif

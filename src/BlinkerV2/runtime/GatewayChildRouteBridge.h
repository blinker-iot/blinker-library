#ifndef BLINKER_RUNTIME_GATEWAYCHILDROUTEBRIDGE_H
#define BLINKER_RUNTIME_GATEWAYCHILDROUTEBRIDGE_H

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "../interface/IRandom.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/Messages.h"
#include "GatewayChildExecutor.h"
#include "GatewayRouteClient.h"
#include "GatewayExecutionLease.h"
#include "GatewayDirectYieldLease.h"

namespace blinker {

// Cloud adapter for one child. It owns Cloud admission, demand lease and route
// correlation only; the injected executor is the sole southbound owner and is
// shared by future LAN or other authorized upstream adapters.
template <typename ChildSession, typename RouteEndpoint>
class BasicGatewayChildRouteBridge {
public:
    typedef BasicGatewayChildExecutor<ChildSession> Executor;

    BasicGatewayChildRouteBridge(
        Executor& executor,
        RouteEndpoint& route,
        GatewayAccessStore& access,
        IClock& clock,
        IRandom& random)
        : executor_(executor), route_(route), access_(access), clock_(clock),
          random_(random), execution_(), executionSentAt_(0U), executionAttempts_(0U),
          executionConfirmation_(0U),
          directYield_(), directSentAt_(0U), directSentControl_(0U), directSentChallenge_(0U),
          directSentOperation_(0U), directAttempts_(0U),
          topologyVersion_(0U), routeGeneration_(0U),
          admittedAccessRevision_(0U), projectionGeneration_(0U),
          helloStartedMillis_(0U), helloSize_(0U), helloAttempts_(0U),
          pendingCorrelation_(),
          pendingFrameDigest_(), recentCorrelation_(), recentFrameDigest_(),
          helloFrame_(), output_(),
          pendingRequestKind_(0U), pendingCloudSequence_(0U),
          recentRequestKind_(0U), recentCloudSequence_(0U),
          recentReplayable_(false), admitted_(false), cloudHelloSent_(false),
          awaitingChild_(false), closurePending_(false),
          lastError_(ErrorCode::Ok) {
        route_.setReceiver(
            &BasicGatewayChildRouteBridge::routeThunk, this);
        executor_.setObserver(
            &BasicGatewayChildRouteBridge::executorFrameThunk,
            &BasicGatewayChildRouteBridge::secureThunk,
            &BasicGatewayChildRouteBridge::disconnectedThunk,
            &BasicGatewayChildRouteBridge::faultThunk,
            this);
    }

    ~BasicGatewayChildRouteBridge() {
        route_.setReceiver(nullptr, nullptr);
        executor_.cancelDataOwner(this);
        executor_.clearObserver(this);
        reset();
    }

    void reset() {
        execution_.clear();
        executor_.setExecutionDemand(GatewayChildDemandCloud, false);
        directYield_.retire(clock_.monotonicMillis());
        directSentControl_ = directSentChallenge_ = 0U;
        directAttempts_ = 0U;
        executionAttempts_ = 0U;
        executionConfirmation_ = 0U;
        // Cloud admission is separate from the still-live child session.
        // Preserve the executor's sequence allocator and the projection
        // high-water mark; Cloud retirement cannot reset another upstream.
        topologyVersion_ = 0U;
        routeGeneration_ = 0U;
        admittedAccessRevision_ = 0U;
        admitted_ = false;
        cloudHelloSent_ = false;
        closurePending_ = false;
        clearHello();
        awaitingChild_ = false;
        pendingRequestKind_ = 0U;
        pendingCloudSequence_ = 0U;
        executor_.cancelDataOwner(this);
        lastError_ = ErrorCode::Ok;
        secureZero(MutableByteSpan(
            pendingCorrelation_, sizeof(pendingCorrelation_)));
        secureZero(MutableByteSpan(
            pendingFrameDigest_, sizeof(pendingFrameDigest_)));
        clearRecentRequest();
        secureZero(MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
        secureZero(MutableByteSpan(output_, sizeof(output_)));
    }

    // Maintain cloud authority/closure without acquiring the southbound radio.
    // A temporary permit-join owner is not a topology or admission reset.
    bool pollControlPlane() {
        if (!route_.subscribed()) { retireAdmission(); return false; }
        if (!admitted_) return false;
        const Result accessState = admittedAccessState();
        if (!accessState) {
            if (accessState.code() == ErrorCode::StateConflict) {
                retireAdmission();
            } else {
                fail(ErrorCode::AuthenticationRequired);
            }
            return false;
        }
        pollExecution();
        pollClosure();
        return true;
    }

    bool poll() {
        if (!pollControlPlane()) return false;
        if (executor_.secure()) {
            if (!cloudHelloSent_) onChildSecure();
            else pollHello();
        }
        // Admission is authority, not physical demand. Composition consumes
        // this result; no ordinary start/connect side effect remains here.
        return true;
    }

    bool admitted() const { return admitted_; }
    // Logical demand only. The physical scheduler is switched with idle/Direct
    // and management ownership, not by merely accepting this new wire format.
    bool executionWanted() {
        const bool wanted = admitted_ && route_.subscribed() &&
            admittedAccessState() &&
            execution_.wanted(clock_.monotonicMillis());
        executor_.setExecutionDemand(GatewayChildDemandCloud, wanted);
        return executor_.executionWanted();
    }
    bool directYieldHolding() {
        return directYield_.holding(clock_.monotonicMillis());
    }

    // Product must invoke this AFTER its sole radio arbitration, with true
    // only for native quiescence AND no management/permit owner AND a decision
    // not to reacquire while holding. This method never acquires/releases BLE.
    // Only the combined radio policy may supply this physical assertion.
    void pollDirectYield(bool quiescent) {
        if (!admitted_ || !route_.subscribed() || !admittedAccessState()) return;
        const uint32_t now = clock_.monotonicMillis();
        gateway::GatewayDirectYieldView control;
        if (!directYield_.challenge(now, control) && !directYield_.receipt(now, quiescent, control)) return;
        const uint8_t operation = static_cast<uint8_t>(control.demand.operation);
        if (directSentControl_ != control.controlId || directSentChallenge_ != control.demand.challengeId || directSentOperation_ != operation) {
            directSentControl_ = control.controlId; directSentChallenge_ = control.demand.challengeId;
            directSentOperation_ = operation; directAttempts_ = 0U;
        }
        if (directAttempts_ >= 5U || (directAttempts_ != 0U && static_cast<uint32_t>(now - directSentAt_) < 1000U)) return;
        ++directAttempts_; directSentAt_ = now;
        const Result result = publishExecutionControl(control.demand, control.controlId);
        if (!result) lastError_ = result.code();
    }
    uint32_t topologyVersion() const { return topologyVersion_; }
    ErrorCode lastError() const { return lastError_; }

private:
    static bool same(ByteView value, const uint8_t* bytes, size_t size) {
        return value.data != nullptr && value.size == size &&
               memcmp(value.data, bytes, size) == 0;
    }

    static bool same(StringView value, const char* text, size_t size) {
        return value.data != nullptr && value.size == size &&
               memcmp(value.data, text, size) == 0;
    }

    bool admissionMatches(
        const gateway::GatewayRouteAdmissionView& admission,
        const GatewayAccessRecord& record,
        uint64_t now) const {
        return now < admission.expiresAtUnixSeconds &&
               admission.expiresAtUnixSeconds ==
                   record.expiresAtUnixSeconds &&
               same(admission.operationId, record.operationId,
                    sizeof(record.operationId)) &&
               same(admission.childLogicalDeviceId,
                    record.childLogicalDeviceId,
                    record.childLogicalDeviceIdSize) &&
               same(admission.childDeviceInstanceId,
                    record.childDeviceInstanceId,
                    sizeof(record.childDeviceInstanceId)) &&
               admission.accessEpoch == record.accessEpoch &&
               admission.gatewayCredentialVersion ==
                   record.credentialVersion;
    }

    template <typename RouteIdentity>
    bool envelopeMatches(
        const RouteIdentity& envelope,
        const GatewayAccessRecord& record) const {
        return admitted_ &&
               routeGeneration_ == route_.subscriptionGeneration() &&
               record.deliveryRevision == admittedAccessRevision_ &&
               envelope.topologyVersion == topologyVersion_ &&
               same(envelope.childLogicalDeviceId,
                    record.childLogicalDeviceId,
                    record.childLogicalDeviceIdSize) &&
               same(envelope.childDeviceInstanceId,
                    record.childDeviceInstanceId,
                    sizeof(record.childDeviceInstanceId)) &&
               envelope.accessEpoch == record.accessEpoch &&
               envelope.gatewayCredentialVersion ==
                   record.credentialVersion;
    }

    Result admittedAccessState() {
        if (routeGeneration_ != route_.subscriptionGeneration()) {
            return Result::failure(ErrorCode::StateConflict);
        }
        GatewayAccessRecord record;
        uint64_t now = 0U;
        const Result result = access_.load(record);
        Result state = result;
        if (state && (!clock_.unixTime(now) || now == 0U ||
                      now >= record.expiresAtUnixSeconds)) {
            state = Result::failure(ErrorCode::AuthenticationRequired);
        }
        if (state && record.deliveryRevision != admittedAccessRevision_) {
            state = Result::failure(ErrorCode::StateConflict);
        }
        clearGatewayAccessRecord(record);
        return state;
    }

    void retireAdmission() {
        execution_.clear();
        executor_.setExecutionDemand(GatewayChildDemandCloud, false);
        directYield_.retire(clock_.monotonicMillis());
        executionAttempts_ = 0U;
        executionConfirmation_ = 0U;
        topologyVersion_ = 0U;
        routeGeneration_ = 0U;
        admittedAccessRevision_ = 0U;
        admitted_ = false;
        cloudHelloSent_ = false;
        closurePending_ = false;
        clearHello();
        executor_.cancelDataOwner(this);
        clearPendingRequest();
        clearRecentRequest();
        lastError_ = ErrorCode::Ok;
    }

    static bool allowedDownstream(uint8_t kind) {
        using bbp2::MessageKind;
        return kind == static_cast<uint8_t>(MessageKind::Hello) ||
               kind == static_cast<uint8_t>(MessageKind::ManifestRequest) ||
               kind == static_cast<uint8_t>(MessageKind::ManifestAccept) ||
               kind == static_cast<uint8_t>(MessageKind::StateRequest) ||
               kind == static_cast<uint8_t>(MessageKind::Command) ||
               kind == static_cast<uint8_t>(MessageKind::Ack) ||
               kind == static_cast<uint8_t>(MessageKind::Error);
    }

    static bool allowedUpstream(uint8_t kind) {
        using bbp2::MessageKind;
        return kind == static_cast<uint8_t>(MessageKind::Hello) ||
               kind == static_cast<uint8_t>(MessageKind::Manifest) ||
               kind == static_cast<uint8_t>(MessageKind::StatePage) ||
               kind == static_cast<uint8_t>(MessageKind::Patch) ||
               kind == static_cast<uint8_t>(MessageKind::Ack) ||
               kind == static_cast<uint8_t>(MessageKind::Error);
    }

    static bool expectsChildResponse(const bbp2::FrameView& frame) {
        if ((frame.header.flags & bbp2::FlagIsResponse) != 0U ||
            frame.header.sequence == 0U) {
            return false;
        }
        using bbp2::MessageKind;
        const MessageKind kind =
            static_cast<MessageKind>(frame.header.kind);
        return kind == MessageKind::Hello ||
               kind == MessageKind::ManifestRequest ||
               kind == MessageKind::ManifestAccept ||
               kind == MessageKind::StateRequest ||
               kind == MessageKind::Command;
    }

    void clearPendingRequest() {
        awaitingChild_ = false;
        pendingRequestKind_ = 0U;
        pendingCloudSequence_ = 0U;
        secureZero(MutableByteSpan(
            pendingCorrelation_, sizeof(pendingCorrelation_)));
        secureZero(MutableByteSpan(
            pendingFrameDigest_, sizeof(pendingFrameDigest_)));
    }

    void clearRecentRequest() {
        recentReplayable_ = false;
        recentRequestKind_ = 0U;
        recentCloudSequence_ = 0U;
        secureZero(MutableByteSpan(
            recentCorrelation_, sizeof(recentCorrelation_)));
        secureZero(MutableByteSpan(
            recentFrameDigest_, sizeof(recentFrameDigest_)));
    }

    bool rememberCompletedRequest() {
        using bbp2::MessageKind;
        const MessageKind kind =
            static_cast<MessageKind>(pendingRequestKind_);
        if (kind != MessageKind::Command &&
            kind != MessageKind::ManifestAccept) {
            return false;
        }
        memcpy(recentCorrelation_, pendingCorrelation_,
               sizeof(recentCorrelation_));
        memcpy(recentFrameDigest_, pendingFrameDigest_,
               sizeof(recentFrameDigest_));
        recentRequestKind_ = pendingRequestKind_;
        recentCloudSequence_ = pendingCloudSequence_;
        recentReplayable_ = true;
        return true;
    }

    Result requestExecution() {
        const Result result = execution_.request(clock_.monotonicMillis());
        if (result) executionAttempts_ = 0U;
        return result.code() == ErrorCode::AlreadyExists ? Result::success() : result;
    }

    void pollExecution() {
        const uint32_t now = clock_.monotonicMillis();
        if (!execution_.pending(now)) {
            if (execution_.challengeId() != 0U &&
                (!execution_.wanted(now) || static_cast<uint32_t>(now - executionSentAt_) < 10000U)) return;
            if (!requestExecution()) return;
        }
        if (executionAttempts_ >= 5U || (executionAttempts_ != 0U &&
            static_cast<uint32_t>(now - executionSentAt_) < 1000U)) return;
        ++executionAttempts_;
        executionSentAt_ = now;
        gateway::GatewayExecutionDemandView demand;
        demand.operation = gateway::GatewayExecutionOperation::Challenge;
        demand.incarnation = execution_.incarnation();
        demand.challengeId = execution_.challengeId();
        const Result result = publishExecutionControl(demand, 0U);
        if (!result) lastError_ = result.code();
    }

    Result publishExecutionControl(gateway::GatewayExecutionDemandView& demand, uint32_t directControlId) {
        GatewayAccessRecord record;
        Result result = access_.load(record);
        size_t written = 0U;
        if (result) {
            demand.topologyVersion = topologyVersion_;
            demand.childLogicalDeviceId = StringView(record.childLogicalDeviceId, record.childLogicalDeviceIdSize);
            demand.childDeviceInstanceId = ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
            demand.accessEpoch = record.accessEpoch;
            demand.gatewayCredentialVersion = record.credentialVersion;
            if (directControlId == 0U) {
                result = gateway::encodeGatewayExecutionDemand(demand, MutableByteSpan(output_, sizeof(output_)), written);
            } else {
                gateway::GatewayDirectYieldView direct; direct.demand = demand; direct.controlId = directControlId;
                result = gateway::encodeGatewayDirectYield(direct, MutableByteSpan(output_, sizeof(output_)), written);
            }
        }
        clearGatewayAccessRecord(record);
        if (result) result = route_.publish(ByteView(output_, written));
        secureZero(MutableByteSpan(output_, sizeof(output_)));
        return result;
    }

    void onRouteMessage(ByteView message) {
        if (message.data != nullptr && message.size != 0U && message.data[0] == 0x8cU) {
            gateway::GatewayDirectYieldView control;
            Result result = gateway::decodeGatewayDirectYield(message, control);
            if (!result) { lastError_ = result.code(); return; }
            GatewayAccessRecord record; uint64_t now = 0U;
            const bool valid = route_.subscribed() && access_.load(record) && clock_.unixTime(now) &&
                now != 0U && now < record.expiresAtUnixSeconds && envelopeMatches(control.demand, record);
            clearGatewayAccessRecord(record);
            if (!valid) { lastError_ = ErrorCode::StateConflict; return; }
            if (control.demand.operation == gateway::GatewayExecutionOperation::Probe) {
                result = directYield_.probe(control.controlId, control.demand.incarnation, clock_.monotonicMillis());
            } else result = directYield_.apply(control, clock_.monotonicMillis());
            lastError_ = result.code() == ErrorCode::AlreadyExists ? ErrorCode::Ok : result.code();
            return;
        }
        if (message.data != nullptr && message.size != 0U && message.data[0] == 0x8bU) {
            gateway::GatewayExecutionDemandView demand;
            Result result = gateway::decodeGatewayExecutionDemand(message, demand);
            if (!result) { lastError_ = result.code(); return; }
            GatewayAccessRecord record;
            uint64_t now = 0U;
            const bool valid = route_.subscribed() && access_.load(record) &&
                clock_.unixTime(now) && now != 0U && now < record.expiresAtUnixSeconds &&
                envelopeMatches(demand, record);
            clearGatewayAccessRecord(record);
            if (!valid) { lastError_ = ErrorCode::StateConflict; return; }
            if (demand.operation == gateway::GatewayExecutionOperation::Probe) result = requestExecution();
            else if (demand.operation == gateway::GatewayExecutionOperation::Decision) {
                result = execution_.apply(demand.incarnation, demand.challengeId, demand.leaseMillis, clock_.monotonicMillis());
                if (result) executionConfirmation_ = 3U;
            } else result = Result::failure(ErrorCode::InvalidArgument);
            // No start/connect/stop here, and no business sequence consumption.
            lastError_ = result.code();
            return;
        }
        // Admission is [2, 1, ...]; the read-only lease is [1, 3, ...].
        if (message.size >= 3U && message.data != nullptr &&
            message.data[0] == 0x8aU && message.data[1] == 1U && message.data[2] == 3U) {
            gateway::GatewayRouteLeaseView lease;
            Result result = gateway::decodeGatewayRouteLease(message, lease);
            if (!result) { lastError_ = result.code(); return; }
            GatewayAccessRecord record;
            uint64_t now = 0U;
            const bool valid = !lease.route.acknowledgment && executor_.secure() &&
                cloudHelloSent_ && !closurePending_ && route_.subscribed() &&
                lease.route.generation == projectionGeneration_ && access_.load(record) &&
                clock_.unixTime(now) && now != 0U && now < record.expiresAtUnixSeconds &&
                envelopeMatches(lease.route, record);
            clearGatewayAccessRecord(record);
            if (!valid) { lastError_ = ErrorCode::StateConflict; return; }
            lease.route.acknowledgment = true;
            size_t written = 0U;
            result = gateway::encodeGatewayRouteLease(lease, MutableByteSpan(output_, sizeof(output_)), written);
            if (result) result = route_.publish(ByteView(output_, written));
            secureZero(MutableByteSpan(output_, sizeof(output_)));
            lastError_ = result.code();
            return;
        }
        // A closure receipt belongs to the Hub/Broker lifecycle, including
        // while GATT is disconnected. Never send it into the child lane.
        if (message.data != nullptr && message.size != 0U && message.data[0] == 0x89U) {
            gateway::GatewayRouteClosureView closure;
            const Result decoded = gateway::decodeGatewayRouteClosure(message, closure);
            if (!decoded) { lastError_ = decoded.code(); return; }
            GatewayAccessRecord record;
            uint64_t now = 0U;
            const Result loaded = access_.load(record);
            const bool matches = loaded && clock_.unixTime(now) && now != 0U &&
                now < record.expiresAtUnixSeconds &&
                envelopeMatches(closure, record);
            clearGatewayAccessRecord(record);
            if (!matches || !closure.acknowledgment || !closurePending_ ||
                closure.generation != projectionGeneration_) {
                lastError_ = ErrorCode::StateConflict;
                return;
            }
            closurePending_ = false;
            lastError_ = ErrorCode::Ok;
            return;
        }
        gateway::GatewayRouteAdmissionView admission;
        Result result = gateway::decodeGatewayRouteAdmission(
            message, admission);
        if (result) {
            acceptAdmission(admission);
            return;
        }
        if (executor_.controllerControlPending()) {
            lastError_ = ErrorCode::StateConflict;
            return;
        }

        gateway::ChildRouteEnvelopeView envelope;
        result = gateway::decodeChildRouteEnvelope(message, envelope);
        if (!result || envelope.innerBbp2Frame.size >
                           gateway::kEdgeHubChildMaximumInnerFrameSize) {
            fail(result ? ErrorCode::CapacityExceeded : result.code());
            return;
        }
        bbp2::FrameView frame;
        result = bbp2::parseFrame(envelope.innerBbp2Frame, frame);
        if (!result || !allowedDownstream(frame.header.kind)) {
            fail(result ? ErrorCode::StateConflict : result.code());
            return;
        }

        GatewayAccessRecord record;
        uint64_t now = 0U;
        result = access_.load(record);
        const bool valid = result && clock_.unixTime(now) && now != 0U &&
            now < record.expiresAtUnixSeconds &&
            envelopeMatches(envelope, record);
        clearGatewayAccessRecord(record);
        if (!valid) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        uint64_t generation = 0U;
        for (size_t index = 0U; index < gateway::kRouteGenerationSize; ++index) {
            generation = (generation << 8U) | envelope.correlationId.data[index];
        }
        if (!cloudHelloSent_ || !executor_.secure() || generation != projectionGeneration_) {
            // An authenticated but delayed request belongs to another child
            // projection, not the successor's native/authentication attempt.
            // Drop it without retiring that attempt or queuing/retrying work.
            lastError_ = ErrorCode::StateConflict;
            return;
        }
        if (helloSize_ != 0U) {
            // A valid same-generation downlink confirms Broker receipt. The
            // Broker's lane, not this transport bridge, owns bootstrap ordering.
            // Its pending exchange now reuses the same correlation workspace.
            clearHello();
        }
        const bool expectsResponse = expectsChildResponse(frame);
        uint8_t frameDigest[kSha256Size] = {};
        if (expectsResponse) {
            result = sha256(
                envelope.innerBbp2Frame,
                MutableByteSpan(frameDigest, sizeof(frameDigest)));
            if (!result) {
                fail(result.code());
                return;
            }
            const bool pendingReplay = awaitingChild_ && same(
                envelope.correlationId,
                pendingCorrelation_, sizeof(pendingCorrelation_)) &&
                memcmp(frameDigest, pendingFrameDigest_,
                       sizeof(frameDigest)) == 0;
            if (awaitingChild_ && !pendingReplay) {
                secureZero(MutableByteSpan(
                    frameDigest, sizeof(frameDigest)));
                // Capacity pressure is not a broken authenticated session.
                // RouteEndpoint already owns one exact, bounded ingress slot;
                // retain the active exchange and let that slot retry later.
                lastError_ = ErrorCode::WouldBlock;
                return;
            }
            const bool completedReplay = !awaitingChild_ && recentReplayable_ && same(
                envelope.correlationId,
                recentCorrelation_, sizeof(recentCorrelation_)) &&
                memcmp(frameDigest, recentFrameDigest_,
                       sizeof(frameDigest)) == 0;
            if (pendingReplay || completedReplay) {
                // The executor owns the reliable-request replay cache and
                // reuses the exact child sequence without exposing it here.
                secureZero(MutableByteSpan(
                    frameDigest, sizeof(frameDigest)));
                result = executor_.replayDataFrame(
                    this, envelope.innerBbp2Frame, completedReplay);
                if (!result) {
                    if (result.code() == ErrorCode::WouldBlock) {
                        lastError_ = result.code();
                    } else {
                        fail(result.code());
                    }
                } else {
                    lastError_ = ErrorCode::Ok;
                }
                return;
            }
        }
        result = executor_.sendDataFrame(
            this, &BasicGatewayChildRouteBridge::executorFrameThunk,
            envelope.innerBbp2Frame);
        if (!result) {
            secureZero(MutableByteSpan(frameDigest, sizeof(frameDigest)));
            if (result.code() == ErrorCode::WouldBlock) {
                // The child bearer rejected the record before enqueue. Keep
                // DirectSecure and route admission alive. RouteEndpoint retains
                // the exact ingress frame for its bounded retry window.
                lastError_ = result.code();
            } else {
                fail(result.code());
            }
            return;
        }
        if (expectsResponse) {
            memcpy(pendingCorrelation_, envelope.correlationId.data,
                   sizeof(pendingCorrelation_));
            memcpy(pendingFrameDigest_, frameDigest,
                   sizeof(pendingFrameDigest_));
            pendingRequestKind_ = frame.header.kind;
            pendingCloudSequence_ = frame.header.sequence;
            awaitingChild_ = true;
        }
        secureZero(MutableByteSpan(frameDigest, sizeof(frameDigest)));
        lastError_ = ErrorCode::Ok;
    }

    void acceptAdmission(
        const gateway::GatewayRouteAdmissionView& admission) {
        GatewayAccessRecord record;
        uint64_t now = 0U;
        Result result = access_.load(record);
        const bool valid = result && clock_.unixTime(now) && now != 0U &&
                           admissionMatches(admission, record, now);
        const uint32_t accessRevision = record.deliveryRevision;
        clearGatewayAccessRecord(record);
        if (!valid) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        if (admitted_ && admission.topologyVersion < topologyVersion_) {
            fail(ErrorCode::StateConflict);
            return;
        }
        const uint32_t generation = route_.subscriptionGeneration();
        if (generation == 0U) {
            fail(ErrorCode::NotConnected);
            return;
        }
        if (admission.retiredGeneration > projectionGeneration_) {
            // The Broker cannot have observed a generation this bridge has
            // never allocated on the same MQTT identity. Do not jump forward.
            lastError_ = ErrorCode::StateConflict;
            return;
        }
        if (!admitted_ || routeGeneration_ != generation ||
            topologyVersion_ != admission.topologyVersion || admittedAccessRevision_ != accessRevision) {
            directYield_.retire(clock_.monotonicMillis());
            result = execution_.begin(random_);
            if (result) result = directYield_.begin(execution_.incarnation());
            directSentControl_ = directSentChallenge_ = 0U;
            directAttempts_ = 0U;
            executionAttempts_ = 0U;
            executionConfirmation_ = 1U;
            if (!result) { retireAdmission(); lastError_ = result.code(); return; }
        } else if (executionConfirmation_ == 1U && execution_.challengeId() != 0U &&
                   !execution_.pending(clock_.monotonicMillis())) {
            // One explicit admission retry may recover a lost first batch.
            // Same incarnation, fresh challenge/time anchor; no polling loop,
            // no reset by duplicates while pending, no third confirmation batch.
            if (requestExecution()) executionConfirmation_ = 2U;
        }
        if (routeGeneration_ != generation ||
            topologyVersion_ != admission.topologyVersion ||
            admittedAccessRevision_ != accessRevision ||
            (cloudHelloSent_ && admission.retiredGeneration == projectionGeneration_)) {
            // The topology is durable, but route admission and the BBP/2
            // child projection are scoped to one MQTT connection. A fast
            // credential rebind may never expose CloudConnecting to the
            // product scheduler, so the connection generation is the only
            // authoritative boundary.
            cloudHelloSent_ = false;
            closurePending_ = false;
            clearHello();
            executor_.cancelDataOwner(this);
            clearPendingRequest();
            clearRecentRequest();
        }
        topologyVersion_ = admission.topologyVersion;
        routeGeneration_ = generation;
        admittedAccessRevision_ = accessRevision;
        admitted_ = true;
        lastError_ = ErrorCode::Ok;
    }

    void onChildFrame(
        ByteView innerFrame,
        GatewayChildFrameSource source,
        bool syntheticHello = false) {
        if (!executor_.secure()) return;
        if (innerFrame.size > gateway::kEdgeHubChildMaximumInnerFrameSize) {
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        bbp2::FrameView frame;
        Result result = bbp2::parseFrame(innerFrame, frame);
        if (!result) {
            fail(result.code());
            return;
        }
        if (!admitted_ || !cloudHelloSent_ || !route_.subscribed() ||
            routeGeneration_ != route_.subscriptionGeneration()) {
            // A management-only DirectSecure session has no ordinary route,
            // but the child may still emit its negotiated Hello/state frames.
            // They are authenticated yet not authorized for cloud routing:
            // consume them locally instead of reopening route/up or tearing
            // down the control session before its response can arrive.
            return;
        }
        if (!allowedUpstream(frame.header.kind)) {
            fail(ErrorCode::UnsupportedFeature);
            return;
        }

        const bool pendingResponse =
            source == GatewayChildFrameSource::PendingResponse;
        const bool recentResponse =
            source == GatewayChildFrameSource::RecentResponse;
        const bbp2::MessageKind kind =
            static_cast<bbp2::MessageKind>(frame.header.kind);
        if (!pendingResponse && !recentResponse &&
            (kind == bbp2::MessageKind::Ack ||
             kind == bbp2::MessageKind::Error)) {
            // ACK/Error are responses, never autonomous child events. A late
            // response outside the bounded translation window cannot be
            // correlated safely and must not be forwarded with a random ID.
            return;
        }
        uint8_t correlation[gateway::kCorrelationIdSize];
        if (syntheticHello || pendingResponse) {
            memcpy(correlation, pendingCorrelation_, sizeof(correlation));
        } else if (recentResponse) {
            memcpy(correlation, recentCorrelation_, sizeof(correlation));
        } else {
            result = random_.fill(MutableByteSpan(
                correlation, sizeof(correlation)));
            if (!result) {
                fail(result.code());
                return;
            }
            for (size_t index = 0U; index < gateway::kRouteGenerationSize; ++index) {
                correlation[index] = static_cast<uint8_t>(
                    projectionGeneration_ >> ((gateway::kRouteGenerationSize - 1U - index) * 8U));
            }
        }

        GatewayAccessRecord record;
        uint64_t now = 0U;
        result = access_.load(record);
        if (!result || !clock_.unixTime(now) || now == 0U ||
            now >= record.expiresAtUnixSeconds) {
            clearGatewayAccessRecord(record);
            secureZero(MutableByteSpan(correlation, sizeof(correlation)));
            fail(ErrorCode::AuthenticationRequired);
            return;
        }
        if (record.deliveryRevision != admittedAccessRevision_) {
            clearGatewayAccessRecord(record);
            secureZero(MutableByteSpan(correlation, sizeof(correlation)));
            // Credential promotion is durable before the Service commits the
            // matching topology version. Keep DirectSecure available to the
            // management transaction, but fence the stale data-plane
            // admission until the Broker sends its new RouteAdmission.
            retireAdmission();
            return;
        }
        ByteView routedFrame = innerFrame;
        if (pendingResponse || recentResponse) {
            result = executor_.mapResponseSequence(
                innerFrame,
                pendingResponse ? pendingCloudSequence_
                                : recentCloudSequence_,
                routedFrame);
            if (!result) {
                clearGatewayAccessRecord(record);
                secureZero(MutableByteSpan(correlation, sizeof(correlation)));
                fail(result.code());
                return;
            }
        }
        gateway::ChildRouteEnvelopeView envelope;
        envelope.topologyVersion = topologyVersion_;
        envelope.childLogicalDeviceId = StringView(
            record.childLogicalDeviceId,
            record.childLogicalDeviceIdSize);
        envelope.childDeviceInstanceId = ByteView(
            record.childDeviceInstanceId,
            sizeof(record.childDeviceInstanceId));
        envelope.accessEpoch = record.accessEpoch;
        envelope.gatewayCredentialVersion = record.credentialVersion;
        envelope.correlationId = ByteView(
            correlation, sizeof(correlation));
        envelope.innerBbp2Frame = routedFrame;
        size_t written = 0U;
        result = gateway::encodeChildRouteEnvelope(
            envelope, MutableByteSpan(output_, sizeof(output_)), written);
        clearGatewayAccessRecord(record);
        secureZero(MutableByteSpan(correlation, sizeof(correlation)));
        if (result) result = route_.publish(ByteView(output_, written));
        secureZero(MutableByteSpan(output_, sizeof(output_)));
        if (pendingResponse) {
            const bool replayable = rememberCompletedRequest();
            executor_.completeDataRequest(this, replayable);
            clearPendingRequest();
        }
        if (!result) {
            if (syntheticHello) lastError_ = result.code();
            else fail(result.code());
            return;
        }
        lastError_ = ErrorCode::Ok;
    }

    void clearHello() {
        if (helloSize_ != 0U) secureZero(MutableByteSpan(pendingCorrelation_, sizeof(pendingCorrelation_)));
        helloSize_ = 0U;
        helloAttempts_ = 0U;
        helloStartedMillis_ = 0U;
        secureZero(MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
    }

    void pollHello() {
        if (helloSize_ == 0U || helloAttempts_ == 9U) return;
        const uint32_t elapsed = clock_.monotonicMillis() - helloStartedMillis_;
        if (elapsed >= 23000U) {
            helloAttempts_ = 9U;
            lastError_ = ErrorCode::NotConnected;
            return;
        }
        // Two bounded rounds, anchored only after actual DirectSecure exists.
        // Authority retries, owner renewal and duplicate Hello never re-anchor.
        const uint32_t due[] = {0U, 1000U, 3000U, 7000U, 12000U, 13000U, 15000U, 19000U};
        if (helloAttempts_ >= 8U || elapsed < due[helloAttempts_]) return;
        ++helloAttempts_;
        onChildFrame(
            ByteView(helloFrame_, helloSize_),
            GatewayChildFrameSource::Event,
            true);
    }

    // Reconstruct the small, exact notice in the existing outbound workspace;
    // no extra frame queue and no publish from a BLE disconnect callback.
    // Reuses Hello's timer while no Hello is active. A fresh Hello supersedes
    // closure delivery and must never wait for the receipt to reconnect.
    void pollClosure() {
        if (!closurePending_ || !admitted_ || !route_.subscribed()) return;
        const uint32_t elapsed = clock_.monotonicMillis() - helloStartedMillis_;
        if (elapsed >= 11000U) { closurePending_ = false; return; }
        const uint32_t due[] = {0U, 1000U, 3000U, 7000U};
        if (helloAttempts_ >= 4U || elapsed < due[helloAttempts_]) return;
        ++helloAttempts_; // Failed publish consumes the same bounded attempt.
        GatewayAccessRecord record;
        Result result = access_.load(record);
        if (result) {
            gateway::GatewayRouteClosureView closure;
            closure.topologyVersion = topologyVersion_;
            closure.childLogicalDeviceId = StringView(record.childLogicalDeviceId, record.childLogicalDeviceIdSize);
            closure.childDeviceInstanceId = ByteView(record.childDeviceInstanceId, sizeof(record.childDeviceInstanceId));
            closure.accessEpoch = record.accessEpoch;
            closure.gatewayCredentialVersion = record.credentialVersion;
            closure.generation = projectionGeneration_;
            size_t written = 0U;
            result = gateway::encodeGatewayRouteClosure(closure, MutableByteSpan(output_, sizeof(output_)), written);
            if (result) result = route_.publish(ByteView(output_, written));
        }
        clearGatewayAccessRecord(record);
        lastError_ = result.code();
    }

    void onChildSecure() {
        if (!admitted_ || cloudHelloSent_ || !executor_.secure()) return;
        if (closurePending_ && admittedAccessState()) pollClosure();
        closurePending_ = false;
        if (projectionGeneration_ == UINT64_MAX) {
            retireAdmission();
            lastError_ = ErrorCode::CapacityExceeded;
            return;
        }
        ++projectionGeneration_;
        const ByteView fingerprint = executor_.manifestFingerprint();
        if (fingerprint.size != bbp2::kManifestFingerprintSize) {
            fail(ErrorCode::ProtocolError);
            return;
        }
        bbp2::HelloBody hello;
        hello.role = bbp2::PeerRole::Device;
        hello.versions[0] = bbp2::kVersion;
        hello.versionCount = 1U;
        hello.features = executor_.negotiatedFeatures() | bbp2::FeatureGatewayRouteGeneration |
            bbp2::FeatureGatewayRouteLease;
        hello.maxFrameSize = executor_.negotiatedMaxFrameSize();
        hello.maxReassemblySize = hello.maxFrameSize;
        hello.manifestRevision = executor_.manifestRevision();
        hello.hasManifestRevision = true;
        hello.manifestFingerprint = fingerprint;
        hello.hasManifestFingerprint = true;
        if ((hello.features & bbp2::FeatureAuthentication) != 0U) {
            hello.authorizationMethods[0] =
                security::kControllerHmacSha256Method;
            hello.authorizationMethodCount = 1U;
        }
        if ((hello.features & bbp2::FeatureReliableDelivery) != 0U) {
            hello.reliableReceiveWindow =
                executor_.remoteReliableReceiveWindow();
        }
        ByteView body;
        Result result = bbp2::encodeHelloBody(
            hello,
            MutableByteSpan(
                helloFrame_ + bbp2::kBaseHeaderSize,
                sizeof(helloFrame_) - bbp2::kBaseHeaderSize),
            body);
        if (result) {
            bbp2::FrameHeader header;
            header.kind = static_cast<uint8_t>(bbp2::MessageKind::Hello);
            header.flags = bbp2::FlagAckRequired;
            header.sequence = 1U;
            header.bodyLength = static_cast<uint16_t>(body.size);
            result = bbp2::encodeHeader(
                header, MutableByteSpan(helloFrame_, sizeof(helloFrame_)));
        }
        if (!result) {
            fail(result.code());
            return;
        }
        result = random_.fill(MutableByteSpan(pendingCorrelation_, sizeof(pendingCorrelation_)));
        if (!result) { fail(result.code()); return; }
        for (size_t index = 0U; index < gateway::kRouteGenerationSize; ++index) {
            pendingCorrelation_[index] = static_cast<uint8_t>(projectionGeneration_ >> ((7U - index) * 8U));
        }
        helloSize_ = static_cast<uint16_t>(bbp2::kBaseHeaderSize + body.size);
        helloAttempts_ = 0U;
        helloStartedMillis_ = clock_.monotonicMillis();
        cloudHelloSent_ = true;
        pollHello();
    }

    void fail(ErrorCode error) {
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::ProtocolError
                         : error;
        executor_.cancelDataOwner(this);
        clearPendingRequest();
        clearRecentRequest();
        executor_.retire(lastError_);
    }

    static Result routeThunk(void* context, ByteView message) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self == nullptr) return Result::failure(ErrorCode::NotConfigured);
        self->onRouteMessage(message);
        return Result(self->lastError_);
    }
    static void executorFrameThunk(
        void* context,
        ByteView frame,
        GatewayChildFrameSource source) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->onChildFrame(frame, source);
    }
    static void secureThunk(void* context) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) self->onChildSecure();
    }
    static void disconnectedThunk(void* context) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) {
            const bool notify = self->admitted_ && self->cloudHelloSent_ &&
                self->route_.subscribed() &&
                self->routeGeneration_ == self->route_.subscriptionGeneration();
            self->cloudHelloSent_ = false;
            // A repeated disconnect callback cannot restart a closure budget.
            if (!self->closurePending_) self->clearHello();
            if (notify) {
                self->closurePending_ = true;
                self->helloStartedMillis_ = self->clock_.monotonicMillis();
                self->helloAttempts_ = 0U;
            }
            self->clearPendingRequest();
            self->clearRecentRequest();
        }
    }
    static void faultThunk(
        void* context,
        ErrorCode error) {
        BasicGatewayChildRouteBridge* self =
            static_cast<BasicGatewayChildRouteBridge*>(context);
        if (self != nullptr) {
            self->lastError_ = error;
            self->clearPendingRequest();
            self->clearRecentRequest();
        }
    }

    Executor& executor_;
    RouteEndpoint& route_;
    GatewayAccessStore& access_;
    IClock& clock_;
    IRandom& random_;
    GatewayExecutionLease execution_;
    uint32_t executionSentAt_;
    uint8_t executionAttempts_;
    uint8_t executionConfirmation_; // 0=none, 1=first, 2=retry, 3=confirmed
    GatewayDirectYieldLease directYield_;
    uint32_t directSentAt_, directSentControl_, directSentChallenge_;
    uint8_t directSentOperation_, directAttempts_;
    uint32_t topologyVersion_;
    uint32_t routeGeneration_;
    uint32_t admittedAccessRevision_;
    uint64_t projectionGeneration_;
    uint32_t helloStartedMillis_;
    uint16_t helloSize_;
    uint8_t helloAttempts_;
    uint8_t pendingCorrelation_[gateway::kCorrelationIdSize];
    uint8_t pendingFrameDigest_[kSha256Size];
    uint8_t recentCorrelation_[gateway::kCorrelationIdSize];
    uint8_t recentFrameDigest_[kSha256Size];
    uint8_t helloFrame_[128U];
    uint8_t output_[gateway::kEdgeHubChildRouteEnvelopeMaximumEncodedSize];
    uint8_t pendingRequestKind_;
    uint16_t pendingCloudSequence_;
    uint8_t recentRequestKind_;
    uint16_t recentCloudSequence_;
    bool recentReplayable_;
    bool admitted_;
    bool cloudHelloSent_;
    bool awaitingChild_;
    bool closurePending_;
    ErrorCode lastError_;

    BasicGatewayChildRouteBridge(
        const BasicGatewayChildRouteBridge&);
    BasicGatewayChildRouteBridge& operator=(
        const BasicGatewayChildRouteBridge&);
};

} // namespace blinker

#endif

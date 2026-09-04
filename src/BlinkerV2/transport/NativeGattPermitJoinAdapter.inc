#include "NativeGattPermitJoinAdapter.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

NativeGattPermitJoinAdapter::NativeGattPermitJoinAdapter(
    IBleCentralPort& port,
    IClock& clock,
    IGatewayPermitJoinPortLease& lease,
    const NativeGattPermitJoinConfig& config)
    : port_(port), clock_(clock), lease_(lease), config_(config),
      candidates_(), rx_(), selectedLocator_(), rxHead_(0U),
      rxTail_(0U), rxCount_(0U),
      nextAttemptId_(0U), activeAttemptId_(0U),
      connectedAttemptId_(0U), droppedPackets_(0U), maxPacketSize_(0U),
      state_(GatewayPermitJoinAdapterState::Stopped),
      lastError_(ErrorCode::Ok), leased_(false), selectionSet_(false) {}

NativeGattPermitJoinAdapter::~NativeGattPermitJoinAdapter() {
    closeWindow();
}

bool NativeGattPermitJoinAdapter::validConfig() const {
    return config_.candidateTtlMillis != 0U;
}

Result NativeGattPermitJoinAdapter::openWindow() {
    if (state_ != GatewayPermitJoinAdapterState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result result = lease_.acquirePermitJoinPort();
    if (!result) return result;
    leased_ = true;
    if (port_.state() != BleCentralPortState::Stopped) {
        closeWindow();
        return Result::failure(ErrorCode::WouldBlock);
    }

    port_.setPacketReceiver(&NativeGattPermitJoinAdapter::packetThunk, this);
    port_.setConnectionHandlers(
        &NativeGattPermitJoinAdapter::connectedThunk,
        &NativeGattPermitJoinAdapter::disconnectedThunk,
        this);
    result = port_.start();
    if (result) result = startScan();
    if (!result) {
        closeWindow();
        return result;
    }
    state_ = GatewayPermitJoinAdapterState::Ready;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

void NativeGattPermitJoinAdapter::closeWindow() {
    if (!leased_) return;
    port_.setPacketReceiver(nullptr, nullptr);
    port_.setConnectionHandlers(nullptr, nullptr, nullptr);
    if (port_.state() != BleCentralPortState::Stopped) port_.stop();
    lease_.releasePermitJoinPort();
    leased_ = false;
    clearRuntime();
    state_ = GatewayPermitJoinAdapterState::Stopped;
    lastError_ = ErrorCode::Ok;
}

void NativeGattPermitJoinAdapter::poll(uint32_t budgetMicros) {
    if (state_ != GatewayPermitJoinAdapterState::Ready) return;
    port_.poll(budgetMicros);
    expireCandidates();
    if (port_.state() == BleCentralPortState::Error) {
        fail(port_.lastError() == ErrorCode::Ok
                 ? ErrorCode::InternalError
                 : port_.lastError());
        return;
    }

    if (activeAttemptId_ != 0U &&
        port_.state() == BleCentralPortState::Idle) {
        activeAttemptId_ = 0U;
        connectedAttemptId_ = 0U;
        maxPacketSize_ = 0U;
    }
    if (activeAttemptId_ == 0U &&
        port_.state() == BleCentralPortState::Idle) {
        const Result result = startScan();
        if (!result) {
            fail(result.code());
            return;
        }
    }
}

size_t NativeGattPermitJoinAdapter::relayCandidateCount() const {
    size_t count = 0U;
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++index) {
        if (candidates_[index].occupied) ++count;
    }
    return count;
}

Result NativeGattPermitJoinAdapter::readRelayCandidate(
    size_t index,
    GatewayPermitJoinCandidate& output) const {
    size_t current = 0U;
    for (size_t slot = 0U;
         slot < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++slot) {
        if (!candidates_[slot].occupied) continue;
        if (current == index) {
            output = candidates_[slot].candidate;
            return Result::success();
        }
        ++current;
    }
    output = GatewayPermitJoinCandidate();
    return Result::failure(ErrorCode::NotFound);
}

Result NativeGattPermitJoinAdapter::selectRelayCandidate(ByteView locator) {
    if (state_ != GatewayPermitJoinAdapterState::Ready) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (locator.data == nullptr ||
        locator.size != sizeof(selectedLocator_)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (selectionSet_) {
        return selected(locator)
                   ? Result::success()
                   : Result::failure(ErrorCode::StateConflict);
    }
    bool found = false;
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++index) {
        if (candidates_[index].occupied &&
            sameLocator(candidates_[index].candidate.token, locator)) {
            found = true;
            break;
        }
    }
    if (!found) return Result::failure(ErrorCode::NotFound);
    memcpy(selectedLocator_, locator.data, sizeof(selectedLocator_));
    selectionSet_ = true;
    return Result::success();
}

void NativeGattPermitJoinAdapter::clearRelaySelection() {
    if (connectedAttemptId_ != 0U ||
        port_.state() == BleCentralPortState::Connecting ||
        port_.state() == BleCentralPortState::Discovering) {
        return;
    }
    secureZero(MutableByteSpan(
        selectedLocator_, sizeof(selectedLocator_)));
    selectionSet_ = false;
}

Result NativeGattPermitJoinAdapter::queueRelayPacket(ByteView packet) {
    if (!relayConnected() || packet.data == nullptr || packet.empty() ||
        packet.size > BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE ||
        packet.size > maxPacketSize_) {
        return Result::failure(
            !relayConnected() ? ErrorCode::NotConnected
                         : ErrorCode::InvalidArgument);
    }
    const Result result = port_.sendPacket(connectedAttemptId_, packet);
    if (!result && result.code() != ErrorCode::WouldBlock &&
        result.code() != ErrorCode::NotConnected) {
        fail(result.code());
    }
    return result;
}

Result NativeGattPermitJoinAdapter::takeRelayPacket(
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    if (rxCount_ == 0U) return Result::failure(ErrorCode::NotFound);
    const PacketSlot& slot = rx_[rxHead_];
    if (output.data == nullptr || output.size < slot.size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memcpy(output.data, slot.bytes, slot.size);
    written = slot.size;
    rx_[rxHead_] = PacketSlot();
    rxHead_ = static_cast<uint8_t>(
        (rxHead_ + 1U) % BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH);
    --rxCount_;
    return Result::success();
}

bool NativeGattPermitJoinAdapter::advertisementThunk(
    void* context,
    const BleCentralAdvertisement& advertisement) {
    NativeGattPermitJoinAdapter* self =
        static_cast<NativeGattPermitJoinAdapter*>(context);
    return self != nullptr && self->onAdvertisement(advertisement);
}

void NativeGattPermitJoinAdapter::packetThunk(
    void* context,
    const BleCentralConnectionInfo& connection,
    ByteView packet) {
    NativeGattPermitJoinAdapter* self =
        static_cast<NativeGattPermitJoinAdapter*>(context);
    if (self != nullptr) self->onPacket(connection, packet);
}

void NativeGattPermitJoinAdapter::connectedThunk(
    void* context,
    const BleCentralConnectionInfo& connection) {
    NativeGattPermitJoinAdapter* self =
        static_cast<NativeGattPermitJoinAdapter*>(context);
    if (self != nullptr) self->onConnected(connection);
}

void NativeGattPermitJoinAdapter::disconnectedThunk(
    void* context,
    const BleCentralConnectionInfo& connection) {
    NativeGattPermitJoinAdapter* self =
        static_cast<NativeGattPermitJoinAdapter*>(context);
    if (self != nullptr) self->onDisconnected(connection);
}

bool NativeGattPermitJoinAdapter::onAdvertisement(
    const BleCentralAdvertisement& advertisement) {
    ble::ModeProfile profile;
    if (!ble::decodeModeServiceData(
            advertisement.modeServiceData, profile)) {
        return false;
    }
    const bool provisioning =
        profile.mode == ble::ApplicationMode::Provisioning &&
        (profile.capabilities & ble::ModeCapabilityEnrollmentV2) != 0U;
    const bool authorizedDirect =
        profile.mode == ble::ApplicationMode::Direct &&
        (profile.capabilities & ble::ModeCapabilityDirectBbp2) != 0U &&
        (profile.capabilities &
         ble::ModeCapabilityAuthorizedPresenceV1) != 0U;
    if (!provisioning && !authorizedDirect) {
        return false;
    }

    const uint32_t now = clock_.monotonicMillis();
    CandidateSlot* target = nullptr;
    CandidateSlot* weakest = nullptr;
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++index) {
        CandidateSlot& slot = candidates_[index];
        if (slot.occupied &&
            memcmp(
                slot.candidate.token,
                profile.modeLocator,
                sizeof(profile.modeLocator)) == 0) {
            target = &slot;
            break;
        }
        if (!slot.occupied && target == nullptr) target = &slot;
        if (slot.occupied &&
            (weakest == nullptr ||
             slot.rssi < weakest->rssi)) {
            weakest = &slot;
        }
    }
    if (target == nullptr && weakest != nullptr &&
        advertisement.rssi > weakest->rssi) {
        target = weakest;
    }
    if (target != nullptr) {
        target->candidate.wireVersion = profile.wireVersion;
        target->candidate.capabilities = profile.capabilities;
        memcpy(
            target->candidate.token,
            profile.modeLocator,
            sizeof(profile.modeLocator));
        target->candidate.tokenSize = sizeof(profile.modeLocator);
        const int32_t quality =
            static_cast<int32_t>(advertisement.rssi) + 100;
        target->candidate.signalQuality = static_cast<uint8_t>(
            quality <= 0 ? 0 : (quality >= 100 ? 100 : quality));
        target->rssi = advertisement.rssi;
        target->lastSeenMillis = now;
        target->occupied = true;
    }
    return selected(ByteView(
        profile.modeLocator,
        sizeof(profile.modeLocator)));
}

void NativeGattPermitJoinAdapter::onPacket(
    const BleCentralConnectionInfo& connection,
    ByteView packet) {
    if (connection.attemptId != connectedAttemptId_ ||
        packet.data == nullptr || packet.empty() ||
        packet.size > BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE ||
        packet.size > maxPacketSize_) {
        fail(ErrorCode::ProtocolError);
        return;
    }
    if (rxCount_ >= BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH) {
        ++droppedPackets_;
        fail(ErrorCode::CapacityExceeded);
        return;
    }
    PacketSlot& slot = rx_[rxTail_];
    memcpy(slot.bytes, packet.data, packet.size);
    slot.size = static_cast<uint8_t>(packet.size);
    rxTail_ = static_cast<uint8_t>(
        (rxTail_ + 1U) % BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH);
    ++rxCount_;
}

void NativeGattPermitJoinAdapter::onConnected(
    const BleCentralConnectionInfo& connection) {
    if (connection.attemptId == 0U ||
        connection.attemptId != activeAttemptId_ ||
        !connection.connected || !connection.notifyEnabled ||
        connection.maxPacketSize < ble::kMinimumPacketSize ||
        connection.maxPacketSize >
            BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE) {
        fail(ErrorCode::ProtocolError);
        return;
    }
    connectedAttemptId_ = connection.attemptId;
    maxPacketSize_ = connection.maxPacketSize;
}

void NativeGattPermitJoinAdapter::onDisconnected(
    const BleCentralConnectionInfo& connection) {
    if (connection.attemptId != activeAttemptId_) return;
    activeAttemptId_ = 0U;
    connectedAttemptId_ = 0U;
    maxPacketSize_ = 0U;
    rxHead_ = rxTail_ = rxCount_ = 0U;
}

Result NativeGattPermitJoinAdapter::startScan() {
    ++nextAttemptId_;
    if (nextAttemptId_ == 0U) ++nextAttemptId_;
    BleCentralConnectRequest request;
    request.attemptId = nextAttemptId_;
    request.matcher = &NativeGattPermitJoinAdapter::advertisementThunk;
    request.matcherContext = this;
    Result result = port_.connect(request);
    if (result) activeAttemptId_ = request.attemptId;
    return result;
}

void NativeGattPermitJoinAdapter::expireCandidates() {
    const uint32_t now = clock_.monotonicMillis();
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++index) {
        CandidateSlot& slot = candidates_[index];
        if (slot.occupied &&
            static_cast<uint32_t>(now - slot.lastSeenMillis) >=
                config_.candidateTtlMillis) {
            slot = CandidateSlot();
        }
    }
}

void NativeGattPermitJoinAdapter::fail(ErrorCode error) {
    lastError_ = error == ErrorCode::Ok
                     ? ErrorCode::InternalError
                     : error;
    state_ = GatewayPermitJoinAdapterState::Fault;
}

void NativeGattPermitJoinAdapter::clearRuntime() {
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES;
         ++index) {
        candidates_[index] = CandidateSlot();
    }
    for (size_t index = 0U;
         index < BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH;
         ++index) {
        rx_[index] = PacketSlot();
    }
    secureZero(MutableByteSpan(
        selectedLocator_, sizeof(selectedLocator_)));
    rxHead_ = rxTail_ = rxCount_ = 0U;
    activeAttemptId_ = 0U;
    connectedAttemptId_ = 0U;
    droppedPackets_ = 0U;
    maxPacketSize_ = 0U;
    selectionSet_ = false;
}

bool NativeGattPermitJoinAdapter::selected(ByteView locator) const {
    return selectionSet_ && sameLocator(selectedLocator_, locator);
}

bool NativeGattPermitJoinAdapter::sameLocator(
    const uint8_t* left,
    ByteView right) {
    return left != nullptr && right.data != nullptr &&
           right.size == kSetupSessionLocatorSize &&
           memcmp(left, right.data, kSetupSessionLocatorSize) == 0;
}

} // namespace blinker

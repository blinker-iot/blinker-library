#include "GatewayCloudMux.h"

namespace blinker {

GatewayCloudMux::GatewayCloudMux(MqttFrameTransport& mqtt)
    : mqtt_(mqtt), slots_() {
    mqtt_.setSideChannelHandler(&GatewayCloudMux::messageThunk, this);
}

GatewayCloudMux::~GatewayCloudMux() {
    mqtt_.setSideChannelHandler(nullptr, nullptr);
}

Result GatewayCloudMux::bind(
    GatewayCloudChannel channel,
    MqttMessageHandler handler,
    void* context) {
    if (handler == nullptr || context == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Slot& slot = slots_[slotIndex(channel)];
    if (slot.handler != nullptr &&
        (slot.handler != handler || slot.context != context)) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    slot.handler = handler;
    slot.context = context;
    return Result::success();
}

void GatewayCloudMux::unbind(
    GatewayCloudChannel channel,
    void* context) {
    Slot& slot = slots_[slotIndex(channel)];
    if (slot.context != context) return;
    slot.handler = nullptr;
    slot.context = nullptr;
}

Result GatewayCloudMux::subscribe(
    StringView topic,
    size_t maximumPayloadSize,
    uint8_t qos) {
    return mqtt_.subscribeSideChannel(topic, maximumPayloadSize, qos);
}

Result GatewayCloudMux::publish(
    StringView topic,
    ByteView payload,
    const MqttPublishOptions& options) {
    return mqtt_.publishSideChannel(topic, payload, options);
}

void GatewayCloudMux::messageThunk(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    GatewayCloudMux* self = static_cast<GatewayCloudMux*>(context);
    if (self != nullptr) self->onMessage(topic, payload, info);
}

void GatewayCloudMux::onMessage(
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    for (size_t index = 0U; index < 2U; ++index) {
        const Slot slot = slots_[index];
        if (slot.handler != nullptr) {
            slot.handler(slot.context, topic, payload, info);
        }
    }
}

size_t GatewayCloudMux::slotIndex(GatewayCloudChannel channel) {
    return channel == GatewayCloudChannel::ChildRoute ? 1U : 0U;
}

} // namespace blinker

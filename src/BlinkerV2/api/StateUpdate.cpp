#include "StateUpdate.h"

#include "Client.h"

namespace blinker {

StateUpdate::StateUpdate(Client& client)
    : client_(client),
      builder_(client.registry_, client.patchBuffer_),
      begun_(false),
      committed_(false),
      acquired_(false) {}

StateUpdate::~StateUpdate() {
    if (acquired_) client_.releasePatchBuffer();
}

Result StateUpdate::ready() const {
    return begun_ && !committed_
               ? Result::success()
               : Result::failure(ErrorCode::NotConfigured);
}

Result StateUpdate::begin(size_t fieldCount) {
    if (begun_ || committed_) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    Result result = client_.acquirePatchBuffer();
    if (!result) return result;
    acquired_ = true;
    result = builder_.begin(fieldCount);
    if (result) {
        begun_ = true;
    } else {
        client_.releasePatchBuffer();
        acquired_ = false;
    }
    return result;
}

Result StateUpdate::set(const EndpointHandle& endpoint, bool value) {
    Result result = ready();
    return result ? builder_.write(endpoint, value) : result;
}

Result StateUpdate::set(const EndpointHandle& endpoint, int64_t value) {
    Result result = ready();
    return result ? builder_.write(endpoint, value) : result;
}

Result StateUpdate::set(const EndpointHandle& endpoint, uint64_t value) {
    Result result = ready();
    return result ? builder_.write(endpoint, value) : result;
}

Result StateUpdate::set(const EndpointHandle& endpoint, float value) {
    Result result = ready();
    return result ? builder_.write(endpoint, value) : result;
}

Result StateUpdate::set(const EndpointHandle& endpoint, double value) {
    Result result = ready();
    return result ? builder_.write(endpoint, value) : result;
}

Result StateUpdate::setText(
    const EndpointHandle& endpoint,
    StringView value) {
    Result result = ready();
    return result ? builder_.writeText(endpoint, value) : result;
}

Result StateUpdate::setBytes(
    const EndpointHandle& endpoint,
    ByteView value) {
    Result result = ready();
    return result ? builder_.writeBytes(endpoint, value) : result;
}

Result StateUpdate::setEncoded(
    const EndpointHandle& endpoint,
    ByteView value) {
    Result result = ready();
    return result ? builder_.writeEncoded(endpoint, value) : result;
}

Result StateUpdate::commit() {
    Result result = ready();
    if (!result) return result;
    ByteView values;
    result = builder_.finish(values);
    if (!result) return result;
    committed_ = true;
    result = client_.commitState(values);
    client_.releasePatchBuffer();
    acquired_ = false;
    return result;
}

} // namespace blinker

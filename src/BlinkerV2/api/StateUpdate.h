#ifndef BLINKER_API_STATE_UPDATE_H
#define BLINKER_API_STATE_UPDATE_H

#include "PatchBuilder.h"

namespace blinker {

class Client;

// Builds and commits one atomic multi-endpoint state transaction using the
// Client-owned patch buffer. It works after Client::prepare(), including while
// offline. Keep at most one active StateUpdate per Client.
class StateUpdate {
public:
    explicit StateUpdate(Client& client);
    ~StateUpdate();

    Result begin(size_t fieldCount);
    Result set(const EndpointHandle& endpoint, bool value);
    Result set(const EndpointHandle& endpoint, int64_t value);
    Result set(const EndpointHandle& endpoint, uint64_t value);
    Result set(const EndpointHandle& endpoint, float value);
    Result set(const EndpointHandle& endpoint, double value);
    Result setText(const EndpointHandle& endpoint, StringView value);
    Result setBytes(const EndpointHandle& endpoint, ByteView value);
    Result setEncoded(const EndpointHandle& endpoint, ByteView value);
    Result commit();

private:
    Result ready() const;

    Client& client_;
    PatchBuilder builder_;
    bool begun_;
    bool committed_;
    bool acquired_;

    StateUpdate(const StateUpdate&);
    StateUpdate& operator=(const StateUpdate&);
};

} // namespace blinker

#endif

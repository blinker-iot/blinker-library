#include <BlinkerV2/transport/detail/WebSocketIngressGuard.h>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
using namespace blinker;
using Bytes = std::vector<uint8_t>;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (false)
Bytes frame(uint8_t opcode, bool final, size_t size) {
    Bytes result = {static_cast<uint8_t>(opcode | (final ? 128U : 0U))};
    if (size < 126U) result.push_back(static_cast<uint8_t>(128U | size));
    else {
        result.push_back(254U); result.push_back(static_cast<uint8_t>(size >> 8U));
        result.push_back(static_cast<uint8_t>(size));
    }
    for (size_t i = 0U; i < 4U; ++i) result.push_back(static_cast<uint8_t>(i + 1U));
    result.resize(result.size() + size, 7U); // Guard does not decode payload.
    return result;
}
bool feed(WebSocketIngressGuard& guard, const Bytes& bytes, size_t chunk = 1U) {
    for (size_t offset = 0U; offset < bytes.size();) {
        const size_t size = bytes.size() - offset < chunk ? bytes.size() - offset : chunk;
        if (!guard.consume(ByteView(bytes.data() + offset, size))) return false;
        offset += size;
    }
    return true;
}
void rejects(const Bytes& bytes) {
    for (size_t chunk = 1U; chunk <= bytes.size(); ++chunk) {
        WebSocketIngressGuard guard(420U);
        CHECK(!feed(guard, bytes, chunk));
        CHECK(!feed(guard, frame(2U, true, 1U))); // Fail is sticky until physical reset.
        guard.reset(); CHECK(feed(guard, frame(2U, true, 1U)));
    }
}
}

int main() {
    for (size_t size : {1U, 2U, 125U, 126U, 272U, 400U, 420U}) {
        const Bytes valid = frame(2U, true, size);
        for (size_t chunk = 1U; chunk <= valid.size(); ++chunk) {
            WebSocketIngressGuard guard(420U);
            CHECK(feed(guard, valid, chunk)); CHECK(guard.boundary()); CHECK(guard.messageBoundary());
            CHECK(feed(guard, valid, chunk)); // Multiple complete messages.
        }
    }
    WebSocketIngressGuard fragmented(420U);
    CHECK(feed(fragmented, frame(2U, false, 100U)));
    CHECK(fragmented.boundary() && !fragmented.messageBoundary());
    CHECK(feed(fragmented, frame(9U, true, 125U))); // Ping does not end/extend a fragmented message.
    CHECK(!fragmented.messageBoundary());
    CHECK(feed(fragmented, frame(0U, false, 320U)));
    CHECK(feed(fragmented, frame(10U, true, 0U)));
    CHECK(feed(fragmented, frame(0U, true, 0U)));
    CHECK(fragmented.messageBoundary());
    CHECK(feed(fragmented, frame(8U, true, 2U)));

    rejects(frame(1U, true, 1U)); rejects(frame(2U, true, 0U));
    rejects(frame(0U, true, 1U)); rejects(frame(2U, true, 421U));
    rejects(frame(9U, true, 126U)); rejects(frame(9U, false, 1U));
    rejects(frame(8U, true, 1U)); rejects(frame(11U, true, 1U));
    for (unsigned opcode : {3U, 4U, 5U, 6U, 7U}) rejects(frame(static_cast<uint8_t>(opcode), true, 2U));
    Bytes bad = frame(2U, true, 1U); bad[0] |= 64U; rejects(bad); // RSV1 without compression negotiation.
    bad = frame(2U, true, 1U); bad[1] &= 127U; rejects(bad); // No client mask.
    rejects(Bytes{130U, 255U}); // No 64-bit narrowing, reject before remaining header/payload.
    rejects(Bytes{130U, 254U, 0U, 1U, 1U, 2U, 3U, 4U, 7U}); // Noncanonical 16-bit size.
    for (bool interleaved : {false, true}) {
        WebSocketIngressGuard guard(420U);
        CHECK(feed(guard, frame(2U, false, 300U)));
        CHECK(!feed(guard, frame(interleaved ? 2U : 0U, true, 121U)));
    }
    // SDK can split the header and payload at every byte without losing bounds.
    WebSocketIngressGuard incomplete(420U);
    const Bytes valid = frame(2U, true, 420U);
    CHECK(incomplete.consume(ByteView(valid.data(), valid.size() - 1U)));
    CHECK(!incomplete.boundary());
    CHECK(incomplete.consume(ByteView(valid.data() + valid.size() - 1U, 1U)));
    CHECK(incomplete.messageBoundary());
    std::cout << "Bounded WebSocket header admission passed\n";
    return 0;
}

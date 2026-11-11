// Real generated WiFi/System shims and real pending/client/codec CPP. Only
// clock, allocator, OS/thread and wire boundaries are Host seams. The extracted
// C bridges below retain their actual patched bodies; fake socket/IP types do
// not claim native SDK ABI or whole-bridge compilation proof.
#include "Arduino.h"
#include "erpc_arbitrated_client_manager.h"
#include "erpc_basic_codec.h"
#include "erpc_transport_arbitrator.h"
#include "blinker_wio_rpc_boundary.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
extern "C" {
#include "rpc_wifi_api.h"
#include "rpc_system.h"
}

using namespace erpc;
namespace {
unsigned stops = 0, allocations = 0, frees = 0;
void check(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
}
namespace blinker { namespace wio_rpc { Boundary boundary; } }
ClientManager* g_client = nullptr;
extern "C" uint32_t blinker_wio_rpc_enter_critical() { const auto old = host_rpc::primask; host_rpc::primask = 1; return old; }
extern "C" void blinker_wio_rpc_leave_critical(uint32_t saved) { host_rpc::primask = saved; }
extern "C" uint32_t blinker_wio_rpc_now() { return host_rpc::clockMs; }
extern "C" void blinker_wio_rpc_pause() { delay(1); }
extern "C" void blinker_wio_rpc_stop_uart() { check(host_rpc::primask == 1U, "unguarded fault stop"); ++stops; }
extern "C" bool blinker_wio_rpc_caller_allowed() {
    return !host_rpc::primask && !host_rpc::basepri && !host_rpc::ipsr &&
        host_rpc::schedulerState == taskSCHEDULER_RUNNING && host_rpc::currentId == 2U;
}
extern "C" int blinker_wio_rpc_faulted() { return blinker::wio_rpc::boundary.faulted(); }
extern "C" int blinker_wio_rpc_finalize(uint32_t sequence, int status) { return blinker::wio_rpc::boundary.finalize(sequence, status); }
extern "C" void blinker_wio_rpc_invalid_output() { blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec); }
extern "C" void* erpc_malloc(size_t bytes) { ++allocations; return std::malloc(bytes); }
extern "C" void erpc_free(void* memory) { if (memory) ++frees; std::free(memory); }

namespace actual_bridge {
// OS/type seams for compiling the exact extracted C function bodies only.
using socklen_t = uint32_t;
struct sockaddr { uint8_t bytes[16]; };
struct ip_addr_t { uint8_t bytes[4]; };
using err_t = int8_t;
static const err_t ERR_OK = 0, ERR_ARG = -16, ERR_VAL = -6;
#ifndef ENOTCONN
#define ENOTCONN 107
#endif
#define FUNC_ENTRY do {} while (0)
#define FUNC_EXIT_RC(value) return (value)
#include "actual_bridge_excerpt.inc"
#undef FUNC_EXIT_RC
#undef FUNC_ENTRY
}

namespace {
class TimedCodec : public BasicCodec {
public:
    TimedCodec(uint32_t& encode, uint32_t& decode) : encode_(encode), decode_(decode) {}
    void startWriteMessage(message_type_t kind, uint32_t service, uint32_t method, uint32_t sequence) override {
        host_rpc::clockMs += encode_;
        BasicCodec::startWriteMessage(kind, service, method, sequence);
    }
    using BasicCodec::read;
    void read(int32_t* value) override {
        BasicCodec::read(value);
        host_rpc::clockMs += decode_;
    }
private: uint32_t& encode_; uint32_t& decode_;
};
class Codecs : public CodecFactory {
public:
    Codec* create() override { ++calls; ++live; return new TimedCodec(encodeCost, decodeCost); }
    void dispose(Codec* value) override { check(value && live, "invalid codec cleanup"); --live; delete value; }
    unsigned calls = 0, live = 0;
    uint32_t encodeCost = 0, decodeCost = 0;
};
class Buffers : public MessageBufferFactory {
public:
    MessageBuffer create() override {
        ++calls;
        if (failNext) { failNext = false; return MessageBuffer(nullptr, 0); }
        ++live; return MessageBuffer(new uint8_t[4096], 4096);
    }
    void dispose(MessageBuffer* value) override { check(value && value->get() && live, "invalid buffer cleanup"); --live; delete[] value->get(); }
    unsigned calls = 0, live = 0;
    bool failNext = false;
};
class InspectManager : public ArbitratedClientManager {
public:
    uint32_t sequence() const { return m_sequence; }
    void setSequence(uint32_t value) { m_sequence = value; }
};
class InspectArbitrator : public TransportArbitrator {
public:
    unsigned active() const {
        unsigned count = 0;
        for (const PendingClientInfo* node = m_clientList; node; node = node->m_next) check(++count < 10, "pending cycle");
        return count;
    }
};
class Wire : public Transport {
public:
    erpc_status_t send(MessageBuffer* outgoing) override {
        if (!blinker::wio_rpc::boundary.markSend()) return kErpcStatus_SendFailed;
        ++sends; host_rpc::clockMs += sendCost;
        if (beforeReply) beforeReply();
        if (failSend || !blinker::wio_rpc::boundary.ready()) return kErpcStatus_SendFailed;
        BasicCodec source; source.setBuffer(*outgoing);
        message_type_t kind; uint32_t service, method, sequence;
        source.startReadMessage(&kind, &service, &method, &sequence);
        check(source.getStatus() == 0, "request header decode");
        append(kReplyMessage, service, method, sequence, payload);
        // Real arbitrator receive keeps routing replies until an invocation
        // returns control to its server. The inert frame is not dispatched.
        append(kInvocationMessage, 1, 1, 0, [](BasicCodec&) {});
        replyAt = host_rpc::clockMs + waitCost;
        if (!waitCost && drain) drain();
        return kErpcStatus_Success;
    }
    erpc_status_t receive(MessageBuffer* buffer) override {
        ++receives;
        if (incoming.empty()) return kErpcStatus_ReceiveFailed;
        const auto bytes = incoming.front(); incoming.pop_front();
        check(bytes.size() <= buffer->getLength(), "reply overflow");
        std::memcpy(buffer->get(), bytes.data(), bytes.size()); buffer->setUsed(static_cast<uint16_t>(bytes.size()));
        return kErpcStatus_Success;
    }
    void append(message_type_t kind, uint32_t service, uint32_t method, uint32_t sequence, const std::function<void(BasicCodec&)>& body) {
        uint8_t bytes[512] = {}; BasicCodec codec; codec.setBuffer(MessageBuffer(bytes, sizeof(bytes)));
        codec.startWriteMessage(kind, service, method, sequence); if (body) body(codec);
        check(codec.getStatus() == 0, "reply encode");
        incoming.emplace_back(codec.getBuffer()->get(), codec.getBuffer()->get() + codec.getBuffer()->getUsed());
    }
    unsigned sends = 0, receives = 0;
    bool failSend = false;
    uint32_t sendCost = 0, waitCost = 0, replyAt = 0;
    std::function<void()> drain, beforeReply;
    std::function<void(BasicCodec&)> payload;
    std::deque<std::vector<uint8_t>> incoming;
};
class Harness {
public:
    Harness() {
        host_rpc::currentId = 2; host_rpc::schedulerState = taskSCHEDULER_RUNNING;
        manager.setCodecFactory(&codecs); manager.setMessageBufferFactory(&buffers);
        manager.setArbitrator(&arbitrator); manager.setServerThreadId(reinterpret_cast<void*>(1));
        arbitrator.setSharedTransport(&wire); arbitrator.setCodec(&routing);
        g_client = &manager;
        wire.payload = [](BasicCodec& codec) { codec.write(int32_t(1)); };
        wire.drain = [this] { drain(); };
        host_rpc::onPause = [this] {
            if (!wire.incoming.empty() && host_rpc::clockMs >= wire.replyAt) drain();
        };
    }
    ~Harness() { host_rpc::onPause = nullptr; g_client = nullptr; }
    void drain() {
        MessageBuffer buffer = buffers.create();
        lastDrain = arbitrator.receive(&buffer);
        buffers.dispose(&buffer);
        check(lastDrain == 0 || (permitFaultDrain && blinker_wio_rpc_faulted()), "routing receive failed");
    }
    void empty() { check(!buffers.live && !codecs.live && !arbitrator.active(), "request cleanup leak"); }
    Codecs codecs; Buffers buffers; BasicCodec routing; InspectArbitrator arbitrator; Wire wire; InspectManager manager;
    bool permitFaultDrain = false;
    erpc_status_t lastDrain = kErpcStatus_Success;
};
using blinker::wio_rpc::Fault;
using blinker::wio_rpc::Outcome;
void normal() {
    Harness h;
    check(rpc_wifi_is_up(0) == 1 && !blinker_wio_rpc_faulted(), "real generated scalar failed");
    h.empty(); check(h.manager.sequence() == 1 && h.wire.sends == 1, "normal owner sequence");
}
void wholeBudget() {
    Harness h; h.codecs.encodeCost = 200; h.wire.sendCost = 300; h.wire.waitCost = 300; h.codecs.decodeCost = 300;
    check(rpc_wifi_is_up(0) == -1, "late final err did not change generated return");
    check(host_rpc::clockMs == 1100 && blinker::wio_rpc::boundary.reason() == Fault::Deadline, "one total encode/send/wait/decode budget");
    check(blinker::wio_rpc::boundary.outcome() == Outcome::Uncertain, "sent timeout became not-sent"); h.empty();
}
void expiredEncode() {
    Harness h; h.codecs.encodeCost = 1000;
    check(rpc_wifi_is_up(0) == -1 && h.wire.sends == 0, "expired encode still sent");
    check(blinker::wio_rpc::boundary.reason() == Fault::Deadline && blinker::wio_rpc::boundary.outcome() == Outcome::NotSent, "pre-send deadline classification"); h.empty();
}
void exactDeadlineLateReply() {
    Harness h; h.wire.waitCost = 1000; h.permitFaultDrain = true;
    check(rpc_wifi_is_up(0) == -1, "reply at deadline became success");
    check(blinker::wio_rpc::boundary.reason() == Fault::Deadline &&
        blinker::wio_rpc::boundary.outcome() == Outcome::Uncertain, "deadline-boundary classification");
    h.empty(); check(!h.wire.incoming.empty(), "old reply was consumed at expired deadline");
    const auto reads = h.wire.receives;
    h.drain(); // Original request codec is already disposed. No live replay.
    check(h.lastDrain != 0 && h.wire.receives == reads && !h.arbitrator.active(), "quarantine borrowed released request or accepted old reply");
    h.empty();
}
void sequenceWrap() {
    Harness h; h.manager.setSequence(UINT32_MAX);
    check(rpc_wifi_is_up(0) == -1 && h.manager.sequence() == UINT32_MAX, "sequence wrapped or committed denied value");
    check(blinker::wio_rpc::boundary.reason() == Fault::Sequence && !h.codecs.calls && !h.buffers.calls && !h.wire.sends, "sequence-wrap allocated or issued I/O");
    h.empty();
}
void allocationFailed() {
    Harness h; h.buffers.failNext = true;
    check(rpc_wifi_is_up(0) == -1 && !h.wire.sends, "allocation failure escape");
    check(blinker::wio_rpc::boundary.reason() == Fault::Codec, "allocation failure did not quarantine"); h.empty();
}
void postFault() {
    Harness h; h.wire.failSend = true;
    check(rpc_wifi_is_up(0) == -1, "initial fault missing"); h.empty();
    const auto codecCalls = h.codecs.calls, bufferCalls = h.buffers.calls, io = h.wire.sends, allocated = allocations;
    check(rpc_lwip_close(5) == -1 && rpc_lwip_errno() == -1 && rpc_system_version() == nullptr, "postfault native return");
    check(h.codecs.calls == codecCalls && h.buffers.calls == bufferCalls && allocations == allocated && h.wire.sends == io, "postfault create/cleanup allocated or issued I/O"); h.empty();
}
void binaryFailure() {
    Harness h; h.wire.failSend = true; binary_t output; output.data = reinterpret_cast<uint8_t*>(1); output.dataLength = 999;
    check(rpc_lwip_read(1, &output, 4, 40) == -1 && !output.data && !output.dataLength, "failed decode binary not NULL/0");
    check(allocations == 0 && frees == 0, "failed binary decode allocated/freed a fabricated output"); h.empty();
}
void stringFailure() {
    Harness h; h.wire.failSend = true;
    check(rpc_system_version() == nullptr && allocations == 0, "failed string decode allocated false empty result"); h.empty();
}
void forbidden(const std::string& kind) {
    Harness h;
    if (kind == "caller-server") host_rpc::currentId = 1;
    else if (kind == "caller-irq") host_rpc::ipsr = 7;
    else host_rpc::primask = 1;
    check(rpc_wifi_is_up(0) == -1, "forbidden caller accepted");
    check(h.manager.sequence() == 0 && h.codecs.calls == 0 && h.buffers.calls == 0 && h.wire.sends == 0, "forbidden caller touched sequence/allocation/wire");
    check(blinker::wio_rpc::boundary.reason() == Fault::Unsupported, "forbidden caller reason");
    host_rpc::primask = 0; h.empty();
}
void reentry() {
    Harness h; h.wire.beforeReply = [&] { check(rpc_lwip_errno() == -1, "reentry accepted"); };
    check(rpc_wifi_is_up(0) == -1 && h.codecs.calls == 1 && h.buffers.calls == 1, "reentry allocated another request");
    check(blinker::wio_rpc::boundary.reason() == Fault::Ownership && h.wire.sends == 1, "reentry ownership classification"); h.empty();
}
void peerShort() {
    Harness h; h.wire.payload = [](BasicCodec& codec) { const uint8_t bytes[4] = {}; codec.writeBinary(4, bytes); codec.write(uint32_t(4)); codec.write(int32_t(0)); };
    actual_bridge::sockaddr address; std::memset(&address, 0xcc, sizeof(address)); actual_bridge::socklen_t length = sizeof(address);
    check(actual_bridge::lwip_getpeername(1, &address, &length) == -1 && !length, "short peer accepted");
    for (auto byte : address.bytes) check(byte == 0, "peer error exposed stale address");
    check(blinker_wio_rpc_faulted() && allocations == frees, "peer error did not quarantine/free"); h.empty();
}
void sockoptShort() {
    Harness h; h.wire.payload = [](BasicCodec& codec) { const uint8_t byte = 0; codec.writeBinary(1, &byte); codec.write(uint32_t(4)); codec.write(int32_t(0)); };
    uint32_t output = 0xccccccccU; actual_bridge::socklen_t length = 4;
    check(actual_bridge::lwip_getsockopt(1, 0, 0, &output, &length) == -1 && !length && !output, "short sockopt became success");
    check(blinker_wio_rpc_faulted() && allocations == frees, "sockopt bad output cleanup"); h.empty();
}
void readOversize() {
    Harness h; h.wire.payload = [](BasicCodec& codec) { const uint8_t bytes[9] = {}; codec.writeBinary(9, bytes); codec.write(int32_t(9)); };
    uint8_t output[5]; std::memset(output, 0xcc, sizeof(output));
    check(actual_bridge::lwip_read(1, output, 4) == -1, "oversized read accepted");
    for (auto byte : output) check(byte == 0xcc, "oversized read touched caller/fence");
    check(blinker_wio_rpc_faulted() && allocations == frees, "read bad output cleanup"); h.empty();
}
void dnsShort() {
    Harness h; h.wire.payload = [](BasicCodec& codec) { const uint8_t bytes[2] = {}; codec.writeBinary(2, bytes); codec.write(int8_t(0)); };
    actual_bridge::ip_addr_t output; std::memset(&output, 0xcc, sizeof(output));
    check(actual_bridge::netconn_gethostbyname("fixture", &output) != 0, "short DNS accepted");
    for (auto byte : output.bytes) check(byte == 0, "DNS error exposed stale address");
    check(blinker_wio_rpc_faulted() && allocations == frees, "DNS bad output cleanup"); h.empty();
}
void errnoLocal() {
    Harness h; blinker::wio_rpc::boundary.trip(Fault::Transport);
    check(actual_bridge::lwip_errno() == ENOTCONN && !h.wire.sends && !h.codecs.calls, "fault errno issued RPC"); h.empty();
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "one finite client scenario required"); const std::string name = argv[1];
        if (name == "normal-generated") normal();
        else if (name == "whole-budget-final-err") wholeBudget();
        else if (name == "encode-expired") expiredEncode();
        else if (name == "exact-deadline-late-reply") exactDeadlineLateReply();
        else if (name == "sequence-wrap") sequenceWrap();
        else if (name == "allocation-failed") allocationFailed();
        else if (name == "postfault-no-allocate") postFault();
        else if (name == "binary-failure") binaryFailure();
        else if (name == "string-failure") stringFailure();
        else if (name.rfind("caller-", 0) == 0) forbidden(name);
        else if (name == "reentry") reentry();
        else if (name == "bridge-getpeer-short") peerShort();
        else if (name == "bridge-sockopt-short") sockoptShort();
        else if (name == "bridge-read-oversize") readOversize();
        else if (name == "bridge-dns-short") dnsShort();
        else if (name == "bridge-errno") errnoLocal();
        else throw std::runtime_error("unknown client scenario");
        std::cout << name << ": actual generated/client/codec flow passed; Host observation only\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

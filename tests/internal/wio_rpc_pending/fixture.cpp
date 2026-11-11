#include "erpc_arbitrated_client_manager.h"
#include "erpc_basic_codec.h"
#include "erpc_transport_arbitrator.h"
#include <atomic>
#include <cstring>
#include <deque>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace host_rpc {
thread_local unsigned heldLocks = 0U;
std::function<void()> beforeUnlock;
std::function<void()> afterSignal;
std::function<void()> beforeBlockedLock;
bool failNextWait = false;
uint32_t lastWait = 0U;
}
using namespace erpc;
namespace {
void check(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
class InspectArbitrator : public TransportArbitrator {
public:
    unsigned active() const {
        unsigned result = 0;
        for (PendingClientInfo* p = m_clientList; p; p = p->m_next)
            check(++result < 20U, "active-list cycle");
        return result;
    }
    bool publishedBound() const {
        return m_clientList && m_clientList->m_request && m_clientList->m_isValid;
    }
    int signals(client_token_t token) const {
        return reinterpret_cast<PendingClientInfo*>(token)->m_sem.getCount();
    }
    void injectSignals(client_token_t token, unsigned count) {
        while (count--) reinterpret_cast<PendingClientInfo*>(token)->m_sem.put();
    }
};
class Buffers : public MessageBufferFactory {
public:
    Buffers() : live(0U) {}
    MessageBuffer create() override { ++live; return MessageBuffer(new uint8_t[4096], 4096); }
    void dispose(MessageBuffer* buffer) override {
        check(buffer->get() != nullptr && live, "invalid real SDK buffer release");
        delete[] buffer->get(); --live;
    }
    unsigned live;
};
class Wire : public Transport {
public:
    Wire() : failSend(false), sends(0U), receiveInsideLock(false) {}
    erpc_status_t send(MessageBuffer*) override {
        ++sends; if (onSend) onSend();
        return failSend ? kErpcStatus_SendFailed : kErpcStatus_Success;
    }
    erpc_status_t receive(MessageBuffer* message) override {
        if (host_rpc::heldLocks) receiveInsideLock = true;
        if (beforeReceive) beforeReceive();
        if (incoming.empty()) return kErpcStatus_ReceiveFailed;
        const std::vector<uint8_t> frame = incoming.front(); incoming.pop_front();
        check(frame.size() <= message->getLength(), "host frame overflow");
        std::memcpy(message->get(), frame.data(), frame.size());
        message->setUsed(static_cast<uint16_t>(frame.size()));
        return kErpcStatus_Success;
    }
    void reply(uint32_t sequence, uint32_t value) {
        uint8_t bytes[32] = {};
        BasicCodec codec; codec.setBuffer(MessageBuffer(bytes, sizeof(bytes)));
        codec.startWriteMessage(kReplyMessage, 3U, 1U, sequence); codec.write(value);
        MessageBuffer* message = codec.getBuffer();
        incoming.emplace_back(message->get(), message->get() + message->getUsed());
    }
    bool failSend;
    unsigned sends;
    bool receiveInsideLock;
    std::function<void()> onSend, beforeReceive;
    std::deque<std::vector<uint8_t>> incoming;
};
class Harness {
public:
    Harness() {
        arbitrator.setSharedTransport(&wire); arbitrator.setCodec(&routingCodec);
        manager.setArbitrator(&arbitrator); manager.setCodecFactory(&codecs);
        manager.setMessageBufferFactory(&buffers);
#if ERPC_NESTED_CALLS
        manager.setServerThreadId(reinterpret_cast<void*>(1));
#endif
    }
    RequestContext request() {
        RequestContext result = manager.createRequest(false);
        result.getCodec()->startWriteMessage(kInvocationMessage, 3U, 1U, result.getSequence());
        result.getCodec()->write(uint32_t(7U)); return result;
    }
    void drain() {
        MessageBuffer buffer = buffers.create();
        do { (void)arbitrator.receive(&buffer); } while (!wire.incoming.empty());
        buffers.dispose(&buffer);
    }
    uint32_t payload(RequestContext& request, bool headerConsumed) {
        Codec* codec = request.getCodec();
        if (!headerConsumed) {
            codec->reset(); message_type_t kind; uint32_t service, method, sequence;
            codec->startReadMessage(&kind, &service, &method, &sequence);
            check(kind == kReplyMessage && sequence == request.getSequence(), "wrong response header");
        }
        uint32_t value = 0U; codec->read(&value); check(codec->getStatus() == 0, "response decode failed"); return value;
    }
    void finish(RequestContext& request) { manager.releaseRequest(request); }
    void empty() { check(arbitrator.active() == 0U && buffers.live == 0U, "pending or buffer leak"); }
    Buffers buffers;
    BasicCodecFactory codecs;
    BasicCodec routingCodec;
    InspectArbitrator arbitrator;
    Wire wire;
    ArbitratedClientManager manager;
};

void atomicPublish() {
    Harness h; RequestContext r = h.request(); bool inspected = false, bound = false;
    host_rpc::beforeUnlock = [&] { inspected = true; bound = h.arbitrator.publishedBound(); };
    const auto token = h.arbitrator.prepareClientReceive(r);
    host_rpc::beforeUnlock = nullptr;
    check(inspected && bound, "node published before binding request/validity");
    h.arbitrator.cancelClientReceive(token); h.finish(r); h.empty();
}
void earlyReply() {
    Harness h; RequestContext r = h.request();
    h.wire.onSend = [&] { h.wire.reply(r.getSequence(), 41U); h.drain(); };
    h.manager.performRequest(r);
    check(r.getCodec()->getStatus() == 0 && h.payload(r, true) == 41U, "early response lost");
    check(!h.wire.receiveInsideLock, "pending mutex held across native receive");
    h.finish(r); h.empty();
}
void duplicateReply() {
    Harness h; RequestContext r = h.request(); const auto token = h.arbitrator.prepareClientReceive(r);
    h.wire.reply(r.getSequence(), 41U); h.wire.reply(r.getSequence(), 99U); h.drain();
    check(h.arbitrator.signals(token) == 1, "duplicate added wakeup signal");
    check(h.arbitrator.clientReceive(token) == 0 && h.payload(r, false) == 41U, "duplicate swapped first response");
    h.finish(r); h.empty();
}
void failedSendLate() {
    Harness h; RequestContext first = h.request(); const uint32_t sequence = first.getSequence();
    h.wire.failSend = true; h.manager.performRequest(first);
    check(first.getCodec()->getStatus() == kErpcStatus_SendFailed, "send failure not propagated");
    check(h.arbitrator.active() == 0U, "send failure retained stack request pointer");
    h.finish(first); // Real SDK frees codec/buffer before the late reply arrives.
    h.wire.reply(sequence, 99U); h.drain(); h.empty();
}
void staleSignal() {
    Harness h; RequestContext first = h.request(); const auto old = h.arbitrator.prepareClientReceive(first);
    h.wire.reply(first.getSequence(), 41U); h.drain(); h.arbitrator.cancelClientReceive(old); h.finish(first);
    RequestContext next = h.request(); const auto token = h.arbitrator.prepareClientReceive(next);
    check(token == old && h.arbitrator.signals(token) == 0, "cached wakeup crossed request lifetime");
    h.wire.reply(first.getSequence(), 99U); h.drain();
    check(h.arbitrator.signals(token) == 0, "late old sequence signaled reused token");
    host_rpc::failNextWait = true;
    check(h.arbitrator.clientReceive(token) == kErpcStatus_Timeout, "stale token returned false success");
    check(host_rpc::lastWait == Semaphore::kWaitForever, "native wait budget changed");
    h.finish(next); h.empty();
}
void freeListNext() {
    Harness h; RequestContext a = h.request(), b = h.request(), c = h.request();
    const auto ta = h.arbitrator.prepareClientReceive(a), tb = h.arbitrator.prepareClientReceive(b), tc = h.arbitrator.prepareClientReceive(c);
    h.arbitrator.cancelClientReceive(tb); h.arbitrator.cancelClientReceive(tc); h.arbitrator.cancelClientReceive(ta);
    h.finish(a); h.finish(b); h.finish(c);
    RequestContext next = h.request(); const auto token = h.arbitrator.prepareClientReceive(next);
    check(h.arbitrator.active() == 1U, "free-list next leaked cached nodes into active list");
    h.arbitrator.cancelClientReceive(token); h.finish(next); h.empty();
}
void waitFalse() {
    Harness h; RequestContext r = h.request(); const auto token = h.arbitrator.prepareClientReceive(r);
    host_rpc::failNextWait = true;
    check(h.arbitrator.clientReceive(token) == kErpcStatus_Timeout, "failed native wait became success");
    check(h.arbitrator.active() == 0U, "failed wait retained request pointer");
    check(host_rpc::lastWait == Semaphore::kWaitForever, "deadline smuggled into pending-only patch");
    h.finish(r); h.empty();
}
void receiveBeforeCancel() {
    Harness h; RequestContext r = h.request(); const auto token = h.arbitrator.prepareClientReceive(r);
    const uint32_t sequence = r.getSequence(); h.wire.reply(sequence, 41U);
    std::promise<void> entered, resume; auto gate = resume.get_future().share(); bool announced = false;
    h.wire.beforeReceive = [&] { if (!announced) { announced = true; entered.set_value(); gate.wait(); } };
    auto reader = std::async(std::launch::async, [&] { h.drain(); });
    check(entered.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready, "receive not reached");
    h.arbitrator.cancelClientReceive(token); h.finish(r);
    resume.set_value(); reader.get();
    check(!h.wire.receiveInsideLock, "UART receive borrowed pending mutex"); h.empty();
}
void cancelDuringDelivery() {
    Harness h; RequestContext r = h.request(); const auto token = h.arbitrator.prepareClientReceive(r);
    h.wire.reply(r.getSequence(), 41U);
    std::promise<void> signaled, resume, contended; auto gate = resume.get_future().share();
    host_rpc::afterSignal = [&] {
        check(host_rpc::heldLocks > 0U, "delivery did not retain pending mutex");
        signaled.set_value(); gate.wait();
    };
    auto reader = std::async(std::launch::async, [&] { h.drain(); });
    check(signaled.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready, "delivery not reached");
    host_rpc::beforeBlockedLock = [&] { contended.set_value(); };
    auto cancel = std::async(std::launch::async, [&] { h.arbitrator.cancelClientReceive(token); });
    const bool blocked = contended.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    const bool unfinished = cancel.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout;
    resume.set_value(); reader.get(); cancel.get();
    host_rpc::afterSignal = nullptr; host_rpc::beforeBlockedLock = nullptr;
    check(blocked && unfinished, "cancel did not contend for receive's pending mutex");
    h.finish(r); h.empty();
}
void excessiveSignals() {
    Harness h; RequestContext r = h.request(); const auto token = h.arbitrator.prepareClientReceive(r);
    h.arbitrator.cancelClientReceive(token); h.finish(r); h.arbitrator.injectSignals(token, 2U);
    RequestContext next = h.request();
    check(h.arbitrator.prepareClientReceive(next) == 0, "unexpected count was unbounded-drained/reused");
    check(h.arbitrator.active() == 0U, "invalid node published"); h.finish(next); h.empty();
}
void loggingFailure() {
#if ERPC_MESSAGE_LOGGING
    Harness h; Wire logger; logger.failSend = true;
    check(h.manager.addMessageLogger(&logger), "real SDK logger install failed");
    RequestContext r = h.request(); const uint32_t sequence = r.getSequence();
    h.manager.performRequest(r);
    check(h.wire.sends == 0U && r.getCodec()->getStatus() == kErpcStatus_SendFailed, "pre-send logging failure escaped");
    check(h.arbitrator.active() == 0U, "logging failure retained request pointer");
    h.finish(r); h.wire.reply(sequence, 99U); h.drain(); h.empty();
#else
    throw std::runtime_error("logging fixture requires real enabled SDK branch");
#endif
}
} // namespace
int main(int argc, char** argv) {
    try {
        check(argc == 2, "one finite scenario required"); const std::string name = argv[1];
        if (name == "atomic-publish") atomicPublish();
        else if (name == "early-reply") earlyReply();
        else if (name == "duplicate-reply") duplicateReply();
        else if (name == "send-failure-late") failedSendLate();
        else if (name == "stale-signal") staleSignal();
        else if (name == "free-list-next") freeListNext();
        else if (name == "wait-false") waitFalse();
        else if (name == "receive-before-cancel") receiveBeforeCancel();
        else if (name == "cancel-during-delivery") cancelDuringDelivery();
        else if (name == "excessive-signals") excessiveSignals();
        else if (name == "logging-failure") loggingFailure();
        else throw std::runtime_error("unknown finite scenario");
        std::cout << name << ": real SDK/overlay passed; no deadline claim\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

#ifndef BLINKER_WIO_RPC_DEADLINE_HOST_THREADING_H
#define BLINKER_WIO_RPC_DEADLINE_HOST_THREADING_H
#include "Arduino.h"
#include <mutex>
namespace erpc {
class Mutex {
public:
    class Guard {
    public:
        explicit Guard(Mutex& mutex) : mutex_(mutex) { mutex_.lock(); }
        ~Guard() { mutex_.unlock(); }
    private: Mutex& mutex_;
    };
    bool lock() { mutex_.lock(); ++host_rpc::heldLocks; return true; }
    bool tryLock() {
        ++host_rpc::tryLockCalls;
        if (host_rpc::failAllTryLocks || !mutex_.try_lock()) return false;
        ++host_rpc::heldLocks; return true;
    }
    bool unlock() { --host_rpc::heldLocks; mutex_.unlock(); return true; }
private: std::recursive_mutex mutex_;
};
class Semaphore {
public:
    static const uint32_t kWaitForever = 0xffffffffU;
    explicit Semaphore(int count = 0) : count_(count) {}
    void put() { ++count_; if (host_rpc::afterSignal) host_rpc::afterSignal(); }
    void putFromISR() { put(); }
    bool get(uint32_t timeout = kWaitForever) {
        if (host_rpc::onSemaphoreWait) host_rpc::onSemaphoreWait(timeout);
        if (!count_) return false;
        --count_; return true;
    }
    int getCount() const { return count_; }
private: int count_;
};
class Thread {
public:
    typedef void* thread_id_t;
    typedef void (*thread_entry_t)(void*);
    explicit Thread(uintptr_t id = 2) : id_(id) {}
    static thread_id_t getCurrentThreadId() { return reinterpret_cast<void*>(host_rpc::currentId); }
    thread_id_t getThreadId() const { return reinterpret_cast<void*>(id_); }
private: uintptr_t id_;
};
}
#endif

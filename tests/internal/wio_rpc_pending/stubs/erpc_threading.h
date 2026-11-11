#ifndef BLINKER_WIO_RPC_HOST_THREADING_H
#define BLINKER_WIO_RPC_HOST_THREADING_H
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>

namespace host_rpc {
extern thread_local unsigned heldLocks;
extern std::function<void()> beforeUnlock;
extern std::function<void()> afterSignal;
extern std::function<void()> beforeBlockedLock;
extern bool failNextWait;
extern uint32_t lastWait;
}
namespace erpc {
class Mutex {
public:
    class Guard {
    public:
        explicit Guard(Mutex& mutex) : mutex_(mutex) { mutex_.lock(); }
        ~Guard() { mutex_.unlock(); }
    private:
        Mutex& mutex_;
    };
    bool lock() {
        if (!mutex_.try_lock()) {
            if (host_rpc::beforeBlockedLock) host_rpc::beforeBlockedLock();
            mutex_.lock();
        }
        ++host_rpc::heldLocks; return true;
    }
    bool unlock() {
        if (host_rpc::beforeUnlock) host_rpc::beforeUnlock();
        --host_rpc::heldLocks; mutex_.unlock(); return true;
    }
    bool tryLock() {
        if (!mutex_.try_lock()) return false;
        ++host_rpc::heldLocks; return true;
    }
private:
    std::recursive_mutex mutex_;
};
class Semaphore {
public:
    static const uint32_t kWaitForever = 0xffffffffU;
    explicit Semaphore(int count) : count_(count) {}
    void put() {
        { std::lock_guard<std::mutex> lock(mutex_); ++count_; }
        ready_.notify_all();
        if (host_rpc::afterSignal) host_rpc::afterSignal();
    }
    int getCount() const { std::lock_guard<std::mutex> lock(mutex_); return count_; }
    bool get(uint32_t timeout = kWaitForever) {
        host_rpc::lastWait = timeout;
        std::unique_lock<std::mutex> lock(mutex_);
        if (timeout != 0U && host_rpc::failNextWait) {
            host_rpc::failNextWait = false; return false;
        }
        if (!count_ && timeout != 0U) {
            // Test-only deadlock bound, not an SDK deadline implementation.
            if (!ready_.wait_for(lock, std::chrono::seconds(5), [this] { return count_ != 0; })) return false;
        }
        if (!count_) return false;
        --count_; return true;
    }
private:
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    int count_;
};
class Thread {
public:
    typedef void* thread_id_t;
    static thread_id_t getCurrentThreadId() { return reinterpret_cast<void*>(2); }
};
} // namespace erpc
#endif

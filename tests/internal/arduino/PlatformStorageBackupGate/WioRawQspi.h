#ifndef BLINKER_TEST_WIO_RAW_QSPI_H
#define BLINKER_TEST_WIO_RAW_QSPI_H

// Compatibility filename for the USB maintenance gate. No raw/native driver
// lives here: all calls use the production owner and its fixed region leases.
#include <Arduino.h>
#include <BlinkerV2Advanced.h>
#include "WioOwnedQspi.h"

namespace storage_gate_qspi {
inline uint32_t clockNow() { return static_cast<uint32_t>(millis()); }
inline OwnedQspiAccess& access() {
    static OwnedQspiAccess value(blinker::wioTerminalQspiOwner(), &clockNow);
    return value;
}
class Transaction {
public:
    Transaction(bool includeTasks = false, bool restoreTasks = false) : ready_(false), owns_(false) {
        if (access().busy()) return;
        owns_ = true;
        uint32_t capacity = 0U;
        ready_ = access().begin(capacity, includeTasks, restoreTasks) &&
                 capacity == OwnedQspiAccess::capacityBytes;
    }
    ~Transaction() { if (owns_) access().end(); }
    bool ready() const { return ready_; }
    bool complete(bool result) {
        if (!owns_) return false; // Nested/rejected transactions own no leases.
        owns_ = false;
        const bool closed = access().finish();
        return ready_ && result && closed;
    }
private:
    bool ready_, owns_;
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
};
} // namespace storage_gate_qspi
#endif

#ifndef BLINKER_TEST_WIO_OWNED_QSPI_H
#define BLINKER_TEST_WIO_OWNED_QSPI_H

#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalQspi.h>
#include <string.h>

namespace storage_gate_qspi {

// Internal fixed-region maintenance seam. It uses the production owner; it
// neither copies the native driver nor proves file-system region ownership.
// One begin/end session has a single, shared admission/late-result deadline.
class OwnedQspiAccess {
public:
    typedef blinker::WioTerminalQspiOwner Owner;
    enum : uint32_t {
        capacityBytes = Owner::expectedCapacity,
        identityAddress = Owner::identityAddress,
        taskAddress = Owner::taskAddress,
        sectorBytes = Owner::eraseBlockSize
    };

    OwnedQspiAccess(Owner& owner, Owner::Now now)
        : owner_(owner), now_(now), started_(0U), budget_(0U),
          tokens_(), leases_(0U), opened_(false), failed_(false),
          includesTasks_(false), restoresTasks_(false), ioActive_(false) {}

    bool begin(uint32_t& capacity, bool includeTasks = false,
               bool restoreTasks = false, uint32_t budgetMillis = 10000U) {
        capacity = 0U;
        if (failed_ || busy() || now_ == nullptr || budgetMillis == 0U ||
            budgetMillis > INT32_MAX || (restoreTasks && !includeTasks)) return false;
        started_ = now_(); budget_ = budgetMillis;
        includesTasks_ = includeTasks; restoresTasks_ = restoreTasks;
        if (!owner_.acquire(&tokens_[0], Owner::Region::Identity)) return fail();
        leases_ = 1U;
        if (includeTasks) {
            if (!owner_.acquire(&tokens_[1], Owner::Region::Tasks)) return fail();
            leases_ = 3U;
        }
        opened_ = true;
        if (!withinDeadline(0U)) return false;
        capacity = owner_.capacity();
        return capacity == capacityBytes || fail();
    }

    // Healthy sessions release both leases. An uncertain native call already
    // quarantined the owner, whose release deliberately performs no cleanup IO.
    // failed_ is permanent for this helper; end/begin cannot unlock it.
    void end() {
        if (ioActive_) return; // A yield callback cannot release a sibling lease.
        if (leases_ != 0U && !owner_.quarantined() && remainingBudget() == 0U)
            (void)withinDeadline((leases_ & 1U) != 0U ? 0U : 1U);
        if ((leases_ & 2U) != 0U && owner_.release(&tokens_[1])) leases_ &= 1U;
        if ((leases_ & 1U) != 0U && owner_.release(&tokens_[0])) leases_ &= 2U;
        if (leases_ == 0U) opened_ = false;
    }

    // A successful read/compare is not complete until close has checked the
    // shared deadline. Do not publish success from a destructor's ignored end.
    bool finish() {
        if (ioActive_) return false;
        const bool opened = opened_;
        end();
        if (!opened || failed_ || owner_.quarantined() || busy()) return false;
        // Native end is void. A late cleanup cannot be reported as confirmed;
        // leases are already released, so lock this helper, not a fake owner.
        return remainingBudget() != 0U || fail();
    }

    bool read(uint32_t address, uint8_t* output, size_t size) {
        if (ioActive_) return false;
        if (!usable() || output == nullptr || !range(address, size)) return fail();
        IoGuard guard(ioActive_);
        while (size != 0U) {
            const uint8_t index = address < identityAddress ? 1U : 0U;
            const size_t remaining = (index == 1U ? identityAddress : capacityBytes) - address;
            const size_t count = size < remaining ? size : remaining;
            if (!withinDeadline(index)) return false;
            Owner::Operation operation(owner_, &tokens_[index], remainingBudget());
            if (!operation.result() || !owner_.read(&tokens_[index], address, output, count)) return fail();
            address += static_cast<uint32_t>(count); output += count; size -= count;
        }
        return true;
    }

    bool matches(uint32_t address, const uint8_t* expected, size_t size) {
        if (ioActive_) return false;
        if (expected == nullptr || !range(address, size)) return fail();
        uint8_t actual[64U] = {};
        while (size != 0U) {
            const size_t count = size < sizeof(actual) ? size : sizeof(actual);
            const bool equal = read(address, actual, count) && memcmp(actual, expected, count) == 0;
            wipe(actual, sizeof(actual));
            if (!equal) return fail();
            address += static_cast<uint32_t>(count); expected += count; size -= count;
        }
        return true;
    }

    // The ONLY mutation primitive: an aligned, fixed owned sector. Before and
    // after compare cover every padding byte. The whole sector transaction
    // holds one owner Operation, including page admission and exact read-back.
    bool restoreSector(uint32_t address, const uint8_t* before, const uint8_t* desired) {
        if (ioActive_) return false;
        const uint8_t index = includesTasks_ ? 1U : 0U;
        const uint32_t base = index == 1U ? taskAddress : identityAddress;
        if (!usable() || before == nullptr || desired == nullptr ||
            (includesTasks_ && !restoresTasks_) ||
            (address != base && address != base + sectorBytes)) return fail();
        IoGuard guard(ioActive_);
        if (!withinDeadline(index)) return false;
        Owner::Operation operation(owner_, &tokens_[index], remainingBudget());
        if (!operation.result()) return fail();
        if (!compareInOperation(index, address, before)) return false;
        if (memcmp(before, desired, sectorBytes) == 0) return true;
        if (!owner_.erase(&tokens_[index], address)) return fail();
        for (uint32_t offset = 0U; offset < sectorBytes; offset += 256U)
            if (!owner_.program(&tokens_[index], address + offset, desired + offset, 256U)) return fail();
        if (!compareInOperation(index, address, desired)) {
            // A confirmed read with different committed bytes is uncertain,
            // not permission to replay, read again, or reset the peripheral.
            if (!owner_.quarantined()) (void)owner_.verificationFailed(&tokens_[index]);
            return false;
        }
        return true;
    }

    bool failed() const { return failed_; }
    bool quarantined() const { return owner_.quarantined(); }
    bool busy() const { return opened_ || leases_ != 0U || ioActive_; }

private:
    class IoGuard {
    public:
        explicit IoGuard(bool& flag) : flag_(flag) { flag_ = true; }
        ~IoGuard() { flag_ = false; }
    private:
        bool& flag_;
    };
    bool fail() { failed_ = true; return false; }
    bool usable() const { return opened_ && !failed_ && !owner_.quarantined(); }
    uint32_t remainingBudget() const {
        const uint32_t elapsed = now_() - started_;
        return elapsed < budget_ ? budget_ - elapsed : 0U;
    }
    bool withinDeadline(uint8_t index) {
        if (remainingBudget() != 0U) return true;
        // No native call occurs here. Make the owner sticky as well, so a
        // last-lease release cannot issue cleanup after the session expired.
        Owner::Operation operation(owner_, &tokens_[index], 1U);
        if (operation.result()) (void)owner_.verificationFailed(&tokens_[index]);
        return fail();
    }
    bool range(uint32_t address, size_t size) const {
        const uint32_t base = includesTasks_ ? taskAddress : identityAddress;
        return size != 0U && address >= base && address <= capacityBytes &&
               size <= capacityBytes - address;
    }
    static void wipe(uint8_t* bytes, size_t size) {
        volatile uint8_t* p = bytes; while (size-- != 0U) *p++ = 0U;
    }
    bool compareInOperation(uint8_t index, uint32_t address, const uint8_t* expected) {
        uint8_t actual[64U] = {};
        for (uint32_t offset = 0U; offset < sectorBytes; offset += sizeof(actual)) {
            const bool equal = owner_.read(&tokens_[index], address + offset, actual, sizeof(actual)) &&
                               memcmp(actual, expected + offset, sizeof(actual)) == 0;
            wipe(actual, sizeof(actual));
            if (!equal) return fail();
        }
        return true;
    }

    Owner& owner_;
    Owner::Now now_;
    uint32_t started_, budget_;
    uint8_t tokens_[2], leases_;
    bool opened_, failed_, includesTasks_, restoresTasks_, ioActive_;
    OwnedQspiAccess(const OwnedQspiAccess&) = delete;
    OwnedQspiAccess& operator=(const OwnedQspiAccess&) = delete;
};

} // namespace storage_gate_qspi
#endif

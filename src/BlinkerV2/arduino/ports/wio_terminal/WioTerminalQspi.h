#ifndef BLINKER_V2_WIO_TERMINAL_QSPI_H
#define BLINKER_V2_WIO_TERMINAL_QSPI_H

#include <BlinkerV2/core/Result.h>
#include <stddef.h>
#include <stdint.h>

namespace blinker {

// Small native-flash seam, not a file system or a second task executor.
class IWioTerminalQspiFlash {
public:
    virtual ~IWioTerminalQspiFlash() {}
    virtual bool begin(uint32_t& capacity) = 0;
    virtual void end() = 0;
    virtual bool read(uint32_t address, uint8_t* output, size_t size) = 0;
    virtual bool erase(uint32_t address) = 0;
    virtual bool program(uint32_t address, const uint8_t* input, size_t size) = 0;
};

// One cooperative owner for the physical QSPI peripheral. Blinker products
// reserve the final 16 KiB even with tasks disabled; no independent onboard
// QSPI/FS driver may share this peripheral. SD is a separate storage device.
// These address bounds are contracts, NOT proof that blank sectors are free.
// The optional task profile must attest its region before acquiring a lease.
// A failed/late native call freezes this owner; there is no end/begin unlock.
// UART/WiFi on the separate RTL module remains independent of this latch.
class WioTerminalQspiOwner {
public:
    typedef uint32_t (*Now)();
    enum class Region : uint8_t { Identity, Tasks };
    enum Outcome : uint8_t { None, DeadlineBeforeWrite, ReadUncertain, WriteUncertain };
    enum : uint32_t {
        expectedCapacity = 4UL * 1024UL * 1024UL,
        eraseBlockSize = 4096U,
        regionSize = eraseBlockSize * 2U,
        taskAddress = expectedCapacity - regionSize * 2U,
        identityAddress = expectedCapacity - regionSize,
        reservedAddress = taskAddress,
        reservedSize = regionSize * 2U,
        identityBudgetMillis = 10000U
    };
    static_assert(taskAddress % eraseBlockSize == 0U &&
                  identityAddress % eraseBlockSize == 0U &&
                  taskAddress + regionSize == identityAddress &&
                  reservedAddress + reservedSize == expectedCapacity,
                  "Wio QSPI layout must retain four disjoint tail sectors");

    WioTerminalQspiOwner(IWioTerminalQspiFlash& flash, Now now)
        : flash_(flash), now_(now), leases_(), active_(nullptr), capacity_(0U),
          started_(0U), budget_(0U), outcome_(None), opened_(false),
          quarantined_(false), wrote_(false) {}

    Result acquire(const void* token, Region region);
    Result release(const void* token);
    uint32_t regionAddress(Region region) const;
    uint32_t capacity() const { return capacity_; }
    bool quarantined() const { return quarantined_; }
    Outcome outcome() const { return outcome_; }

    class Operation {
    public:
        Operation(WioTerminalQspiOwner& owner, const void* token, uint32_t budgetMillis)
            : owner_(owner), token_(token), result_(owner.start(token, budgetMillis)) {}
        ~Operation() { if (result_) owner_.finish(token_); }
        const Result& result() const { return result_; }
    private:
        WioTerminalQspiOwner& owner_;
        const void* token_;
        Result result_;
        Operation(const Operation&) = delete;
        Operation& operator=(const Operation&) = delete;
    };

    Result read(const void* token, uint32_t address, uint8_t* output, size_t size);
    Result erase(const void* token, uint32_t address);
    Result program(const void* token, uint32_t address, const uint8_t* input, size_t size);
    // A native read returned, but committed bytes failed exact verification.
    Result verificationFailed(const void* token);

private:
    int regionIndex(const void* token) const;
    bool inRange(const void* token, uint32_t address, size_t size) const;
    Result start(const void* token, uint32_t budgetMillis);
    void finish(const void* token);
    Result prepare(const void* token);
    Result complete(bool confirmed);

    IWioTerminalQspiFlash& flash_;
    Now now_;
    const void* leases_[2];
    const void* active_;
    uint32_t capacity_, started_, budget_;
    Outcome outcome_;
    bool opened_, quarantined_, wrote_;
    WioTerminalQspiOwner(const WioTerminalQspiOwner&) = delete;
    WioTerminalQspiOwner& operator=(const WioTerminalQspiOwner&) = delete;
};

WioTerminalQspiOwner& wioTerminalQspiOwner();

} // namespace blinker
#endif

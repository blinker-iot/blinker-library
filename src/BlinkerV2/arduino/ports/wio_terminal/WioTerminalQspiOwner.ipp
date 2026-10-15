#ifndef BLINKER_V2_WIO_TERMINAL_QSPI_OWNER_IPP
#define BLINKER_V2_WIO_TERMINAL_QSPI_OWNER_IPP

#include "WioTerminalQspi.h"

// These guards bound call admission and detect late results. The unchanged
// native cache/AHB transactions and busy wait are not a preemptible hard deadline.

namespace blinker {

int WioTerminalQspiOwner::regionIndex(const void* token) const {
    if (token == nullptr) return -1;
    for (uint8_t i = 0U; i < 2U; ++i) if (leases_[i] == token) return i;
    return -1;
}

uint32_t WioTerminalQspiOwner::regionAddress(Region region) const {
    if (capacity_ != expectedCapacity ||
        (region != Region::Identity && region != Region::Tasks)) return 0U;
    return capacity_ - (region == Region::Identity ? regionSize : regionSize * 2U);
}

Result WioTerminalQspiOwner::acquire(const void* token, Region region) {
    if (token == nullptr || now_ == nullptr ||
        (region != Region::Identity && region != Region::Tasks))
        return Result::failure(ErrorCode::InvalidArgument);
    if (quarantined_) return Result::failure(ErrorCode::StateConflict);
    if (active_ != nullptr) return Result::failure(ErrorCode::WouldBlock);
    const uint8_t index = static_cast<uint8_t>(region);
    if (leases_[index] == token) return Result::success();
    if (leases_[index] != nullptr || regionIndex(token) >= 0)
        return Result::failure(ErrorCode::WouldBlock);
    if (!opened_) {
        // begin may yield; block all other callers during that lifecycle call.
        active_ = token;
        const uint32_t started = now_();
        uint32_t found = 0U;
        const bool confirmed = flash_.begin(found);
        active_ = nullptr;
        if (!confirmed || found != expectedCapacity ||
            now_() - started >= identityBudgetMillis) {
            quarantined_ = true;
            outcome_ = ReadUncertain;
            return Result::failure(ErrorCode::StateConflict);
        }
        capacity_ = found;
        opened_ = true;
    }
    leases_[index] = token;
    return Result::success();
}

Result WioTerminalQspiOwner::release(const void* token) {
    const int index = regionIndex(token);
    if (index < 0) return Result::failure(ErrorCode::NotConfigured);
    if (active_ == token) return Result::failure(ErrorCode::WouldBlock);
    leases_[index] = nullptr;
    if (leases_[0] == nullptr && leases_[1] == nullptr) {
        // Do not send cleanup/reset commands after an uncertain native call.
        if (opened_ && !quarantined_) {
            active_ = token;
            flash_.end();
            active_ = nullptr;
        }
        opened_ = false;
        capacity_ = 0U;
    }
    return Result::success();
}

Result WioTerminalQspiOwner::start(const void* token, uint32_t budgetMillis) {
    if (token == nullptr || now_ == nullptr || budgetMillis == 0U || budgetMillis > INT32_MAX)
        return Result::failure(ErrorCode::InvalidArgument);
    if (quarantined_) return Result::failure(ErrorCode::StateConflict);
    if (!opened_ || regionIndex(token) < 0) return Result::failure(ErrorCode::NotConfigured);
    if (active_ != nullptr) return Result::failure(ErrorCode::WouldBlock);
    active_ = token;
    started_ = now_();
    budget_ = budgetMillis;
    wrote_ = false;
    outcome_ = None;
    return Result::success();
}

void WioTerminalQspiOwner::finish(const void* token) {
    if (active_ == token) active_ = nullptr;
}

bool WioTerminalQspiOwner::inRange(const void* token, uint32_t address, size_t size) const {
    const int index = regionIndex(token);
    if (index < 0 || capacity_ != expectedCapacity || size == 0U || size > regionSize) return false;
    const uint32_t base = regionAddress(static_cast<Region>(index));
    return address >= base && address - base <= regionSize &&
           size <= regionSize - (address - base);
}

Result WioTerminalQspiOwner::prepare(const void* token) {
    if (quarantined_) return Result::failure(ErrorCode::StateConflict);
    if (active_ != token || token == nullptr || regionIndex(token) < 0)
        return Result::failure(ErrorCode::NotConfigured);
    if (now_() - started_ >= budget_) {
        quarantined_ = true;
        outcome_ = wrote_ ? WriteUncertain : DeadlineBeforeWrite;
        return Result::failure(ErrorCode::WouldBlock);
    }
    return Result::success();
}

Result WioTerminalQspiOwner::complete(bool confirmed) {
    if (!confirmed || now_() - started_ >= budget_) {
        quarantined_ = true;
        outcome_ = wrote_ ? WriteUncertain : ReadUncertain;
        return Result::failure(ErrorCode::WouldBlock);
    }
    return Result::success();
}

Result WioTerminalQspiOwner::read(
    const void* token, uint32_t address, uint8_t* output, size_t size) {
    if (output == nullptr || !inRange(token, address, size))
        return Result::failure(ErrorCode::InvalidArgument);
    const Result ready = prepare(token);
    return ready ? complete(flash_.read(address, output, size)) : ready;
}

Result WioTerminalQspiOwner::erase(const void* token, uint32_t address) {
    if ((address % eraseBlockSize) != 0U || !inRange(token, address, eraseBlockSize))
        return Result::failure(ErrorCode::InvalidArgument);
    const Result ready = prepare(token);
    if (!ready) return ready;
    wrote_ = true;
    return complete(flash_.erase(address));
}

Result WioTerminalQspiOwner::program(
    const void* token, uint32_t address, const uint8_t* input, size_t size) {
    if (input == nullptr || !inRange(token, address, size))
        return Result::failure(ErrorCode::InvalidArgument);
    const Result ready = prepare(token);
    if (!ready) return ready;
    wrote_ = true;
    return complete(flash_.program(address, input, size));
}

Result WioTerminalQspiOwner::verificationFailed(const void* token) {
    if (quarantined_) return Result::failure(ErrorCode::StateConflict);
    if (active_ != token || token == nullptr) return Result::failure(ErrorCode::NotConfigured);
    return complete(false);
}

} // namespace blinker



#endif

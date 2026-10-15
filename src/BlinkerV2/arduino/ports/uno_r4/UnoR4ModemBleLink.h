#ifndef BLINKER_UNO_R4_MODEM_BLE_LINK_H
#define BLINKER_UNO_R4_MODEM_BLE_LINK_H

#include "UnoR4ModemOwner.h"
#include <BlinkerV2/interface/IBleModeLink.h>

namespace blinker {

// R4 ArduinoBLE's HCI transport uses the SAME AT modem as Preferences.
// Keep the guard in this port, not in the shared ArduinoBLE implementation.
template <typename Native>
class RenesasUnoModemBleLink : public Native {
public:
    template <typename Config>
    explicit RenesasUnoModemBleLink(const Config& config) : Native(config) {}
    Result start() override { return allowed() ? Native::start() : blocked(); }
    void stop() override { if (allowed()) Native::stop(); }
    void poll(uint32_t budget) override { if (allowed()) Native::poll(budget); }
    Result sendPacket(uint32_t session, ByteView packet) override {
        return allowed() ? Native::sendPacket(session, packet) : blocked();
    }
    Result disconnectSession(uint32_t session) override {
        return allowed() ? Native::disconnectSession(session) : blocked();
    }
    Result configureBleProfile(const ble::ModeProfile& profile) override {
        return allowed() ? Native::configureBleProfile(profile) : blocked();
    }
    BleLinkState state() const override { return allowed() ? Native::state() : BleLinkState::Error; }
    ErrorCode lastError() const override { return allowed() ? Native::lastError() : ErrorCode::StateConflict; }
    size_t sessionCount() const override { return allowed() ? Native::sessionCount() : 0U; }
    Result sessionAt(size_t index, BleSessionInfo& session) const override {
        if (allowed()) return Native::sessionAt(index, session);
        session = BleSessionInfo(); return blocked();
    }
private:
    static bool allowed() { return renesasUnoModemOwner().networkAllowed(); }
    static Result blocked() { return Result::failure(ErrorCode::StateConflict); }
};

} // namespace blinker
#endif

#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/wio_terminal/WioTerminalStorage.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>

namespace {

constexpr uint32_t kMaximumCommits = 120U;

blinker::WioTerminalFlashBlobBank bank;
blinker::DeviceAccessStore access(bank.deviceAccessBlob());
bool ready = false;
bool running = false;
uint32_t sequence = 0U;
uint32_t nextReadyReportAt = 0U;

bool validateAccess() {
    size_t controllers = 0U;
    uint32_t accessEpoch = 0U;
    blinker::DevicePresenceKey presence;
    return access.count(controllers) &&
           access.loadAccessEpoch(accessEpoch) &&
           access.loadPresence(accessEpoch, presence) &&
           blinker::validateDevicePresenceKey(presence);
}

bool rewriteIdenticalAccess() {
    uint8_t bytes[blinker::DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    blinker::IAtomicBlobStore& blob = bank.deviceAccessBlob();
    return blob.load(blinker::MutableByteSpan(bytes, sizeof(bytes)), written) &&
           written == sizeof(bytes) &&
           blob.replace(blinker::ByteView(bytes, written)) &&
           validateAccess();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 5000U;
    while (!Serial && static_cast<int32_t>(millis() - serialDeadline) < 0) {
    }

    Serial.println(F("[fault-probe] BOOT"));
    ready = bank.begin() && validateAccess();
    Serial.println(ready
                       ? F("[fault-probe] BOOT_VALID bytes=196")
                       : F("[fault-probe] BOOT_INVALID"));
    Serial.println(F("[fault-probe] send 's' to start"));
    nextReadyReportAt = millis() + 5000U;
}

void loop() {
    if (Serial.available() && Serial.read() == 's' && ready && !running) {
        running = true;
        Serial.println(F("[fault-probe] WINDOW_ACTIVE"));
    }
    if (!running) {
        const uint32_t now = millis();
        if (static_cast<int32_t>(now - nextReadyReportAt) >= 0) {
            Serial.println(ready
                               ? F("[fault-probe] READY bytes=196")
                               : F("[fault-probe] NOT_READY"));
            nextReadyReportAt = now + 5000U;
        }
        return;
    }

    if (!rewriteIdenticalAccess()) {
        running = false;
        ready = false;
        Serial.println(F("[fault-probe] WRITE_FAILED"));
        return;
    }
    ++sequence;
    if ((sequence % 10U) == 0U) {
        Serial.print(F("[fault-probe] COMMIT sequence="));
        Serial.println(sequence);
    }
    if (sequence >= kMaximumCommits) {
        running = false;
        Serial.println(F("[fault-probe] WINDOW_COMPLETE"));
    }
}

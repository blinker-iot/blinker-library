// ESP32-S3 hardware gate ONLY. Build with CDC on boot DISABLED and USB mode
// Hardware CDC/JTAG. This sketch explicitly owns the IDF console; it must not
// call Serial.begin(), install HWCDC or enable SDK USB console writers.
// No automatic access reset, no credentials or arbitrary payload logging.
#include <BlinkerBLE.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32UsbDiagnosticOutput.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S3) || ARDUINO_USB_CDC_ON_BOOT
#error "DiagnosticOutputGate requires ESP32-S3 and CDC on boot disabled"
#endif

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

blinker::esp32::Esp32UsbDiagnosticOutput diagnosticOutput;
uint32_t pressCount = 0U;
uint32_t runMaxMicros = 0U;
uint32_t outputMaxMicros = 0U;
uint32_t loops = 0U;
uint32_t nextNoise = 0U;
bool noiseEnabled = true;
bool consoleReady = false;

void onPower(bool value) { power.report(value); }
void onButton() { buttonPresses.report(++pressCount); }

void setup() {
    // Driver-owned 256 B TX ring, not a Blinker log queue. Admission is a
    // single write with ticks_to_wait=0 for both mutex and ring-buffer send.
    usb_serial_jtag_driver_config_t config = {};
    config.tx_buffer_size = 256U;
    config.rx_buffer_size = 128U;
    consoleReady = usb_serial_jtag_driver_install(&config) == ESP_OK;
    if (consoleReady) Blinker.debug(diagnosticOutput, blinker::LogLevel::Trace);
    power.onWrite(onPower);
    button.onInvoke(onButton);
    if (Blinker.begin(power, buttonPresses, button)) {
        Blinker.report(power, false, buttonPresses, pressCount);
    }
}

void loop() {
    const uint32_t start = micros();
    Blinker.run();
    const uint32_t elapsed = micros() - start;
    if (elapsed > runMaxMicros) runMaxMicros = elapsed;
    ++loops;

    // Synthetic numeric error events stress diagnostic output only, not BBP,
    // property revisions, MQTT or the user's business traffic limiter.
    const uint32_t now = millis();
    if (noiseEnabled && static_cast<int32_t>(now - nextNoise) >= 0) {
        nextNoise = now + 2U;
        const uint32_t outputStart = micros();
        auto& diagnostics = blinker::facade_detail::diagnostics();
        diagnostics.recordProductStatus(0U, static_cast<uint16_t>(loops));
        diagnostics.flush();
        const uint32_t outputElapsed = micros() - outputStart;
        if (outputElapsed > outputMaxMicros) outputMaxMicros = outputElapsed;
    }
    uint8_t command = 0U;
    if (!consoleReady || usb_serial_jtag_read_bytes(&command, 1U, 0) != 1) return;
    if (command == 'p') noiseEnabled = false;
    if (command == 'g') {
        runMaxMicros = outputMaxMicros = loops = 0U;
        noiseEnabled = true;
    }
    if (command == 's') {
        char response[180];
        const int length = snprintf(response, sizeof(response),
            "D1B loops=%lu runMaxUs=%lu outputMaxUs=%lu dropped=%lu local=%u button=%lu\r\n",
            static_cast<unsigned long>(loops), static_cast<unsigned long>(runMaxMicros),
            static_cast<unsigned long>(outputMaxMicros),
            static_cast<unsigned long>(Blinker.diagnosticCounters().outputDropped),
            Blinker.status().localReady ? 1U : 0U, static_cast<unsigned long>(pressCount));
        if (length > 0 && static_cast<size_t>(length) < sizeof(response)) {
            diagnosticOutput.tryWrite(reinterpret_cast<const uint8_t*>(response),
                                      static_cast<size_t>(length));
        }
    }
}

#include <BlinkerV2Advanced.h>
#include <UnoR4Storage.h>

blinker::RenesasUnoPreferencesBlobBank storage;
blinker::DeviceInstanceIdStore deviceInstance(
    storage.deviceInstanceBlob());
blinker::DeviceAccessStore access(storage.deviceAccessBlob());

void setup() {
    if (!storage.begin()) return;
    (void)deviceInstance.protection();
    (void)access.protection();
    storage.end();
}

void loop() {}

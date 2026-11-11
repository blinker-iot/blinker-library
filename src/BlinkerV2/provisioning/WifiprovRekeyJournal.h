#ifndef BLINKER_PROVISIONING_WIFIPROVREKEYJOURNAL_H
#define BLINKER_PROVISIONING_WIFIPROVREKEYJOURNAL_H

#include "WifiprovRekeyEndpoint.h"

namespace blinker {

enum class WifiprovRekeyJournalPhase : uint8_t { AttemptReserved = 1U, ApplyConsumed = 2U, Complete = 3U };

struct WifiprovRekeyJournalRecord {
    security::WifiprovRekeyBinding binding;
    WifiprovRekeyAdmin admin;
    WifiprovRekeyJournalPhase phase = WifiprovRekeyJournalPhase::AttemptReserved;
    uint32_t networkGeneration = 0U;
};

// Dedicated, single-record port for the restricted physical setup candidate.
// Use only under its serialized owner. The same object must outlive an
// uncertain storage handle; reopen on a confirmed fresh storage lifecycle,
// never merely on BLE reconnect. No expiry, overwrite, replay or clear API.
class WifiprovRekeyJournal {
public:
    enum : size_t { serializedSize = 121U };

    explicit WifiprovRekeyJournal(IAtomicBlobStore& storage) : storage_(storage), uncertain_(false) {}

    Result inspect(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin,
        WifiprovRekeyJournalPhase& phase);
    Result read(WifiprovRekeyJournalRecord& record);
    Result reserveAttempt(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin);
    Result consumeApply(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin);
    // Called only after official Wi-Fi success, real owner exit, matching
    // durable Key, and confirmation of the new network generation.
    Result complete(const security::WifiprovRekeyBinding& binding, const WifiprovRekeyAdmin& admin,
        uint32_t networkGeneration);

private:
    Result write(ByteView bytes);

    IAtomicBlobStore& storage_;
    bool uncertain_;
    WifiprovRekeyJournal(const WifiprovRekeyJournal&) = delete;
    WifiprovRekeyJournal& operator=(const WifiprovRekeyJournal&) = delete;
};

} // namespace blinker

#endif

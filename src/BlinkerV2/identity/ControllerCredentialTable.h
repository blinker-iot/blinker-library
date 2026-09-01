#ifndef BLINKER_IDENTITY_CONTROLLERCREDENTIALTABLE_H
#define BLINKER_IDENTITY_CONTROLLERCREDENTIALTABLE_H

#include "ControllerCredential.h"

namespace blinker {

// Allocation-free in-memory table and canonical 64-byte slot codec used by
// DeviceAccessStore. It performs no persistence itself and stores no account
// role; the two non-admin slots are distinguished by Service metadata.
class ControllerCredentialTable {
public:
    enum : size_t {
        capacity = 3U,
        slotSize = 64U,
        encodedSize = capacity * slotSize
    };

    ControllerCredentialTable();
    ~ControllerCredentialTable();

    Result decode(ByteView slots, size_t count);
    Result encode(MutableByteSpan slots) const;

    size_t count() const { return count_; }
    const ControllerCredential& at(size_t index) const {
        return credentials_[index];
    }
    Result loadActive(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        ControllerCredential& output) const;
    Result install(
        const ControllerCredential& credential,
        bool& changed);
    Result rotate(
        uint32_t expectedCredentialVersion,
        const ControllerCredential& credential,
        bool& changed);
    Result revoke(
        ByteView controllerId,
        ControllerCredentialDomain domain,
        uint32_t accessEpoch,
        uint32_t expectedCredentialVersion,
        bool& changed);
    bool hasManager(uint32_t accessEpoch) const;
    void clear();

private:
    size_t lowerBound(ByteView controllerId) const;

    ControllerCredential credentials_[capacity];
    size_t count_;

    ControllerCredentialTable(const ControllerCredentialTable&);
    ControllerCredentialTable& operator=(
        const ControllerCredentialTable&);
};

} // namespace blinker

#endif

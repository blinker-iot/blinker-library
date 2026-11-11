#include <BlinkerV2/arduino/platform/esp32/Esp32GatewayStorage.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
using namespace blinker;
#define CHECK(value) do { if (!(value)) { std::cerr << "Gateway NVS line " << __LINE__ << ": " << #value << '\n'; std::exit(1); } } while (false)

// Multi-handle/key NVS boundary, with conservative commit/power-loss faults.
// Not a hardware NVS crash-consistency proof; production blob/record code runs
// unchanged, unlike a map that bypasses the actual NVS adapter and name mapping.
struct Value { std::vector<uint8_t> durable, working; };
struct Handle { std::string prefix, lastKey; };
std::map<std::string, Value> values;
std::map<nvs_handle_t, Handle> handles;
nvs_handle_t nextHandle = 0;
std::string failOpen, failGet, failSet, failErase, failCommit;
size_t writes = 0, erases = 0, openCalls = 0, failOpenAt = 0;
const std::string access0 = "protected/bl_eh_access/child", access1 = "protected/bl_eh_access/child_1";
const std::string renew0 = "protected/bl_eh_renew/pending", renew1 = "protected/bl_eh_renew/pending_1";
const std::string rootKey = "protected/bl_eh_key/root";

void powerLoss() {
    handles.clear();
    for (auto& value : values) value.second.working = value.second.durable;
    failOpen.clear(); failGet.clear(); failSet.clear(); failErase.clear(); failCommit.clear();
}
bool encrypted(const char* partition) { return std::strcmp(partition, "protected") == 0; }
Esp32NvsBlobStoreConfig config(const char* name = "ignored", const char* key = "ignored") {
    Esp32NvsBlobStoreConfig result;
    result.namespaceName = name; result.key = key; result.partitionLabel = "protected";
    result.encrypted = true; result.encryptionVerifier = &encrypted;
    result.maximumSize = GatewayCredentialRenewalStore::serializedSize;
    return result;
}
GatewayAccessRecord accessRecord(uint8_t seed) {
    GatewayAccessRecord record;
    const char name[] = "child-a";
    record.childLogicalDeviceIdSize = sizeof(name) - 1;
    std::memcpy(record.childLogicalDeviceId, name, sizeof(name) - 1);
    record.childLogicalDeviceId[sizeof(name) - 2] = static_cast<char>('a' + seed);
    std::memset(record.operationId, seed, sizeof(record.operationId));
    std::memset(record.childDeviceInstanceId, seed, sizeof(record.childDeviceInstanceId));
    std::memset(record.envelopeDigest, seed, sizeof(record.envelopeDigest));
    std::memset(record.accessMaterialDigest, seed, sizeof(record.accessMaterialDigest));
    std::memset(record.controllerId, seed, sizeof(record.controllerId));
    std::memset(record.secret, seed, sizeof(record.secret));
    std::memset(record.activePresence.key, seed, sizeof(record.activePresence.key));
    record.accessEpoch = 3; record.credentialVersion = 4; record.activePresence.version = 2;
    record.permissions = gateway::kGatewayPermissions; record.expiresAtUnixSeconds = 2000000000ULL;
    return record;
}
GatewayCredentialRenewalRecord renewalRecord(uint8_t seed) {
    GatewayCredentialRenewalRecord record;
    std::memset(record.renewalOperationId, seed + 10, sizeof(record.renewalOperationId));
    std::memset(record.topologyOperationId, seed, sizeof(record.topologyOperationId));
    std::memset(record.childDeviceInstanceId, seed, sizeof(record.childDeviceInstanceId));
    std::memset(record.controllerId, seed, sizeof(record.controllerId));
    std::memset(record.secret, seed + 1, sizeof(record.secret));
    std::memset(record.accessMaterialDigest, seed + 1, sizeof(record.accessMaterialDigest));
    record.accessEpoch = 3; record.expectedCredentialVersion = 4; record.credentialVersion = 5;
    record.presenceKeyVersion = 2; record.expiresAtUnixSeconds = 2000000000ULL;
    return record;
}
void checkIdentity(GatewayChildRecords& records, uint8_t seed) {
    const GatewayChildIdentity* identity = nullptr; CHECK(records.identity(identity));
    CHECK(identity->state == GatewayChildRecordState::Active && identity->instance[0] == seed);
}

void testLegacyAndIndependentBindings() {
    GatewayAccessCommitOutcome outcome; GatewayCredentialRenewalCommitOutcome renewalOutcome;
    // Create the deployed v1 slot names without using the new bank.
    {
        Esp32NvsAtomicBlobStore active(config("bl_eh_access", "child")), pending(config("bl_eh_renew", "pending"));
        CHECK(active.begin()); CHECK(pending.begin());
        GatewayAccessStore access(active); GatewayCredentialRenewalStore renewal(pending);
        CHECK(access.commit(accessRecord(1), outcome)); CHECK(renewal.commit(renewalRecord(1), renewalOutcome));
    }
    const auto originalAccess = values[access0].durable, originalRenewal = values[renew0].durable;
    {
        BasicEsp32GatewayStorage<> bank(config());
        const size_t before = writes; CHECK(bank.begin()); CHECK(bank.begin());
        CHECK(handles.size() == 4 && before == writes && erases == 0);
        CHECK(bank.records(2) == nullptr && bank.records(static_cast<size_t>(-1)) == nullptr);
        checkIdentity(*bank.records(0), 1);
        const GatewayChildIdentity* empty = nullptr; CHECK(bank.records(1)->identity(empty));
        CHECK(empty->state == GatewayChildRecordState::Empty);
        CHECK(bank.records(1)->access.commit(accessRecord(2), outcome));
        CHECK(bank.records(1)->renewal.commit(renewalRecord(2), renewalOutcome));
        checkIdentity(*bank.records(1), 2);
        CHECK(values[access0].durable == originalAccess && values[renew0].durable == originalRenewal);
        CHECK(!values[access1].durable.empty() && !values[renew1].durable.empty());
        // Damaged or unreadable storage is not vacant capacity.
        const auto intact = values[access1].working;
        values[access1].working[0] ^= 1; bank.records(1)->invalidate();
        CHECK(!bank.records(1)->identity(empty)); CHECK(empty == nullptr);
        values[access1].working = intact;
        failGet = access1; bank.records(1)->invalidate();
        CHECK(!bank.records(1)->identity(empty)); CHECK(empty == nullptr);
        failGet.clear(); bank.end();
        CHECK(!bank.records(0)->identity(empty));
    }
    powerLoss(); // Reconstruct ALL RAM, not just a second reference to a cached index.
    {
        BasicEsp32GatewayStorage<> bank(config()); CHECK(bank.begin());
        checkIdentity(*bank.records(1), 2); checkIdentity(*bank.records(0), 1);
        const size_t before = writes;
        CHECK(bank.records(0)->access.commit(accessRecord(1), outcome));
        CHECK(outcome.kind == GatewayAccessCommitKind::ExactReplay && writes == before);
        CHECK(bank.records(1)->renewal.commit(renewalRecord(2), renewalOutcome));
        CHECK(renewalOutcome.kind == GatewayCredentialRenewalCommitKind::ExactReplay && writes == before);
        auto replacement = accessRecord(2); ++replacement.accessEpoch;
        failSet = access1; CHECK(!bank.records(1)->access.commit(replacement, outcome));
        failSet.clear(); failCommit = access1; CHECK(!bank.records(1)->access.commit(replacement, outcome));
        CHECK(values[access0].durable == originalAccess && values[renew0].durable == originalRenewal);
        bank.end();
    }
    powerLoss();
    {
        BasicEsp32GatewayStorage<> bank(config()); CHECK(bank.begin());
        GatewayAccessRecord loaded; CHECK(bank.records(1)->access.load(loaded)); CHECK(loaded.accessEpoch == 3);
        checkIdentity(*bank.records(0), 1);
    }
    CHECK(handles.empty());
}

void testPartialResetAndLifecycle() {
    Esp32NvsAtomicBlobStore root(config("bl_eh_key", "root")); CHECK(root.begin());
    const uint8_t rootBytes[] = {1, 2, 3}; CHECK(root.replace(ByteView(rootBytes, sizeof(rootBytes))));
    const auto originalRoot = values[rootKey].durable;
    {
        BasicEsp32GatewayStorage<> bank(config()); CHECK(bank.begin());
        failErase = renew1; CHECK(!bank.clear());
        CHECK(values[access0].durable.empty() && values[renew0].durable.empty());
        CHECK(!values[access1].durable.empty() && !values[renew1].durable.empty());
        CHECK(values[rootKey].durable == originalRoot); // Product must not clear root on failure.
        bank.end();
    }
    root.end(); powerLoss();
    {
        BasicEsp32GatewayStorage<> bank(config()); CHECK(bank.begin());
        failCommit = access1; CHECK(!bank.clear());
        CHECK(values[access1].working.empty() && !values[access1].durable.empty());
        // No reboot: a failed erase commit can now return NotFound. Retry must
        // still commit the absence, before the product may clear its root.
        failCommit.clear(); CHECK(bank.clear()); CHECK(bank.clear());
        CHECK(values[access1].durable.empty() && values[renew1].durable.empty());
        const GatewayChildIdentity* identity = nullptr;
        CHECK(bank.records(0)->identity(identity) && identity->state == GatewayChildRecordState::Empty);
        CHECK(bank.records(1)->identity(identity) && identity->state == GatewayChildRecordState::Empty);
    }
    CHECK(values[rootKey].durable == originalRoot);
    // Partial open must close every already-opened handle; never erase to
    // recover a partition/configuration failure.
    const size_t beforeErase = erases;
    failOpen = "protected/bl_eh_renew/";
    {
        BasicEsp32GatewayStorage<> bank(config()); CHECK(!bank.begin()); CHECK(handles.empty());
    }
    failOpen.clear(); CHECK(erases == beforeErase);
    for (size_t position = 1; position <= 4; ++position) {
        failOpenAt = openCalls + position;
        BasicEsp32GatewayStorage<> bank(config()); CHECK(!bank.begin()); CHECK(handles.empty());
        CHECK(erases == beforeErase && values[rootKey].durable == originalRoot);
    }
    failOpenAt = 0;
    auto rejected = config(); rejected.partitionLabel = "plain";
    {
        BasicEsp32GatewayStorage<> bank(rejected); CHECK(!bank.begin()); CHECK(handles.empty());
    }
    rejected.encrypted = false;
    {
        BasicEsp32GatewayStorage<> bank(rejected); CHECK(bank.begin());
        GatewayAccessCommitOutcome outcome;
        CHECK(bank.records(0)->access.commit(accessRecord(1), outcome).code() == ErrorCode::AuthenticationRequired);
        CHECK(bank.clear().code() == ErrorCode::AuthenticationRequired);
    }
    CHECK(values[rootKey].durable == originalRoot && handles.empty());
}

Handle* find(nvs_handle_t id) { auto it = handles.find(id); return it == handles.end() ? nullptr : &it->second; }
} // namespace

extern "C" esp_err_t nvs_open_from_partition(const char* partition, const char* name, nvs_open_mode_t, nvs_handle_t* output) {
    const std::string prefix = std::string(partition) + "/" + name + "/";
    ++openCalls;
    if (prefix == failOpen || openCalls == failOpenAt) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    *output = ++nextHandle; handles[*output].prefix = prefix; return ESP_OK;
}
extern "C" esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* output) {
    return nvs_open_from_partition("nvs", name, mode, output);
}
extern "C" void nvs_close(nvs_handle_t handle) { handles.erase(handle); }
extern "C" esp_err_t nvs_get_blob(nvs_handle_t id, const char* key, void* output, size_t* size) {
    auto* handle = find(id); if (handle == nullptr) return ESP_ERR_NVS_INVALID_HANDLE;
    const std::string path = handle->prefix + key;
    if (path == failGet) return ESP_ERR_NVS_NOT_INITIALIZED;
    const auto& bytes = values[path].working;
    if (bytes.empty()) return ESP_ERR_NVS_NOT_FOUND;
    if (output == nullptr) { *size = bytes.size(); return ESP_OK; }
    if (*size < bytes.size()) { *size = bytes.size(); return ESP_ERR_NVS_INVALID_LENGTH; }
    std::memcpy(output, bytes.data(), bytes.size()); *size = bytes.size(); return ESP_OK;
}
extern "C" esp_err_t nvs_set_blob(nvs_handle_t id, const char* key, const void* value, size_t size) {
    auto* handle = find(id); if (handle == nullptr) return ESP_ERR_NVS_INVALID_HANDLE;
    handle->lastKey = handle->prefix + key;
    if (handle->lastKey == failSet) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    const auto* bytes = static_cast<const uint8_t*>(value);
    values[handle->lastKey].working.assign(bytes, bytes + size); ++writes; return ESP_OK;
}
extern "C" esp_err_t nvs_erase_key(nvs_handle_t id, const char* key) {
    auto* handle = find(id); if (handle == nullptr) return ESP_ERR_NVS_INVALID_HANDLE;
    handle->lastKey = handle->prefix + key;
    if (handle->lastKey == failErase) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    auto& bytes = values[handle->lastKey].working;
    if (bytes.empty()) return ESP_ERR_NVS_NOT_FOUND;
    bytes.clear(); ++erases; return ESP_OK;
}
extern "C" esp_err_t nvs_commit(nvs_handle_t id) {
    auto* handle = find(id); if (handle == nullptr) return ESP_ERR_NVS_INVALID_HANDLE;
    if (handle->lastKey == failCommit) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    auto& value = values[handle->lastKey]; value.durable = value.working; return ESP_OK;
}

int main() {
    testLegacyAndIndependentBindings(); testPartialResetAndLifecycle();
    std::cout << "ESP32 gateway NVS bank: legacy names, two bindings, failure/restart/reset isolation passed\n";
}

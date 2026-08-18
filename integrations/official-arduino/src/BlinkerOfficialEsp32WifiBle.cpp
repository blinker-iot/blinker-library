#include "BlinkerOfficialEsp32WifiBle.h"
#include "BlinkerOfficialEsp32BlePlatform.h"
#include "BlinkerOfficialEsp32WifiPlatform.h"
#include "BlinkerOfficialWifiBleComposition.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

class Esp32WifiBlePlatform : public Esp32WifiPlatform {
public:
    enum : size_t {
        maximumBlePacketSize = Esp32NimBleRadio::maximumPacketSize
    };

    Esp32WifiBlePlatform()
        : Esp32WifiPlatform(),
          controllerBlob_(esp32BlobStorageConfig(
              "bl_v2_ctl",
              "table",
              ControllerCredentialStore::serializedSize)),
          setupSagaBlob_(esp32BlobStorageConfig(
              "bl_v2_setup",
              "saga",
              LocalSetupSagaStore::serializedSize)),
          bleRadio_() {}

    Result begin() {
        Result result = Esp32WifiPlatform::begin();
        if (result) result = controllerBlob_.begin();
        if (result) result = setupSagaBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        bleRadio_.stop();
        setupSagaBlob_.end();
        controllerBlob_.end();
        Esp32WifiPlatform::end();
    }

    IAtomicBlobStore& controllerCredentialBlob() {
        return controllerBlob_;
    }
    IAtomicBlobStore& localSetupSagaBlob() { return setupSagaBlob_; }
    Esp32NimBleLink& bleLink() { return bleRadio_.link(); }

private:
    Esp32NvsAtomicBlobStore controllerBlob_;
    Esp32NvsAtomicBlobStore setupSagaBlob_;
    Esp32NimBleRadio bleRadio_;
};

typedef WifiBleComposition<Esp32WifiBlePlatform>
    Esp32WifiBleComposition;

} // namespace

IProductLifecycle& esp32WifiBleLifecycle() {
    static Esp32WifiBleComposition instance;
    return instance;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker

#include "BlinkerOfficialRenesasUnoWifiBle.h"
#include "BlinkerOfficialRenesasUnoBlePlatform.h"
#include "BlinkerOfficialRenesasUnoWifiPlatform.h"
#include "BlinkerOfficialWifiBleComposition.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

class RenesasUnoWifiBlePlatform : public RenesasUnoWifiPlatform {
public:
    enum : size_t {
        maximumBlePacketSize = RenesasUnoBleRadio::maximumPacketSize
    };

    RenesasUnoWifiBlePlatform()
        : RenesasUnoWifiPlatform(), bleRadio_() {}

    void end() {
        bleRadio_.stop();
        RenesasUnoWifiPlatform::end();
    }

    ArduinoBleLink& bleLink() { return bleRadio_.link(); }

private:
    RenesasUnoBleRadio bleRadio_;
};

typedef WifiBleComposition<RenesasUnoWifiBlePlatform>
    RenesasUnoWifiBleComposition;

} // namespace

IProductLifecycle& renesasUnoWifiBleLifecycle() {
    static RenesasUnoWifiBleComposition instance;
    return instance;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker

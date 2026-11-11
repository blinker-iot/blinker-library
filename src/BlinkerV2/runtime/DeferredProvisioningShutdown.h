#ifndef BLINKER_RUNTIME_DEFERREDPROVISIONINGSHUTDOWN_H
#define BLINKER_RUNTIME_DEFERREDPROVISIONINGSHUTDOWN_H

namespace blinker {

// Internal native-owner teardown only. A failed bounded end() request does not
// authorize closing Store or starting tasks. Later owner polls only observe
// actual deinit; they do not repeat the potentially blocking stop request.
class DeferredProvisioningShutdown {
public:
    DeferredProvisioningShutdown() : requested_(false) {}
    void reset() { requested_ = false; }
    template <typename Provisioner>
    bool poll(Provisioner& provisioner) {
        if (!requested_) {
            requested_ = true;
            provisioner.end();
        } else if (provisioner.shutdownComplete()) {
            provisioner.end();
        }
        return provisioner.shutdownComplete();
    }
private:
    bool requested_;
};

} // namespace blinker
#endif

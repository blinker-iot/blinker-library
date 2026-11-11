#include <WiFiS3.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemClient.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4WiFi.h>
#include <BlinkerV2/arduino/ports/arduino/ArduinoClientHttp.h>
#include <BlinkerV2/arduino/ports/arduino/PubSubMqtt.h>
#include "modem_seam.h"
#include <cstdlib>
#include <iostream>

namespace blinker {
namespace {
uint32_t now() { return cleanup_seam::now; }
void timeout(uint32_t value) { modem.timeout(value); }
}
RenesasUnoModemOwner& renesasUnoModemOwner() {
    static RenesasUnoModemOwner owner(&now, &timeout, 10000U);
    return owner;
}
}

#define CHECK(condition) do { if (!(condition)) { std::cerr << __LINE__ << ": " << #condition << '\n'; return 1; } } while (false)

template <typename Base>
class ObservedClient : public Base {
public:
    int socket() const { return this->_sock; }
    int buffered() const { return this->rx_buffer->available(); }
};
typedef ObservedClient<blinker::RenesasUnoModemClient<WiFiClient>> Guarded;
class Clock : public blinker::IClock {
public:
    uint32_t monotonicMillis() const override { return cleanup_seam::now; }
    bool unixTime(uint64_t&) const override { return false; }
};

template <typename T>
bool connectedAndBuffered(T& client) {
    // Exercise actual SDK socket assignment and RX fill instead of assigning
    // private state or starting from an already invalid socket/empty buffer.
    uint8_t first = 0U;
    return client.connect("192.0.2.1", 1883U) == 1 && client.socket() == 7 &&
        client.read(&first, 1U) == 1 && first == 'A' && client.buffered() == 1;
}
bool fault() {
    auto& owner = blinker::renesasUnoModemOwner();
    blinker::RenesasUnoModemRpcBoundary boundary(owner, 1000U);
    if (!boundary.begin(1000U)) return false;
    boundary.fault();
    return owner.quarantined() && !owner.networkAllowed() && !boundary.active();
}
bool commandSince(size_t start, const char* expected) {
    for (size_t i = start; i < cleanup_seam::calls.commands.size(); ++i)
        if (cleanup_seam::calls.commands[i] == expected) return true;
    return false;
}
blinker::WifiNetworkConfig network() {
    blinker::WifiNetworkConfig config;
    static const uint8_t ssid[] = "host-test-open";
    config.ssid = blinker::ByteView(ssid, sizeof(ssid) - 1U);
    config.authentication = blinker::WifiAuthentication::Open;
    return config;
}

int main(int argc, char** argv) {
    CHECK(argc == 2);
    const std::string name(argv[1]);
    if (name == "ipv4-format") {
        // The unchanged SDK overload uses IPAddress::toString(). Make that
        // conversion unavailable in this seam; do not simulate real SDK OOM.
        IPAddress::failIpv4ToString() = true;
        {
            WiFiClient native;
            Client& api = native;
            IPAddress::resetObservations();
            CHECK(api.connect(IPAddress(192U, 0U, 2U, 9U), 1883U) == 1);
            CHECK(IPAddress::toStringCalls() == 1U);
            CHECK(commandSince(0U, "AT+CLIENTCONNECTNAME=7,,1883\r\n"));
        }
        const uint8_t octets[][4] = {{0U, 0U, 0U, 0U}, {255U, 255U, 255U, 255U}, {192U, 0U, 2U, 9U}};
        const char* expected[] = {"AT+CLIENTCONNECTNAME=7,0.0.0.0,1\r\n",
            "AT+CLIENTCONNECTNAME=7,255.255.255.255,65535\r\n",
            "AT+CLIENTCONNECTNAME=7,192.0.2.9,1883\r\n"};
        const uint16_t ports[] = {1U, 65535U, 1883U};
        for (size_t i = 0U; i < 3U; ++i) {
            Guarded client; Client& api = client;
            IPAddress::resetObservations();
            const size_t start = cleanup_seam::calls.commands.size();
            CHECK(api.connect(IPAddress(octets[i][0], octets[i][1], octets[i][2], octets[i][3]), ports[i]) == 1);
            CHECK(commandSince(start, expected[i]));
            CHECK(IPAddress::toStringCalls() == 0U && IPAddress::typeCalls() == 1U && IPAddress::indexCalls() == 4U);
            CHECK(client.socket() == 7);
        }
        // Hostname overload remains the same real SDK route.
        Guarded hostname; Client& api = hostname;
        IPAddress::resetObservations();
        const size_t start = cleanup_seam::calls.commands.size();
        CHECK(api.connect("example.invalid", 8883U) == 1);
        CHECK(commandSince(start, "AT+CLIENTCONNECTNAME=7,example.invalid,8883\r\n"));
        CHECK(IPAddress::typeCalls() == 0U && IPAddress::indexCalls() == 0U && IPAddress::toStringCalls() == 0U);
        CHECK(blinker::renesasUnoModemOwner().networkAllowed());
    } else if (name == "ipv6-route") {
        // IPv6 representation itself is a compatibility seam. This proves
        // only that the production wrapper retains the original IP overload.
        Guarded client; Client& api = client;
        IPAddress::resetObservations();
        CHECK(api.connect(IPAddress(arduino::IPv6), 1884U) == 1);
        CHECK(IPAddress::typeCalls() == 1U && IPAddress::indexCalls() == 0U && IPAddress::toStringCalls() == 1U);
        CHECK(commandSince(0U, "AT+CLIENTCONNECTNAME=7,2001:db8::1,1884\r\n"));
        CHECK(blinker::renesasUnoModemOwner().networkAllowed());
    } else if (name == "ipv4-busy" || name == "ipv4-fault") {
        Guarded client; Client& api = client;
        auto& owner = blinker::renesasUnoModemOwner();
        int token = 0;
        CHECK(owner.claimNamespace(&token));
        {
            blinker::RenesasUnoModemOwner::Operation operation(owner, &token, 1000U);
            CHECK(operation.result());
            if (name == "ipv4-fault") CHECK(fault());
            const unsigned before = cleanup_seam::calls.total();
            IPAddress::resetObservations();
            CHECK(api.connect(IPAddress(255U, 255U, 255U, 255U), 65535U) == 0);
            CHECK(IPAddress::typeCalls() == 0U && IPAddress::indexCalls() == 0U && IPAddress::toStringCalls() == 0U);
            CHECK(cleanup_seam::calls.total() == before && client.socket() == -1);
        }
        CHECK(owner.quarantined() == (name == "ipv4-fault"));
        CHECK(owner.networkAllowed() == (name == "ipv4-busy"));
        owner.releaseNamespace(&token);
    } else if (name == "native-positive") {
        unsigned beforeDestruction;
        {
            ObservedClient<WiFiClient> native;
            CWifi nativeStation;
            CHECK(connectedAndBuffered(native));
            const size_t start = cleanup_seam::calls.commands.size();
            native.flush(); native.stop();
            CHECK(commandSince(start, "AT+FLUSH=7\r\n"));
            CHECK(commandSince(start, "AT+CLIENTCLOSE=7\r\n"));
            CHECK(native.socket() == -1 && native.buffered() == 0);
            nativeStation.setTimeout(0U);
            (void)nativeStation.begin("native-positive-open");
            CHECK(nativeStation.disconnect() == 1); nativeStation.end();
            CHECK(commandSince(start, "AT+DISCONNECT\r\n"));
            CHECK(commandSince(start, "AT+SOFTRESETWIFI\r\n"));
            beforeDestruction = cleanup_seam::calls.total();
        }
        CHECK(cleanup_seam::calls.total() == beforeDestruction);
    } else if (name == "client-fault") {
        Guarded client; CHECK(connectedAndBuffered(client));
        CHECK(client.WiFiClient::operator bool());
        CHECK(fault());
        const unsigned before = cleanup_seam::calls.total();
        Client& api = client;
        uint8_t value = 99U;
        api.flush(); api.stop(); api.stop();
        CHECK(api.connect("192.0.2.1", 1883U) == 0);
        CHECK(api.connect(IPAddress(192U, 0U, 2U, 1U), 1883U) == 0);
        CHECK(api.write(value) == 0U && api.write(&value, 1U) == 0U);
        CHECK(api.available() == 0 && api.read() == -1 && api.read(&value, 1U) == -1);
        CHECK(value == 99U && api.peek() == -1 && api.connected() == 0U && !api);
        CHECK(client.remoteIP() == IPAddress() && client.remotePort() == 0U);
        CHECK(client.socket() == 7 && client.buffered() == 1);
        CHECK(cleanup_seam::calls.total() == before);
    } else if (name == "station-connecting-fault" || name == "station-connected-fault") {
        blinker::RenesasUnoWifiStation station;
        CHECK(station.start(network()).ok());
        CHECK(station.state() == blinker::WifiStationState::Connecting);
        if (name == "station-connected-fault") {
            station.poll();
            CHECK(station.state() == blinker::WifiStationState::Connected);
        }
        CHECK(cleanup_seam::calls.write >= 2U); // Real CWifi begin was reached.
        CHECK(fault());
        const unsigned before = cleanup_seam::calls.total();
        CHECK(station.state() == (name == "station-connected-fault"
            ? blinker::WifiStationState::Connected : blinker::WifiStationState::Connecting));
        // Direct cleanup from the active pre-fault state, not after poll has
        // already changed it to Failed. No request is made to reset the modem.
        station.stop(); station.stop();
        CHECK(station.state() == blinker::WifiStationState::Stopped &&
              station.lastError() == blinker::ErrorCode::Ok);
        CHECK(blinker::renesasUnoModemOwner().quarantined() &&
              !blinker::renesasUnoModemOwner().networkAllowed());
        station.poll(); // Stopped polling remains local; no active-poll claim.
        CHECK(station.start(network()).code() == blinker::ErrorCode::StateConflict);
        CHECK(cleanup_seam::calls.total() == before);
    } else if (name == "http-stop-fault" || name == "http-fail-fault") {
        Guarded client; CHECK(connectedAndBuffered(client));
        Clock clock;
        blinker::ArduinoClientHttpAdapter<Guarded> http(client, blinker::HttpSecurity::PlainTcp, clock);
        CHECK(fault());
        const unsigned before = cleanup_seam::calls.total();
        if (name == "http-stop-fault") {
            http.stop(); http.stop();
            CHECK(http.state() == blinker::HttpClientState::Idle);
        } else {
            // start first calls stop, then invalid host reaches actual fail(),
            // which calls stop again. Non-empty native state is retained.
            blinker::HttpRequest invalid;
            invalid.security = blinker::HttpSecurity::PlainTcp;
            CHECK(!http.start(invalid));
            CHECK(http.state() == blinker::HttpClientState::Failed &&
                  http.lastError() == blinker::ErrorCode::InvalidArgument);
        }
        CHECK(client.socket() == 7 && client.buffered() == 1);
        CHECK(cleanup_seam::calls.total() == before);
    } else if (name == "mqtt-disconnect-fault") {
        Guarded client; CHECK(connectedAndBuffered(client));
        PubSubClient native(client);
        blinker::PubSubClientAdapter<48U> adapter(native, blinker::MqttSecurity::PlainTcp, 256U);
        CHECK(fault());
        const unsigned before = cleanup_seam::calls.total();
        adapter.disconnect(); adapter.disconnect();
        CHECK(native.state() == MQTT_DISCONNECTED);
        CHECK(client.socket() == 7 && client.buffered() == 1);
        CHECK(cleanup_seam::calls.total() == before);
    } else if (name == "client-destruction-fault") {
        unsigned beforeDestruction;
        {
            Guarded client; CHECK(connectedAndBuffered(client)); CHECK(fault());
            beforeDestruction = cleanup_seam::calls.total();
        }
        CHECK(cleanup_seam::calls.total() == beforeDestruction);
    } else CHECK(false);
    if (name != "native-positive" && name != "ipv4-format" && name != "ipv6-route" && name != "ipv4-busy") {
        CHECK(blinker::renesasUnoModemOwner().quarantined());
        CHECK(!blinker::renesasUnoModemOwner().networkAllowed());
    }
    std::cout << name << ": passed; modem calls=" << cleanup_seam::calls.total()
              << "; original SDK bodies, permissive Modem seam; no UART/hardware evidence\n";
    return 0;
}

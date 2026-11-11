// Compile the complete generated original FSM, not a second parser model.
#include "Modem.h"
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemResponseBounds.h>
#include <BlinkerV2/arduino/ports/uno_r4/UnoR4ModemRpcBoundary.h>

#include <iostream>

UART Serial2;
uint32_t fakeNow = 0U, fakeWriteCost = 0U, fakeReadCost = 0U, fakePollCost = 0U;
size_t fakeWriteLimit = static_cast<size_t>(-1);
bool fakePollFault = false;
unsigned long millis() { return fakeNow; }
namespace blinker {
namespace {
uint32_t now() { return fakeNow; }
void timeout(uint32_t value) { modem.timeout(value); }
}
RenesasUnoModemOwner& renesasUnoModemOwner() {
    static RenesasUnoModemOwner owner(&now, &timeout, 10000U);
    return owner;
}
}
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; return 1; } } while (0)

int main(int argc, char** argv) {
    CHECK(argc == 2);
    const std::string name(argv[1]);
    modem.noDebug(); // Original uninitialized per-instance debug is outside scope.
    modem.timeout(10000U);
    unsigned int parsed = 99U;
    CHECK(!blinker::unoR4ModemSizedLength(nullptr, 1U, 1024U, parsed) && parsed == 0U);
    CHECK(!blinker::unoR4ModemSizedLength("", 0U, 1024U, parsed));
    CHECK(blinker::unoR4ModemSizedLength("0", 1U, 0U, parsed) && parsed == 0U);
    CHECK(blinker::unoR4ModemSizedLength("4294967295", 10U, UINT_MAX, parsed) && parsed == UINT_MAX);
    CHECK(!blinker::unoR4ModemSizedLength("4294967296", 10U, UINT_MAX, parsed) && parsed == 0U);
    CHECK(!blinker::unoR4ModemAppendAllowed(0U, 0U));

    if (name == "sized-prefix-boundaries") {
        CHECK(blinker::unoR4ModemSizedLength("  0", 3U, 0U, parsed) && parsed == 0U);
        CHECK(blinker::unoR4ModemSizedLength(" 1024", 5U, 1024U, parsed) && parsed == 1024U);
        const char* invalid[] = {" ", "  ", "\t8", " +8", " -8", " 8 ", " 8x", " 1025"};
        for (const char* value : invalid) {
            parsed = 99U;
            CHECK(!blinker::unoR4ModemSizedLength(value, std::strlen(value), 1024U, parsed));
            CHECK(parsed == 0U);
        }
        CHECK(Serial2.writes == 0U && Serial2.reads == 0U);
        std::cout << name << ": only leading ASCII spaces precede bounded digits\n";
        return 0;
    }

    // Actual generated Modem code uses the same production owner and boundary.
    if (name == "zero-budget") {
        modem.timeout(0U);
        std::string untouched = "untouched";
        CHECK(!modem.write("+T:", untouched, "%s", "AT+T?\r\n"));
        CHECK(untouched == "untouched" && Serial2.writes == 0U);
        CHECK(!blinker_r4_rpc_faulted() && blinker::renesasUnoModemOwner().networkAllowed());
        std::cout << name << ": no TX and no fabricated uncertainty\n";
        return 0;
    }
    if (name == "owner-deadline") {
        int token = 0;
        auto& owner = blinker::renesasUnoModemOwner();
        CHECK(owner.claimNamespace(&token));
        blinker::RenesasUnoModemOwner::Operation operation(owner, &token, 10U);
        CHECK(operation.result());
        fakeNow = 7U;
        CHECK(owner.prepare());
        fakeWriteCost = 3U;
        std::string response;
        Serial2.input = "+T:late\r\nOK\r\n";
        CHECK(!modem.write("+T:", response, "%s", "AT+T?\r\n"));
        CHECK(owner.quarantined() && owner.outcome() == blinker::RenesasUnoModemOwner::ReadUncertain);
        CHECK(Serial2.writes == 1U && Serial2.reads == 0U);
        std::cout << name << ": remaining storage budget includes TX\n";
        return 0;
    }

    std::string prompt = "+T:", command = "AT+T?\r\n", expected;
    bool success = false, headerPayload = false;
    unsigned expectedWrites = 1U;
    if (name == "normal") {
        Serial2.input = "+T: a b c\r\nOK\r\n"; expected = "abc"; success = true;
    } else if (name == "binary-exact") {
        expected.assign(1024U, 'x'); expected[11] = '\0';
        Serial2.input = "+T:1024|" + expected + "OK\r\n"; success = true;
    } else if (name == "sized-zero") {
        Serial2.input = "+T:0|OK\r\n"; success = true;
    } else if (name == "command-exact") {
        prompt = "+" + std::string(30U, 'T') + ":";
        Serial2.input = prompt + "abc\r\nOK\r\n"; expected = "abc"; success = true;
    } else if (name == "long-command") {
        Serial2.input = "+" + std::string(32U, 'T') + ":abc\r\nOK\r\n";
    } else if (name == "unterminated-data") {
        Serial2.input = "+T:" + std::string(1025U, 'x');
    } else if (name == "untrimmed-exact") {
        modem.avoid_trim_results();
        expected.assign(1024U, ' '); expected[11] = 'x';
        Serial2.input = "+T:" + expected + "\r\nOK\r\n"; success = true;
    } else if (name == "untrimmed-over-cap") {
        modem.avoid_trim_results();
        Serial2.input = "+T:" + std::string(1025U, ' ');
    } else if (name == "untrimmed-sized-no-space" || name == "untrimmed-sized-leading-space") {
        // Preferences uses untrimmed sized responses. Only the length prefix is
        // grammar: spaces, NUL and '|' inside the binary payload are all data.
        modem.avoid_trim_results();
        prompt = "+PREFGET:";
        command = "AT+PREFGET=\"fixture\"\r\n";
        const char payload[] = {' ', '\0', '|', '4', ' ', 'x', '\0', ' '};
        expected.assign(payload, sizeof(payload));
        Serial2.input = prompt + (name == "untrimmed-sized-leading-space" ? " 8|" : "8|")
            + expected + "OK\r\n";
        success = true;
    } else if (name == "header-payload-normal") {
        headerPayload = true; success = true; Serial2.input = "OK\r\n";
    } else if (name == "header-payload-over-cap") {
        headerPayload = true; Serial2.input = "+T:" + std::string(1025U, 'x');
    } else if (name == "sized-negative") {
        Serial2.input = "+T:-1|xOK\r\n";
    } else if (name == "sized-overflow") {
        Serial2.input = "+T:4294967296|xOK\r\n";
    } else if (name == "sized-over-cap") {
        Serial2.input = "+T:1025|xOK\r\n";
    } else if (name == "sized-nondigit") {
        Serial2.input = "+T:1x|xOK\r\n";
    } else if (name == "wrong-command-data") {
        Serial2.input = "+OTHER:" + std::string(1025U, 'x') + "\r\nOK\r\n+T:abc\r\nOK\r\n";
    } else if (name == "tx-deadline") {
        modem.timeout(10U); fakeWriteCost = 10U; Serial2.input = "+T:late\r\nOK\r\n";
    } else if (name == "rx-deadline") {
        modem.timeout(5U); fakePollCost = 1U;
    } else if (name == "rx-owner-fault") {
        fakePollFault = true;
    } else if (name == "final-byte-deadline") {
        modem.timeout(4U); fakeReadCost = 1U; prompt = DO_NOT_CHECK_CMD;
        Serial2.input = "OK\r\n";
    } else if (name == "short-write") {
        fakeWriteLimit = 3U; Serial2.input = "+T:late\r\nOK\r\n";
    } else if (name == "header-payload-deadline" || name == "header-rpc-interleave") {
        modem.timeout(10U); fakeWriteCost = 6U;
        std::string ignored;
        modem.write_nowait(prompt, ignored, "%s", "AT+T=3\r\n");
        CHECK(blinker_r4_rpc_active() && !blinker::renesasUnoModemOwner().networkAllowed());
        if (name == "header-payload-deadline") {
            fakeNow += 5U;
            CHECK(!modem.passthrough(reinterpret_cast<const uint8_t*>("abc"), 3U));
        } else {
            CHECK(!modem.write(prompt, ignored, "%s", "AT+T?\r\n"));
        }
        // Existing generic assertion below must not issue a second command.
        CHECK(blinker_r4_rpc_faulted() && Serial2.writes == 1U);
        Serial2.input = "+T:late\r\nOK\r\n";
        CHECK(!modem.write(prompt, ignored, "%s", "AT+T?\r\n"));
        CHECK(Serial2.writes == 1U && Serial2.reads == 0U);
        std::cout << name << ": pending header never receives a renewed budget\n";
        return 0;
    } else if (name == "format-overflow") {
        std::string response;
        const std::string oversized(128U, 'x');
        CHECK(!modem.write(prompt, response, "%s", oversized.c_str()));
        CHECK(Serial2.writes == 0U && blinker_r4_rpc_faulted());
        std::cout << name << ": truncated command is not transmitted\n";
        return 0;
    } else {
        CHECK(false);
    }
    std::string response;
    if (headerPayload) {
        const uint8_t payload[3] = {'x', 0U, 'y'};
        modem.write_nowait(prompt, response, "%s", "AT+T=3\r\n");
        CHECK(modem.passthrough(payload, sizeof(payload)) == success);
        CHECK(Serial2.writes == 2U);
        CHECK(Serial2.output == std::string("AT+T=3\r\n") + std::string("x\0y", 3U));
    } else {
        CHECK(modem.write(prompt, response, "%s", command.c_str()) == success);
        CHECK(Serial2.writes == expectedWrites);
    }
    if (success) {
        CHECK(response == expected);
        Serial2.input = "+T:done\r\nOK\r\n";
        CHECK(modem.write("+T:", response, "%s", "AT+T?\r\n") && response == "done");
    } else {
        const unsigned reads = Serial2.reads, writes = Serial2.writes, starts = Serial2.starts;
        const unsigned flushes = Serial2.flushes, checks = Serial2.checks;
        Serial2.input = "+T:late\r\nOK\r\n";
        response = "untouched";
        modem.begin();
        CHECK(!modem.write("+T:", response, "%s", "AT+T?\r\n"));
        CHECK(response == "untouched");
        modem.write_nowait("+T:", response, "%s", "AT+T?\r\n");
        CHECK(!modem.passthrough(reinterpret_cast<const uint8_t*>("x"), 1U));
        CHECK(Serial2.reads == reads && Serial2.writes == writes && Serial2.starts == starts);
        CHECK(Serial2.flushes == flushes && Serial2.checks == checks);
    }
    std::cout << name << ": original Modem FSM finite response guard passed\n";
    return 0;
}

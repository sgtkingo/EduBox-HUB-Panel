"""Native tests for Panel heartbeat scheduling, six probes, and reconnect latch."""
from pathlib import Path
import subprocess
import tempfile
import unittest

SRC = Path(__file__).resolve().parents[2] / "vscp/src"


class PingWatchdogTest(unittest.TestCase):
    def test_schedule_loss_and_recovery(self):
        with tempfile.TemporaryDirectory(prefix="edubox-ping-watchdog-") as directory:
            root = Path(directory)
            source = root / "main.cpp"
            source.write_text(r'''
#include "runtime_protocol_session.hpp"
#include <cassert>
#include <deque>
static uint32_t now = 0;
static uint32_t clockMs() { return now; }
class Transport : public vscp::Transport {
public:
    std::deque<vscp::String> incoming;
    bool answerPing = true, answerConnect = true, answerInit = true;
    int pings = 0, writes = 0;
protected:
    bool writeLineImpl(const vscp::String& message) override {
        ++writes;
        vscp::Request request;
        vscp::String error;
        assert(vscp::Codec::parseRequest(message, request, error));
        if (request.command == vscp::Command::Ping) {
            ++pings;
            if (answerPing) incoming.push_back("?side=server&status=1&seq=" + request.value("seq"));
        } else if (request.command == vscp::Command::Init) {
            incoming.push_back(answerInit ? "?status=1" : "?status=0");
        } else if (request.command == vscp::Command::Connect) {
            incoming.push_back(answerConnect ? "?id=S01&status=1" : "?id=S01&status=0");
        } else incoming.push_back("?id=S01&status=1");
        return true;
    }
    vscp::ReadStatus readLineImpl(vscp::String& message) override {
        if (incoming.empty()) return vscp::ReadStatus::NoData;
        message = incoming.front(); incoming.pop_front();
        return vscp::ReadStatus::Message;
    }
};
int main() {
    Transport wire;
    vscp::Client protocol(wire, 50);
    RuntimeProtocolSession session(protocol, clockMs);
    now = 10000; session.serviceLink(true); assert(wire.pings == 0);
    wire.answerInit = false;
    session.init(); now += 3000; session.serviceLink(true); assert(wire.pings == 0);
    wire.answerInit = true; session.init();
    now += 3000; session.serviceLink(false); assert(wire.pings == 0);
    assert(session.consecutivePingFailures() == 0 && !session.connectionLost());
    now += 30000; session.serviceLink(false); assert(wire.pings == 0); // Online Run, no auto-ping.
    now += 2999; session.serviceLink(true); assert(wire.pings == 0);
    ++now; session.serviceLink(true); assert(wire.pings == 1); // Active immediately after INIT, no CONNECT.
    now += 3000; session.serviceLink(true); assert(wire.pings == 2);
    assert(session.consecutivePingFailures() == 0);
    wire.answerPing = false;
    now += 3000; session.serviceLink(true); assert(session.consecutivePingFailures() == 1);
    now += 99; session.serviceLink(true); assert(wire.pings == 3);
    ++now; session.serviceLink(true); assert(wire.pings == 4);
    // A valid runtime response doesn't conceal a failing heartbeat.
    session.update("S01"); assert(session.consecutivePingFailures() == 2);
    wire.answerPing = true;
    now += 100; session.serviceLink(true); assert(session.consecutivePingFailures() == 0);
    now += 2999; session.serviceLink(true); assert(wire.pings == 5);
    wire.answerPing = false;
    ++now; session.serviceLink(true); assert(session.consecutivePingFailures() == 1);
    for (int retry = 1; retry <= 5; ++retry) {
        now += 100; session.serviceLink(true);
        assert(session.connectionLost() == (retry == 5));
    }
    assert(session.consecutivePingFailures() == 6);
    const int writes = wire.writes;
    session.serviceLink(false); assert(session.connectionLost());
    now += 3000; session.serviceLink(true);
    session.update("S01"); session.control("S01", {}); session.config("S01", {});
    assert(wire.writes == writes); // No polling or automatic recovery after DISCONNECT.
    wire.answerPing = true;
    session.invalidateInitialization(); session.init();
    assert(session.connectionLost()); // INIT alone doesn't restore the device session.
    wire.answerConnect = false; session.connect("S01", "1"); assert(session.connectionLost());
    wire.answerConnect = true; session.connect("S01", "1"); assert(!session.connectionLost());
    assert(session.consecutivePingFailures() == 0);
    now += 3000; session.serviceLink(true); assert(!session.connectionLost());
    // Reconnect on Select device needs INIT only, then explicit link recovery.
    session.stopCommunication(); session.invalidateInitialization();
    assert(!session.completeLinkReconnect());
    session.init(); assert(session.connectionLost());
    assert(session.completeLinkReconnect() && !session.connectionLost());
    assert(session.consecutiveFailures() == 0 && session.consecutivePingFailures() == 0);
    now += 3000; session.serviceLink(true); assert(!session.connectionLost());
    // Unsigned scheduling survives the 32-bit millis rollover.
    now = UINT32_MAX - 1000; session.init();
    const int pings = wire.pings;
    now += 2999; session.serviceLink(true); assert(wire.pings == pings);
    ++now; session.serviceLink(true); assert(wire.pings == pings + 1);
    // Run suspends an in-progress retry sequence; Pause starts a fresh interval.
    wire.answerPing = false;
    now += 3000; session.serviceLink(true); assert(session.consecutivePingFailures() == 1);
    const int beforeRun = wire.pings;
    now += 100; session.serviceLink(false);
    assert(wire.pings == beforeRun && session.consecutivePingFailures() == 0);
    now += 30000; session.serviceLink(false); assert(wire.pings == beforeRun);
    now += 2999; session.serviceLink(true); assert(wire.pings == beforeRun);
    ++now; session.serviceLink(true);
    assert(wire.pings == beforeRun + 1 && session.consecutivePingFailures() == 1);
}
''', encoding="utf-8")
            executable = root / "ping-watchdog.exe"
            subprocess.run([
                "g++", "-std=c++17", "-DSTDIO_H_ENV", "-I", str(SRC),
                "-I", str(SRC.parents[1] / "engine/src"), str(source),
                *(str(SRC / name) for name in (
                    "vscp_client.cpp", "vscp_codec.cpp", "vscp_types.cpp", "io/vscp_transport.cpp")),
                "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()

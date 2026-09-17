"""Five consecutive protocol failures latch runtime traffic until CONNECT succeeds."""
from pathlib import Path
import subprocess
import tempfile
import unittest

SRC = Path(__file__).resolve().parents[2] / "vscp/src"

class ConnectionWatchdogTest(unittest.TestCase):
    def test_failure_streak_stop_and_recovery(self):
        with tempfile.TemporaryDirectory(prefix="edubox-watchdog-") as directory:
            root = Path(directory)
            source = root / "main.cpp"
            source.write_text(r"""
#include "vscp.hpp"
#include "runtime_protocol_session.hpp"
#include <cassert>
class Transport : public vscp::Transport {
public:
    vscp::String response = "?status=1";
    vscp::ReadStatus readStatus = vscp::ReadStatus::Message;
    int writes = 0;
protected:
    bool writeLineImpl(const vscp::String&) override { ++writes; return true; }
    vscp::ReadStatus readLineImpl(vscp::String& message) override {
        message = response;
        return readStatus;
    }
};
int main() {
    Transport transport;
    vscp::Client protocol(transport, 2);
    RuntimeProtocolSession client(protocol);
    assert(client.init().status == vscp::Status::Ok);
    transport.response = "?id=S01&status=1";
    assert(client.connect("S01", "1").status == vscp::Status::Ok);
    transport.response = "?id=S01&status=0&error=Device not connected";
    for (int i = 0; i < 4; ++i) client.update("S01");
    assert(client.consecutiveFailures() == 4 && !client.connectionLost());
    transport.response = "?id=S01&status=1";
    client.control("S01", {});
    assert(client.consecutiveFailures() == 0);
    transport.readStatus = vscp::ReadStatus::NoData;
    assert(client.update("S01").error == "Response timeout");
    transport.readStatus = vscp::ReadStatus::Message;
    transport.response = "garbage";
    client.config("S01", {});
    transport.response = "?status=0&error=Protocol not initialized";
    client.control("S01", {});
    transport.response = "?id=other&status=1";
    client.update("S01");
    assert(client.consecutiveFailures() == 4 && !client.connectionLost());
    transport.readStatus = vscp::ReadStatus::MessageTooLong;
    const auto last = client.config("S01", {});
    assert(last.status == vscp::Status::Error);
    assert(last.error.find("DISCONNECT") == 0);
    assert(client.connectionLost() && client.consecutiveFailures() == 5);
    const int writes = transport.writes;
    client.update("S01"); client.config("S01", {}); client.control("S01", {});
    assert(transport.writes == writes && client.consecutiveFailures() == 5);
    transport.readStatus = vscp::ReadStatus::Message;
    transport.response = "?status=1";
    assert(client.init().status == vscp::Status::Ok);
    assert(client.connectionLost()); // INIT alone does not restore device session.
    transport.response = "?id=S01&status=0";
    client.connect("S01", "1");
    assert(client.connectionLost());
    transport.response = "?id=S01&status=1";
    client.connect("S01", "1");
    assert(!client.connectionLost() && client.consecutiveFailures() == 0);
    client.update("S01");
    assert(transport.writes > writes);
    // Reinitialization is a Panel policy, not a mutation of the shared client.
    client.invalidateInitialization();
    assert(protocol.isInitialized() && !client.isInitialized());
    transport.response = "?status=1";
    assert(client.init().status == vscp::Status::Ok);
    assert(client.isInitialized());
    // The shared client remains capable of transactions after any failure count.
    transport.response = "?id=S01&status=0";
    for (int i = 0; i < 6; ++i) protocol.update("S01");
    assert(client.consecutiveFailures() == 0 && !client.connectionLost());
}
""")
            executable = root / "watchdog.exe"
            subprocess.run(["g++", "-std=c++17", "-DSTDIO_H_ENV", "-I", str(SRC), "-I", str(SRC.parents[1] / "engine/src"),
                            str(source), str(SRC / "vscp_client.cpp"),
                            str(SRC / "vscp_codec.cpp"), str(SRC / "vscp_types.cpp"),
                            str(SRC / "io/vscp_transport.cpp"), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True, timeout=10)

if __name__ == "__main__":
    unittest.main()

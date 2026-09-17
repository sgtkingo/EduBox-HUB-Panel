"""Panel frame traces are exact and emitted only at debug level 3."""
from pathlib import Path
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[3]


class PanelVscpLoggingTest(unittest.TestCase):
    def test_trace_levels_and_invalid_frames(self):
        with tempfile.TemporaryDirectory(prefix="edubox-vscp-log-") as directory:
            root = Path(directory)
            (root / "expt.hpp").write_text('#pragma once\nenum { DEBUG_VERBOSE_ALL = 3 };\nvoid debugLogMessage(int, const char*, const char*, const char*, ...);\n')
            (root / "main.cpp").write_text(r'''
#include "vscp_panel_log_sink.hpp"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
std::string output;
void debugLogMessage(int, const char* source, const char* reason, const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    output += std::string("[DEBUG][") + source + "]: " + buffer + " reason=" + reason;
    output += '\n';
}
class TestTransport : public vscp::Transport {
public:
    using Transport::Transport;
    std::string incoming;
    std::string sent;
protected:
    vscp::ReadStatus readLineImpl(vscp::String& frame) override {
        frame = incoming;
        incoming.clear();
        return frame.empty() ? vscp::ReadStatus::NoData : vscp::ReadStatus::Message;
    }
    bool writeLineImpl(const vscp::String& frame) override { sent = frame; return true; }
};
int main() {
    PanelVscpLogSink sink;
    TestTransport transport(&sink);
    transport.writeLine("?type=INIT&api=1.4");
    assert(transport.sent == "?type=INIT&api=1.4");
    transport.incoming = "?api=1.4&status=1";
    std::string frame;
    assert(transport.readLine(frame) == vscp::ReadStatus::Message);
    assert(frame == "?api=1.4&status=1");
    transport.incoming = "DEBUG: unexpected line";
    assert(transport.readLine(frame) == vscp::ReadStatus::Message);
#if EXPECT_TRACE
    assert(output == "[DEBUG][VSCP]: [VSCP][TX] ?type=INIT&api=1.4 reason=transport\n"
                     "[DEBUG][VSCP]: [VSCP][RX] ?api=1.4&status=1 reason=transport\n"
                     "[DEBUG][VSCP]: [VSCP][RX] DEBUG: unexpected line reason=transport\n");
#else
    assert(output.empty());
#endif
}
''')
            for debug, level, extra, trace in [
                (0, 3, [], 0), (1, 1, [], 0), (1, 2, [], 0), (1, 3, [], 1),
                (1, 3, ["-DARDUINO=10819", "-DARDUINO_USB_CDC_ON_BOOT=0",
                         "-DEDUBOX_HUB_PANEL_VSCP_UART_PORT=0"], 1),
                (1, 3, ["-DARDUINO=10819", "-DARDUINO_USB_CDC_ON_BOOT=1",
                         "-DEDUBOX_HUB_PANEL_VSCP_UART_PORT=0"], 1),
                (1, 3, ["-DARDUINO=10819", "-DARDUINO_USB_CDC_ON_BOOT=0",
                         "-DEDUBOX_HUB_PANEL_VSCP_UART_PORT=2"], 1),
            ]:
                with self.subTest(debug=debug, level=level, flags=extra):
                    executable = root / "logging.exe"
                    subprocess.run([
                        "g++", "-std=c++17", "-DSTDIO_H_ENV", "-DPROTOCOL_VERBOSE=2",
                        f"-DENABLE_DEBUG={debug}", f"-DDEBUG_VERBOSE_LEVEL={level}",
                        f"-DEXPECT_TRACE={trace}", *extra,
                        "-I", str(root), "-I", str(REPO / "ui"),
                        "-I", str(REPO / "libraries/vscp/src"),
                        str(root / "main.cpp"),
                        str(REPO / "libraries/vscp/src/io/vscp_transport.cpp"),
                        "-o", str(executable),
                    ], check=True)
                    subprocess.run([str(executable)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()

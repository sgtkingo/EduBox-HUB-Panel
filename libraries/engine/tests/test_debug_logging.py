"""Debug is off by default; regular logging on the shared UART still works."""
from pathlib import Path
import subprocess
import tempfile
import unittest

LOGGER = Path(__file__).resolve().parents[2] / "expt/src/logs/logs.cpp"


class DebugLoggingTest(unittest.TestCase):
    def test_debug_switch_does_not_disable_regular_logs(self):
        with tempfile.TemporaryDirectory(prefix="edubox-debug-") as directory:
            root = Path(directory)
            (root / "Arduino.h").write_text(r'''
#pragma once
#include <string>
struct SerialMock {
    bool started = false;
    std::string output;
    explicit operator bool() const { return started; }
    void begin(unsigned) { started = true; }
    void setTimeout(unsigned) {}
    void println(const char* line) { output += line; output += '\n'; }
    void flush() {}
};
extern SerialMock Serial;
''')
            (root / "main.cpp").write_text(r'''
#include "Arduino.h"
#include "logs.hpp"
#include <cassert>
static_assert(ENABLE_DEBUG == EXPECT_DEBUG);
SerialMock Serial;
int main() {
    initLogger();
    assert(Serial.started);
    debugLogMessage(DEBUG_VERBOSE_ERRORS, "connect", "attempt", "INIT");
    debugLogMessage("connect", "attempt", "CONNECT");
#if EXPECT_DEBUG
    assert(Serial.output.find("DEBUG:") != std::string::npos);
#else
    assert(Serial.output.empty());
#endif
    logMessage("regular marker");
    assert(Serial.output.find("regular marker") != std::string::npos);
    setLoggerUsbCdcAvailable(false);
    const std::string beforeBuffering = Serial.output;
    logMessage("buffered marker");
    flushBufferedLogMessages();
    assert(Serial.output == beforeBuffering);
    setLoggerUsbCdcAvailable(true);
    flushBufferedLogMessages();
    assert(Serial.output.find("buffered marker") != std::string::npos);
}
''')
            for debug, flags in [(0, []), (1, ["-DENABLE_DEBUG=1"])]:
                with self.subTest(debug=debug):
                    executable = root / f"debug_{debug}.exe"
                    subprocess.run([
                        "g++", "-std=c++17", "-DARDUINO=10819", "-DESP32",
                        "-DARDUINO_USB_CDC_ON_BOOT=0", f"-DEXPECT_DEBUG={debug}",
                        *flags, "-I", str(root), "-I", str(LOGGER.parent),
                        str(LOGGER), str(root / "main.cpp"), "-o", str(executable),
                    ], check=True)
                    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()

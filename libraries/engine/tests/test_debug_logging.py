"""Debug levels control diagnostics and exceptions, preserving critical GUI errors."""
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
#include "exceptions.hpp"
#include <cassert>
static_assert(ENABLE_DEBUG == EXPECT_SWITCH);
SerialMock Serial;
int splashCount = 0;
void splashMessage(const char*, ...) { ++splashCount; }
int main() {
    initLogger();
    assert(Serial.started);
    Exception ex("BaseDevice::connect", "Response UID mismatch?\nnext", ErrorCode::CRITICAL_ERROR_CODE,
                 new Exception("inner", "detail"));
    ex.print();
    assert(splashCount == 1);
    debugLogMessage(DEBUG_VERBOSE_ERRORS, "connect", "attempt", "INIT");
    debugLogMessage("connect", "attempt", "CONNECT");
#if EXPECT_DEBUG
    assert(Serial.output.find("[DEBUG][connect]:") != std::string::npos);
#else
    assert(Serial.output.empty());
#endif
#if EXPECT_DEBUG
    assert(Serial.output.find("[DEBUG][BaseDevice::connect]: EXCEPTION: Response UID mismatch? next") != std::string::npos);
    assert(Serial.output.find("[DEBUG][inner]: EXCEPTION: detail") != std::string::npos);
    assert((Serial.output.find("reason=attempt") != std::string::npos));
    assert((Serial.output.find(": CONNECT") != std::string::npos) == (DEBUG_VERBOSE_LEVEL >= 3));
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
            (root / "lvgl.h").write_text("#pragma once\n#include <cstdint>\ntypedef struct lv_event_t lv_event_t;\n")
            for debug, level in [(0, 3), (1, 0), (1, 1), (1, 2), (1, 3)]:
                flags = [f"-DENABLE_DEBUG={debug}", f"-DDEBUG_VERBOSE_LEVEL={level}"]
                with self.subTest(debug=debug):
                    executable = root / f"debug_{debug}.exe"
                    subprocess.run([
                        "g++", "-std=c++17", "-DARDUINO=10819", "-DESP32",
                        "-DARDUINO_USB_CDC_ON_BOOT=0", f"-DEXPECT_DEBUG={int(debug and level > 0)}", f"-DEXPECT_SWITCH={debug}",
                        *flags, "-I", str(root), "-I", str(LOGGER.parent),
                        str(LOGGER), str(LOGGER.parent.parent / "exceptions/exceptions.cpp"),
                        "-I", str(LOGGER.parent.parent / "exceptions"), str(root / "main.cpp"), "-o", str(executable),
                    ], check=True)
                    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()

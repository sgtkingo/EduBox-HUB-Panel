"""Host regression checks for the real transfer service (requires g++)."""
from pathlib import Path
import subprocess
import tempfile
import unittest


MANAGERS = Path(__file__).resolve().parents[1] / "src" / "managers"


class TransferStartTest(unittest.TestCase):
    def test_start_and_rollback(self):
        with tempfile.TemporaryDirectory(prefix="edubox-transfer-") as directory:
            root = Path(directory)
            # Replace only hardware/logging dependencies; compile the production
            # service implementation and its public header without changing logic.
            source = (MANAGERS / "file_transfer_service.cpp").read_text()
            source = source.replace('#include "storage_manager.hpp"', '#include "mocks.hpp"')
            source = source.replace('#include "expt.hpp"', '#include "mocks.hpp"')
            (root / "service.cpp").write_text(source)
            (root / "SD.h").write_text('#include "mocks.hpp"\n')
            (root / "SPI.h").write_text('#include "mocks.hpp"\n')
            (root / "mocks.hpp").write_text(r'''
#pragma once
#include <exception>
#include <string>
inline bool supported = false, backendOk = true, lockOk = true;
inline bool available = true, locked = false, loggerAvailable = true;
inline int locks = 0, unlocks = 0, backendStarts = 0, sdProbes = 0, loggerChanges = 0;
struct StorageMock {
    bool isAvailable() const { return available; }
    bool isTransferLocked() const { return locked; }
    bool enterTransferLock() { ++locks; locked = lockOk; return lockOk; }
    bool exitTransferLock() { ++unlocks; locked = false; return true; }
};
inline StorageMock &storageManager() { static StorageMock storage; return storage; }
struct SdMock {
    bool begin(int) { ++sdProbes; return false; }
    void end() {}
};
struct SpiMock { void begin(int, int, int, int) {} };
inline SdMock SD;
inline SpiMock SPI;
inline void setLoggerUsbCdcAvailable(bool value) { ++loggerChanges; loggerAvailable = value; }
inline void flushBufferedLogMessages() {}
constexpr int DEBUG_VERBOSE_ERRORS = 1, DEBUG_VERBOSE_IMPORTANT = 2;
template<typename... Args> void debugLogMessage(Args...) {}
struct Exception {
    Exception(const char *, const char *) {}
    void print() const {}
    std::string flush(int) const { return "mock exception"; }
};
''')
            (root / "main.cpp").write_text(r'''
#include "file_transfer_service.hpp"
#include "file_transfer_usb_msc_bridge.hpp"
#include "mocks.hpp"
#include <cassert>
#include <iostream>
FileTransferUsbMscBridge &fileTransferUsbMscBridge() {
    static FileTransferUsbMscBridge bridge;
    return bridge;
}
bool FileTransferUsbMscBridge::isSupported() const { return supported; }
bool FileTransferUsbMscBridge::start(std::string &error) {
    ++backendStarts;
    if (!backendOk) error = "backend failed";
    return backendOk;
}
bool FileTransferUsbMscBridge::stop(std::string &) { return true; }
int main() {
    // Hardware CDC: rejecting MSC must never unmount SD or suspend logging.
    FileTransferService unsupported;
    assert(!unsupported.start());
    assert(unsupported.getState() == FileTransferState::UNSUPPORTED);
    assert(!unsupported.isTransferModeActive());
    assert(locks == 0 && unlocks == 0 && backendStarts == 0);
    assert(sdProbes == 0 && loggerChanges == 0 && loggerAvailable);
    assert(!unsupported.start());
    assert(locks == 0 && loggerChanges == 0);

    supported = true;
    available = false;
    FileTransferService missing;
    assert(!missing.start());
    assert(missing.getState() == FileTransferState::MISSING_SD);
    assert(locks == 0 && backendStarts == 0);

    available = true;
    lockOk = false;
    FileTransferService lockFailure;
    assert(!lockFailure.start());
    assert(!lockFailure.isTransferModeActive() && loggerAvailable);
    assert(backendStarts == 0);

    lockOk = true;
    backendOk = false;
    FileTransferService failed;
    assert(!failed.start());
    assert(failed.getState() == FileTransferState::ERROR);
    assert(failed.getLastMessage() == "backend failed");
    assert(!locked && loggerAvailable && !failed.isTransferModeActive());
    assert(unlocks == 1);

    backendOk = true;
    FileTransferService ready;
    assert(ready.start());
    assert(ready.getState() == FileTransferState::READY);
    assert(locked && !loggerAvailable && ready.isTransferModeActive());
    const int previousStarts = backendStarts, previousLocks = locks;
    assert(ready.start());
    assert(backendStarts == previousStarts && locks == previousLocks);
    assert(ready.stop());
    assert(!locked && loggerAvailable && !ready.isTransferModeActive());
    std::cout << "Transfer startup and rollback checks passed\n";
}
''')
            executable = root / "transfer_test.exe"
            subprocess.run([
                "g++", "-std=c++17", "-DARDUINO=10819", "-I", str(root),
                "-I", str(MANAGERS), str(root / "service.cpp"),
                str(root / "main.cpp"), "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()

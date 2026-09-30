from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ConnectionErrorReportingTest(unittest.TestCase):
    def read(self, name):
        return (ROOT / name).read_text(encoding="utf-8")

    def test_device_manager_preserves_protocol_and_device_errors(self):
        header = self.read("src/managers/device_manager.hpp")
        source = self.read("src/managers/device_manager.cpp")
        self.assertIn("const std::string& getLastError() const", header)
        self.assertIn('lastError = "INIT failed: ";', source)
        self.assertIn("INIT timed out: the Board did not answer over UART", source)
        self.assertIn("INIT timed out: the connected Board did not answer over BLE", source)
        self.assertIn("response.error.c_str()", source)
        self.assertIn("device->getError()", source)
        self.assertIn('lastError = "Physical transport disconnected.";', source)

    def test_failed_connect_rolls_back_pin_map_and_preserves_error(self):
        manager = self.read("src/managers/device_manager.cpp")
        connect = manager.split("bool DeviceManager::connectAssignedDevice", 1)[1].split(
            "void DeviceManager::erase", 1)[0]
        self.assertLess(connect.index("const std::string connectionError"),
                        connect.index("unassignAllPinsForDevice(device)"))
        self.assertLess(connect.index("unassignAllPinsForDevice(device)"),
                        connect.index("lastError = connectionError"))
        menu = self.read("src/gui/menu_gui.cpp")
        self.assertIn("initializePins(); // CONNECT rollback removed the device", menu)

    def test_connection_guis_show_propagated_detail(self):
        files = [
            "src/gui/communication_selection_gui.cpp",
            "src/gui/menu_gui.cpp",
            "src/gui/device_selection_gui.cpp",
            "src/gui/gui_manager.cpp",
            "src/gui/signals_visualization_gui.cpp",
        ]
        combined = "\n".join(self.read(name) for name in files)
        self.assertGreaterEqual(combined.count("deviceManager.getLastError()"), 6)
        self.assertNotIn("Device connection failed. Check cable and emulator.", combined)
        self.assertNotIn("Unable to establish connection. Check cable connection and emulator.", combined)
        self.assertNotIn("Reconnect failed. Check Board power", combined)

    def test_connect_exceptions_are_human_readable(self):
        source = self.read("src/devices/base_device.cpp")
        connect = source.split("bool connectDevice", 1)[1].split("bool disconnectDevice", 1)[0]
        disconnect = source.split("bool disconnectDevice", 1)[1]
        self.assertIn("device->setError(ex.Message)", connect)
        self.assertIn("device->setError(ex.Message)", disconnect)
        self.assertNotIn("ex.flush(0)", connect)


if __name__ == "__main__":
    unittest.main()

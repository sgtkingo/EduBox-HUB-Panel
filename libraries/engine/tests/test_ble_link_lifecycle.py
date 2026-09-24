from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VSCP = ROOT.parent / "vscp" / "src"

class BleLifecycleTest(unittest.TestCase):
    def test_native_lifecycle(self):
        with tempfile.TemporaryDirectory(prefix="panel-ble-lifecycle-") as directory:
            executable = Path(directory) / "lifecycle.exe"
            subprocess.run(["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
                            "-DSTDIO_H_ENV", "-I", str(ROOT / "src"), "-I", str(VSCP),
                            str(ROOT / "tests/ble_link_lifecycle_test.cpp"),
                            *[str(VSCP / name) for name in (
                                "vscp_client.cpp", "vscp_codec.cpp", "vscp_types.cpp",
                                "io/vscp_transport.cpp")], "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True, timeout=15)

    def test_ui_safety_contract(self):
        controls = (ROOT / "src/devices/base_device.hpp").read_text(encoding="utf-8")
        sync = controls.split("void syncControls(", 1)[1].split("bool hasValues()", 1)[0]
        self.assertLess(sync.index("isControlsSync = true"), sync.index("protocolClient.control("))
        self.assertIn("protocolClient.stopCommunication();", sync)
        gui = (ROOT / "src/gui/signals_visualization_gui.cpp").read_text(encoding="utf-8")
        reconnect = gui.split("bool SignalsVisualizationGui::reconnectAfterDisconnect()", 1)[1].split("void ", 1)[0]
        self.assertIn("paused = true;", reconnect)
        main = (ROOT.parents[1] / "ui/ui.ino").read_text(encoding="utf-8")
        loop = main.split("void loop ()", 1)[1]
        self.assertLess(loop.index("protocolLink.service()"), loop.index("vscpClient.poll()"))
        self.assertLess(loop.index("serviceProtocolSafety()"), loop.index("vscpClient.poll()"))
        self.assertIn("deviceManager.shouldPollProtocol()", loop)

    def test_wireless_pin_keyboard_is_lazy_and_floating(self):
        gui = (ROOT / "src/gui/communication_selection_gui.cpp").read_text(encoding="utf-8")
        show_wireless = gui.split("void CommunicationSelectionGui::showWireless(bool remembered)", 1)[1].split(
            "void CommunicationSelectionGui::startWirelessConnection", 1)[0]
        show_keyboard = gui.split("void CommunicationSelectionGui::showWirelessKeyboard()", 1)[1].split(
            "void CommunicationSelectionGui::hideWirelessKeyboard()", 1)[0]
        self.assertNotIn("lv_keyboard_create", show_wireless)
        self.assertIn("lv_keyboard_create(wirelessKeyboardOverlay)", show_keyboard)
        self.assertIn("lv_obj_create(lv_layer_top())", show_keyboard)
        self.assertIn("LV_EVENT_FOCUSED", show_wireless)
        self.assertIn("LV_EVENT_CLICKED", show_wireless)

if __name__ == "__main__":
    unittest.main()

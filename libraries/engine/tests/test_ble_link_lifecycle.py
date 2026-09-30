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
        manager = (ROOT / "src/managers/device_manager.cpp").read_text(encoding="utf-8")
        end_session = manager.split("void DeviceManager::endProtocolSession()", 1)[1].split(
            "bool DeviceManager::reconnectProtocolLink()", 1)[0]
        self.assertLess(end_session.index("protocolClient.bye(true)"), end_session.index("stopWireless()"))
        central = (ROOT.parent / "edubox-ble/src/ble_central.cpp").read_text(encoding="utf-8")
        gui_manager = (ROOT / "src/gui/gui_manager.cpp").read_text(encoding="utf-8")
        navigation = gui_manager.split("void GuiManager::navigateTo", 1)[1].split("void GuiManager::navigateBack", 1)[0]
        self.assertIn("targetState == GuiState::COMMUNICATION_SELECTION", navigation)
        self.assertIn("currentState == GuiState::SELECTION", navigation)
        self.assertIn("deviceManager.endProtocolSession();", navigation)
        peripheral = (ROOT.parent / "edubox-ble/src/ble_peripheral.cpp").read_text(encoding="utf-8")
        forget = peripheral.split("bool Peripheral::forgetBond", 1)[1]
        self.assertLess(forget.index("advertising->stop()"), forget.index("deleteAllBonds()"))
        self.assertIn("if (enabled_ && (disconnected ||", central)


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

    def test_cable_commissioning_and_manual_settings_contract(self):
        gui = (ROOT / "src/gui/communication_selection_gui.cpp").read_text(encoding="utf-8")
        link = (ROOT.parents[1] / "ui/panel_protocol_link.hpp").read_text(encoding="utf-8")
        self.assertIn("requestCablePairing(resetExisting)", gui)
        self.assertIn("attemptCablePairing(forgetOnly); // First request is immediate", gui)
        self.assertIn("beginCablePairing(true)", gui)
        self.assertIn("WirelessFlow::LocalForgetting", gui)
        self.assertIn("flow != WirelessFlow::LocalForgetting", gui)
        self.assertIn("lv_tick_elaps(wirelessFlowStarted) >= 5000", gui)
        self.assertIn("lv_tick_elaps(lastPairAttempt) >= 500", gui)
        self.assertIn("targetBoardId == info.peers[i].name", gui)
        self.assertIn("CablePairingStatus::AlreadyPaired", gui)
        self.assertNotIn('addButton("Remembered"', gui)
        self.assertIn('addButton("Connect"', gui)
        self.assertIn('addButton("Scan"', gui)
        self.assertIn('addButton("Forget pairing"', gui)
        self.assertIn("client_.pair(resetExisting)", link)
        self.assertIn("source.savedBoardId", link)

if __name__ == "__main__":
    unittest.main()

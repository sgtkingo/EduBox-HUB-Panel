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

if __name__ == "__main__":
    unittest.main()

import unittest

from emulator.engine.emulator import VSCPEmulator, find_vscp_request_start
from emulator.engine.emulator_patterns import find_vscp_request_start as find_pattern_request_start


class ProtocolFramingTest(unittest.TestCase):
    def test_request_detection_does_not_depend_on_type_order(self):
        request = "?api=1.6&app=board&db=1.3&type=INIT"
        self.assertEqual(find_vscp_request_start(request), 0)
        self.assertEqual(find_pattern_request_start(request), 0)

        emulator = VSCPEmulator()
        messages, remainder = emulator._extract_messages(request + "\r\n")
        self.assertEqual(messages, [("request", request)])
        self.assertEqual(remainder, "")
        self.assertIn("status=1", emulator.process_request(request))

    def test_request_is_extracted_after_a_log_prefix(self):
        request = "?db=1.3&type=INIT&api=1.6&app=board"
        line = "DEBUG: transport ready " + request

        emulator = VSCPEmulator()
        messages, remainder = emulator._extract_messages(line + "\n")
        self.assertEqual(messages, [("log", "DEBUG: transport ready"), ("request", request)])
        self.assertEqual(remainder, "")


if __name__ == "__main__":
    unittest.main()

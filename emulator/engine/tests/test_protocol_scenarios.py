import io
import random
import unittest
from contextlib import redirect_stdout
from unittest.mock import patch

from emulator.engine.emulator import VSCPEmulator as BasicEmulator
from emulator.engine.emulator_patterns import VSCPEmulator as PatternEmulator


SCENARIOS = (
    "normal",
    "high_activity",
    "thermal_event",
    "environmental",
    "vibration",
    "recovery",
)

SENSORS = {
    "S01": {
        "temp": 24,
        "humi": 55,
        "type": "DHT11",
        "_value_access": {"temp": "read", "humi": "read"},
        "_value_dtypes": {"temp": "int", "humi": "int"},
        "_restrictions": {
            "temp": {"min": -20, "max": 80},
            "humi": {"min": 0, "max": 100},
        },
    },
    "S02": {
        "Pressure": 1013.25,
        "Temperature": 22.1,
        "type": "BMP280",
        "_value_access": {"Pressure": "read", "Temperature": "read"},
        "_value_dtypes": {"Pressure": "float", "Temperature": "float"},
        "_restrictions": {
            "Pressure": {"min": 900, "max": 1100},
            "Temperature": {"min": -40, "max": 120},
        },
    },
    "imu_001": {
        "acm_x": 0.0,
        "type": "IMU",
        "_value_access": {"acm_x": "read"},
        "_value_dtypes": {"acm_x": "float"},
        "_restrictions": {"acm_x": {"min": -20, "max": 20}},
    },
    "A00": {
        "Brightness": 40,
        "type": "PWM LED",
        "_configs": {"Enabled": "1"},
        "_control_values": {"Brightness": 40},
        "_value_access": {"Brightness": "write"},
        "_value_dtypes": {"Brightness": "int"},
    },
    "H00": {
        "set_point": 25,
        "temp": 20,
        "type": "Temperature Regulator",
        "_configs": {"speed": 2},
        "_control_values": {"set_point": 25},
        "_value_access": {"set_point": "write", "temp": "read"},
        "_value_dtypes": {"set_point": "int", "temp": "int"},
    },
}


def request(command, **parameters):
    # Keep type last to verify that routing does not depend on parameter order.
    fields = [f"{key}={value}" for key, value in parameters.items()]
    fields.append(f"type={command}")
    return "?" + "&".join(fields)


def response_parameters(emulator, command, **parameters):
    return emulator.parse_message(emulator.process_request(request(command, **parameters)))


class ProtocolScenarioTest(unittest.TestCase):
    def setUp(self):
        self.output = io.StringIO()

    def assert_success(self, response, expected_id=None):
        self.assertEqual(response.get("status"), "1", response)
        if expected_id is not None:
            self.assertEqual(response.get("id"), expected_id, response)

    def exercise_command_matrix(self, emulator_class):
        emulator = emulator_class(SENSORS)

        response = response_parameters(emulator, "UPDATE", id="S01")
        self.assertEqual(response.get("status"), "0")
        self.assertIn("initialized", response.get("error", "").lower())

        incompatible = emulator_class(SENSORS)
        response = response_parameters(
            incompatible, "INIT", api="9.9", app="board", db="1.3"
        )
        self.assertEqual(response.get("status"), "0")

        response = response_parameters(
            emulator, "INIT", api="1.5", app="board", db="1.3"
        )
        self.assert_success(response)

        response = response_parameters(emulator, "CONNECT", id="S01", pins="1,2")
        self.assert_success(response, "S01")

        response = response_parameters(emulator, "CONNECT", id="S02", pins="2,3")
        self.assertEqual(response.get("status"), "0")
        self.assertEqual(response.get("id"), "S02")

        response = response_parameters(emulator, "CONNECT", id="S02", pins="bad")
        self.assertEqual(response.get("status"), "0")
        self.assertEqual(response.get("id"), "S02")

        for uid, expected_values in {
            "S01": {"temp", "humi"},
            "S02": {"Pressure", "Temperature"},
            "imu_001": {"acm_x"},
        }.items():
            response = response_parameters(emulator, "UPDATE", id=uid)
            self.assert_success(response, uid)
            self.assertTrue(expected_values.issubset(response), response)

        response = response_parameters(emulator, "CONFIG", id="H00", speed="5")
        self.assert_success(response, "H00")
        self.assertEqual(emulator.sensor_configs["H00"]["speed"], "5")

        response = response_parameters(emulator, "CONTROL", id="H00", set_point="35")
        self.assert_success(response, "H00")
        self.assertEqual(emulator.control_values["H00"]["set_point"], "35")

        response = response_parameters(emulator, "CONTROL", id="H00", temp="50")
        self.assertEqual(response.get("status"), "0")
        self.assertEqual(response.get("id"), "H00")

        response = response_parameters(emulator, "UPDATE", id="missing")
        self.assertEqual(response.get("status"), "0")
        self.assertEqual(response.get("id"), "missing")

        response = response_parameters(emulator, "RESET", id="H00")
        self.assert_success(response, "H00")

        response = response_parameters(emulator, "DISCONNECT", id="S01")
        self.assert_success(response, "S01")

        response = response_parameters(emulator, "UNKNOWN", id="S01")
        self.assertEqual(response.get("status"), "0")

    def test_basic_emulator_command_matrix(self):
        random.seed(100)
        with redirect_stdout(self.output):
            self.exercise_command_matrix(BasicEmulator)

    def test_pattern_emulator_command_matrix(self):
        random.seed(100)
        with redirect_stdout(self.output):
            self.exercise_command_matrix(PatternEmulator)

    def test_pattern_updates_in_every_scenario(self):
        random.seed(200)
        emulator = PatternEmulator(SENSORS)
        with redirect_stdout(self.output):
            self.assert_success(
                response_parameters(emulator, "INIT", api="1.5", app="board", db="1.3")
            )

            for scenario in SCENARIOS:
                emulator.simulation_mode = scenario
                for uid in ("S01", "S02", "imu_001", "H00"):
                    with self.subTest(scenario=scenario, uid=uid):
                        response = response_parameters(emulator, "UPDATE", id=uid)
                        self.assert_success(response, uid)
                        self.assertNotIn("error", response)

    def test_scenarios_materially_change_representative_values(self):
        def sample(scenario, uid, key):
            random.seed(300)
            with patch("emulator.engine.emulator_patterns.time.time", return_value=1000.0):
                emulator = PatternEmulator(SENSORS)
                with redirect_stdout(self.output):
                    self.assert_success(
                        response_parameters(
                            emulator, "INIT", api="1.5", app="board", db="1.3"
                        )
                    )
                    emulator.simulation_mode = scenario
                    response = response_parameters(emulator, "UPDATE", id=uid)
                self.assert_success(response, uid)
                return float(response[key])

        comparisons = (
            ("high_activity", "S02", "Pressure"),
            ("thermal_event", "S02", "Temperature"),
            ("environmental", "S02", "Pressure"),
            ("vibration", "imu_001", "acm_x"),
            ("recovery", "S02", "Pressure"),
        )
        for scenario, uid, key in comparisons:
            with self.subTest(scenario=scenario, uid=uid, key=key):
                self.assertNotEqual(
                    sample("normal", uid, key),
                    sample(scenario, uid, key),
                )


if __name__ == "__main__":
    unittest.main()

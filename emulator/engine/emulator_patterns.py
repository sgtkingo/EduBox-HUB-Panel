#!/usr/bin/env python3
"""
Virtual Sensors Protocol (VSCP) Emulator
========================================

Reactive client emulator that implements the complete VSCP communication protocol
as specified in vscp.hpp. Handles all protocol methods with dummy responses.

Protocol Methods:
- INIT: Handshake and version compatibility check
- UPDATE: Sensor data update requests
- CONFIG: Configuration changes from HMI to HW
- RESET: Sensor reset operations
- CONNECT: Connect sensor to specific pin
- DISCONNECT: Disconnect sensor from pin
- PING: Link heartbeat, independent of INIT and sensor sessions

Protocol Format: URL-like with key-value pairs
Request: ?type=METHOD&param1=value1&param2=value2
Response: ?status=1/0&param1=value1&error=message

Author: Generated for VSCP Protocol Testing
"""

import time
import random
import threading
import math
import sys
import traceback
from urllib.parse import unquote
from typing import Dict, Any, Optional

try:
    import serial
except ModuleNotFoundError:
    serial = None

try:
    from .emulator import (
        DEFAULT_APP_NAME,
        DEFAULT_DB_VERSION,
        INT_DTYPES,
        FLOAT_DTYPES,
        PROTOCOL_API_VERSION,
        available_serial_ports,
        load_catalog_defaults,
        ping_response,
    )
except ImportError:
    from emulator import (
        DEFAULT_APP_NAME,
        DEFAULT_DB_VERSION,
        INT_DTYPES,
        FLOAT_DTYPES,
        PROTOCOL_API_VERSION,
        available_serial_ports,
        load_catalog_defaults,
        ping_response,
    )

for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(encoding="utf-8", errors="replace")


def is_firmware_log_line(line: str) -> bool:
    upper_line = line.upper()
    return "DEBUG" in upper_line or "WARNING" in upper_line or "WARN:" in upper_line or "EXCEPTION" in upper_line

def find_vscp_request_start(line: str) -> int:
    lower_line = line.lower()
    search_from = 0
    while True:
        request_start = lower_line.find("?", search_from)
        if request_start == -1:
            return -1

        candidate = lower_line[request_start + 1:]
        if any(part.strip().startswith("type=") for part in candidate.split("&")):
            return request_start
        search_from = request_start + 1

class VSCPEmulator:
    """Virtual Sensors Communication Protocol Emulator"""

    PATTERN_PROFILES: Dict[str, Dict[str, float]] = {
        "temp": {"base": 25.5, "range": 10, "trend": 0.1, "noise": 0.5},
        "Temperature": {"base": 22.1, "range": 12, "trend": 0.08, "noise": 0.4},
        "humi": {"base": 80, "range": 30, "trend": -0.2, "noise": 2},
        "pressure": {"base": 1013.25, "range": 50, "trend": 0.01, "noise": 1.5},
        "Pressure": {"base": 1013.25, "range": 50, "trend": 0.01, "noise": 1.5},
        "intensity": {"base": 85, "range": 70, "trend": 0, "noise": 5},
        "lux_est": {"base": 11.5, "range": 100, "trend": 0, "noise": 2},
        "dBFS": {"base": 89.5, "range": 40, "trend": 0, "noise": 3},
        "peak": {"base": 12.0, "range": 20, "trend": 0, "noise": 1.5},
        "large": {"base": 12500, "range": 250000, "trend": 100, "noise": 2500},
        "tiny": {"base": 0.00025, "range": 0.00075, "trend": 0, "noise": 0.00005},
        "normal": {"base": 500, "range": 500, "trend": 2, "noise": 25},
        "micro": {"base": 0.00000254, "range": 0.000007, "trend": 0, "noise": 0.0000004},
    }

    DEVICE_PATTERN_PROFILES: Dict[tuple[str, str], Dict[str, float]] = {
        ("cpu_temp", "temp"): {"base": 55.3, "range": 25, "trend": 0.2, "noise": 1.2},
        ("H00", "temp"): {"base": 20, "range": 0, "trend": 0, "noise": 0},
        ("H00", "set_point"): {"base": 25, "range": 0, "trend": 0, "noise": 0},
        ("A00", "Brightness"): {"base": 40, "range": 0, "trend": 0, "noise": 0},
    }

    def __init__(
        self,
        sensors: Optional[Dict[str, Dict[str, Any]]] = None,
        port='COM3',
        baudrate=115200,
        timeout=0.1,
        api_version: str = PROTOCOL_API_VERSION,
        db_version: Optional[str] = None,
        app_name: Optional[str] = None,
        strict_api: bool = True,
    ):
        """Initialize the VSCP emulator"""
        catalog_sensors, metadata = load_catalog_defaults()

        self.API_VERSION = api_version
        self.DB_VERSION = db_version or metadata.get("version", DEFAULT_DB_VERSION)
        self.APP_NAME = app_name or metadata.get("application", DEFAULT_APP_NAME)
        self.APP_VERSION = "1.0.0"
        self.strict_api = strict_api
        
        # Protocol state
        self.initialized = False
        self.connected_sensors = {}  # uid -> pin mapping
        self.sensor_configs = {}     # uid -> config dict
        self.control_values = {}     # uid -> control dict
        
        # Serial connection
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.running = False
        
        # Simulation state tracking
        self.start_time = time.time()
        self.sensor_states = {}  # Track sensor simulation state
        self.simulation_mode = "normal"  # Current simulation scenario
        
        self.sensor_data = self._build_pattern_catalog(sensors if sensors is not None else catalog_sensors)
        
        # Initialize sensor states
        for uid in self.sensor_data:
            self.sensor_states[uid] = {
                "last_update": 0,
                "phase_offset": random.uniform(0, 2 * math.pi),
                "peak_timer": 0,
                "dropout_timer": 0,
                "trend_accumulator": 0,
                "spike_countdown": random.randint(50, 200)
            }
            configs = self.sensor_data[uid].get("_configs", {})
            if isinstance(configs, dict):
                self.sensor_configs[uid] = {key: str(value) for key, value in configs.items()}
            control_defaults = self.sensor_data[uid].get("_control_values", {})
            if isinstance(control_defaults, dict):
                self.control_values[uid] = {key: str(value) for key, value in control_defaults.items()}

    @staticmethod
    def _stringify(value: Any) -> str:
        if isinstance(value, bool):
            return "1" if value else "0"
        return str(value)

    @staticmethod
    def _is_numeric_value(value: Any) -> bool:
        if isinstance(value, bool):
            return False
        try:
            float(value)
            return True
        except (TypeError, ValueError):
            return False

    @staticmethod
    def _is_float_dtype(dtype: Any) -> bool:
        return str(dtype).lower() in FLOAT_DTYPES

    @staticmethod
    def _is_int_dtype(dtype: Any) -> bool:
        return str(dtype).lower() in INT_DTYPES

    @classmethod
    def _format_float(cls, value: Any) -> str:
        numeric = cls._to_float(value, 0.0)
        abs_value = abs(numeric)
        if abs_value != 0.0 and abs_value < 0.001:
            text = f"{numeric:.9f}"
        elif abs_value < 1.0:
            text = f"{numeric:.6f}"
        else:
            text = f"{numeric:.2f}"
        return text.rstrip("0").rstrip(".") if "." in text else text

    @classmethod
    def _infer_pattern_config(cls, uid: str, key: str, value: Any, payload: Dict[str, Any]) -> Dict[str, float]:
        profile = cls.DEVICE_PATTERN_PROFILES.get((uid, key)) or cls.PATTERN_PROFILES.get(key)
        if profile:
            config = dict(profile)
            if cls._is_numeric_value(value) and key not in {"temp", "humi", "pressure", "Pressure", "intensity", "lux_est", "dBFS", "peak"}:
                config["base"] = float(value)
            return config

        base = float(value) if cls._is_numeric_value(value) else 0.0
        restrictions = payload.get("_restrictions", {}).get(key, {})
        minimum = restrictions.get("min")
        maximum = restrictions.get("max")
        if minimum is not None and maximum is not None:
            min_value = float(minimum)
            max_value = float(maximum)
            span = max(0.0, max_value - min_value)
            range_value = max(span * 0.25, 1.0)
            if base == 0.0 and min_value <= max_value:
                base = min_value + span / 2
        elif maximum is not None:
            range_value = max(abs(float(maximum)) * 0.25, 1.0)
        else:
            range_value = max(abs(base) * 0.2, 1.0)

        return {
            "base": base,
            "range": range_value,
            "trend": 0.0,
            "noise": max(range_value * 0.05, 0.1),
        }

    @classmethod
    def _build_pattern_catalog(cls, sensors: Dict[str, Dict[str, Any]]) -> Dict[str, Dict[str, Any]]:
        pattern_catalog: Dict[str, Dict[str, Any]] = {}
        for uid, payload in sensors.items():
            pattern_payload: Dict[str, Any] = {}
            for key, value in payload.items():
                if key.startswith("_"):
                    pattern_payload[key] = value
                elif key == "type":
                    pattern_payload[key] = value
                elif isinstance(value, dict) and "base" in value:
                    pattern_payload[key] = value
                elif cls._is_numeric_value(value):
                    pattern_payload[key] = cls._infer_pattern_config(uid, key, value, payload)
                else:
                    pattern_payload[key] = value
            pattern_catalog[uid] = pattern_payload
        return pattern_catalog
        
    def connect_serial(self) -> bool:
        """Connect to serial port"""
        if serial is None:
            print("pyserial is not installed. Install emulator/requirements.txt before using serial mode.")
            return False

        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            if not self.ser.is_open:
                self.ser.open()
            print(f"✓ Connected to {self.port} at {self.baudrate} baud")
            return True
        except Exception as e:
            print(f"✗ Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect_serial(self):
        """Disconnect from serial port"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✓ Serial connection closed")
    
    def parse_message(self, message: str) -> Dict[str, str]:
        """Parse protocol message into key-value pairs"""
        params = {}
        
        # Remove leading '?' if present
        clean_message = message.strip()
        if clean_message.startswith('?'):
            clean_message = clean_message[1:]
        
        # Split by '&' and parse key=value pairs
        if clean_message:
            pairs = clean_message.split('&')
            for pair in pairs:
                if '=' in pair:
                    key, value = pair.split('=', 1)
                    params[key.strip()] = unquote(value.strip())
        
        return params
    
    def build_message(self, params: Dict[str, Any]) -> str:
        """Build protocol message from key-value pairs"""
        if not params:
            return "?status=0&error=No parameters"
        
        parts = []
        for key, value in params.items():
            parts.append(f"{key}={self._stringify(value)}")
        
        return "?" + "&".join(parts)

    def _require_initialized(self) -> Optional[str]:
        if self.initialized:
            return None
        return self.build_message({'status': '0', 'error': 'Protocol not initialized'})

    def _require_device_id(self, uid: str) -> Optional[str]:
        if not uid:
            return self.build_message({'id': uid, 'status': '0', 'error': 'UID cannot be empty'})
        if uid not in self.sensor_data:
            return self.build_message({'id': uid, 'status': '0', 'error': f'Device {uid} not found'})
        return None
    
    def handle_init(self, params: Dict[str, str]) -> str:
        """Handle INIT method - handshake and version check"""
        print(f"🔄 INIT request: {params}")
        
        # Extract parameters
        app = params.get('app', 'Unknown')
        dbversion = params.get('db', '')
        api = params.get('api', '')
        
        # Simulate version compatibility check
        response_params = {}
        
        if not self.strict_api or not api or api == self.API_VERSION:
            self.initialized = True
            response_params = {
                'status': '1'
            }
            print(f"✓ Initialization successful for {app}")
        else:
            response_params = {
                'status': '0',
                'error': f'Version mismatch - API:{api} (need {self.API_VERSION}), DB:{dbversion} (need {self.DB_VERSION})'
            }
            print(f"✗ Version mismatch: API {api}, DB {dbversion}")
        
        return self.build_message(response_params)
    
    def simulate_realistic_value(self, uid: str, param_name: str, config: Dict[str, float]) -> float:
        """Generate realistic sensor values with peaks, dropdowns, trends, and noise"""
        current_time = time.time()
        elapsed = current_time - self.start_time
        state = self.sensor_states[uid]
        
        base = config["base"]
        range_val = config["range"]
        trend = config["trend"]
        noise_level = config["noise"]
        
        # Apply scenario-based modifications
        scenario_multiplier = 1.0
        scenario_offset = 0.0
        
        if self.simulation_mode == "high_activity":
            scenario_multiplier = 1.5
            noise_level *= 2.0
        elif self.simulation_mode == "thermal_event" and param_name in ["temp", "Temperature"]:
            scenario_offset = range_val * 0.3
            scenario_multiplier = 1.3
        elif self.simulation_mode == "environmental" and param_name in ["Pressure", "intensity", "lux_est"]:
            scenario_multiplier = 0.7
            noise_level *= 1.5
        elif self.simulation_mode == "vibration" and uid in ["imu_001", "S05"]:
            scenario_multiplier = 2.0
            noise_level *= 3.0
        elif self.simulation_mode == "recovery":
            scenario_multiplier = 0.8
        
        # Time-based components
        slow_cycle = math.sin(elapsed * 0.05 + state["phase_offset"]) * 0.3  # Slow oscillation
        fast_cycle = math.sin(elapsed * 0.5 + state["phase_offset"]) * 0.1   # Fast oscillation
        
        # Trend accumulation (drift over time)
        state["trend_accumulator"] += trend * 0.01
        if abs(state["trend_accumulator"]) > range_val * 0.5:
            state["trend_accumulator"] *= 0.8  # Prevent runaway trends
        
        # Peak generation (sudden spikes)
        peak_factor = 0
        state["spike_countdown"] -= 1
        if state["spike_countdown"] <= 0:
            state["peak_timer"] = random.randint(5, 15)  # Peak duration
            state["spike_countdown"] = random.randint(100, 300)  # Next spike timing
        
        if state["peak_timer"] > 0:
            peak_intensity = math.exp(-((15 - state["peak_timer"]) ** 2) / 10)  # Gaussian peak
            peak_factor = random.uniform(0.5, 1.5) * peak_intensity * scenario_multiplier
            state["peak_timer"] -= 1
        
        # Dropout simulation (sudden drops)
        dropout_factor = 0
        if random.random() < (0.005 * scenario_multiplier):  # Scenario affects dropout chance
            state["dropout_timer"] = random.randint(3, 10)
        
        if state["dropout_timer"] > 0:
            dropout_factor = -random.uniform(0.3, 0.8) * scenario_multiplier
            state["dropout_timer"] -= 1
        
        # Environmental noise patterns
        if param_name in ["temp", "Temperature"]:
            # Temperature has daily cycle + weather patterns
            daily_cycle = math.sin(elapsed * 0.1) * 0.4  # Simulate daily temperature variation
            weather_noise = math.sin(elapsed * 0.02) * 0.3  # Weather pattern
            environmental = daily_cycle + weather_noise
        elif param_name in ["humi"]:
            # Humidity inversely related to temperature
            environmental = -math.sin(elapsed * 0.1) * 0.3
        elif param_name in ["Pressure", "pressure"]:
            # Barometric pressure has weather patterns
            environmental = math.sin(elapsed * 0.01) * 0.5 + math.sin(elapsed * 0.03) * 0.2
        elif param_name in ["intensity", "lux_est"]:
            # Light follows day/night cycle with cloud variations
            day_night = math.sin(elapsed * 0.15) * 0.8
            clouds = math.sin(elapsed * 0.3) * 0.3 * random.uniform(0.5, 1.0)
            environmental = max(0.1, day_night + clouds)  # Don't go completely dark
        elif param_name in ["dBFS", "peak"]:
            # Audio has bursts and quiet periods
            burst_factor = scenario_multiplier
            if random.random() < 0.02:  # 2% chance of audio burst
                burst_factor *= random.uniform(1.5, 3.0)
            environmental = math.sin(elapsed * 0.2) * 0.2 * burst_factor
        else:
            environmental = 0
        
        # Random noise (always present)
        noise = random.gauss(0, noise_level * 0.1)
        
        # Occasional large spikes/glitches (affected by scenario)
        if random.random() < (0.001 * scenario_multiplier):  # 0.1% base chance
            noise += random.uniform(-noise_level * 2, noise_level * 2) * scenario_multiplier
        
        # Combine all factors
        total_variation = (
            slow_cycle * range_val * 0.3 * scenario_multiplier +
            fast_cycle * range_val * 0.1 * scenario_multiplier +
            state["trend_accumulator"] +
            peak_factor * range_val * 0.4 +
            dropout_factor * range_val * 0.6 +
            environmental * range_val * 0.3 * scenario_multiplier +
            noise +
            scenario_offset
        )
        
        # Calculate final value with bounds checking
        final_value = base + total_variation
        
        # Apply realistic bounds for specific sensor types
        if param_name in ["humi"]:
            final_value = max(0, min(100, final_value))  # Humidity 0-100%
        elif param_name in ["Button", "Detected"]:
            # Binary sensors more active during high activity scenarios
            active_chance = 0.1 if self.simulation_mode != "high_activity" else 0.3
            final_value = 1 if random.random() < active_chance else 0
        elif param_name in ["intensity", "lux_est"] and final_value < 0:
            final_value = abs(final_value) * 0.1  # Light can't be negative
        elif param_name in ["Pressure", "pressure"]:
            final_value = max(900, min(1100, final_value))  # Reasonable pressure range

        restrictions = self.sensor_data.get(uid, {}).get("_restrictions", {}).get(param_name, {})
        if "min" in restrictions:
            final_value = max(float(restrictions["min"]), final_value)
        if "max" in restrictions:
            final_value = min(float(restrictions["max"]), final_value)
        
        return final_value

    @staticmethod
    def _to_float(value: Any, fallback: float) -> float:
        try:
            return float(value)
        except (TypeError, ValueError):
            return fallback

    @staticmethod
    def _to_int(value: Any, fallback: int) -> int:
        try:
            return int(float(value))
        except (TypeError, ValueError):
            return fallback

    def advance_temperature_regulator(self, uid: str) -> bool:
        sensor_config = self.sensor_data.get(uid, {})
        value_access = sensor_config.get("_value_access", {})
        if value_access.get("set_point") != "write" or value_access.get("temp", "read") != "read":
            return False

        temp_config = sensor_config.get("temp")
        if not isinstance(temp_config, dict) or "base" not in temp_config:
            return False

        set_point_config = sensor_config.get("set_point", {})
        fallback_set_point = set_point_config.get("base", temp_config.get("base", 20)) if isinstance(set_point_config, dict) else temp_config.get("base", 20)
        current_temp = self._to_float(temp_config.get("base"), 20.0)
        set_point = self._to_float(self.control_values.get(uid, {}).get("set_point", fallback_set_point), current_temp)
        speed = self._to_int(self.sensor_configs.get(uid, {}).get("speed", sensor_config.get("_configs", {}).get("speed", 2)), 2)
        speed = max(1, min(5, speed))

        delta = set_point - current_temp
        if abs(delta) <= speed:
            next_temp = set_point
        else:
            next_temp = current_temp + (speed if delta > 0 else -speed)

        temp_config["base"] = int(round(next_temp))
        return True

    def handle_update(self, params: Dict[str, str]) -> str:
        """Handle UPDATE method - return realistic sensor data"""
        uid = params.get('id', '')
        print(f"📊 UPDATE request for sensor: {uid}")
        
        error = self._require_initialized()
        if error:
            return error

        error = self._require_device_id(uid)
        if error:
            return error
        
        if uid in self.sensor_data:
            # Get sensor configuration
            sensor_config = self.sensor_data[uid]
            sensor_info = {}
            value_access = sensor_config.get("_value_access", {})
            value_dtypes = sensor_config.get("_value_dtypes", {})
            regulator_updated = self.advance_temperature_regulator(uid)
            
            # Generate realistic values for each parameter
            for key, config in sensor_config.items():
                if key.startswith('_') or key == 'type':
                    continue
                if value_access.get(key, "read") == "write":
                    continue
                if isinstance(config, dict) and 'base' in config:
                    # Generate realistic value using simulation
                    value = self.simulate_realistic_value(uid, key, config)
                    dtype = value_dtypes.get(key, "")
                    
                    # Format appropriately (int vs float)
                    if self._is_int_dtype(dtype) or key in ["Button", "Detected", "X", "Y"] or (regulator_updated and key == "temp" and not self._is_float_dtype(dtype)):
                        sensor_info[key] = int(round(value))
                    else:
                        sensor_info[key] = self._format_float(value)
                else:
                    # Fallback for any static values
                    sensor_info[key] = config
            
            response_params = {'id': uid, 'status': '1'}
            response_params.update(sensor_info)
            
            print(f"✓ Sensor {uid} data: {sensor_info}")
        else:
            response_params = {
                'id': uid,
                'status': '0',
                'error': f'Sensor {uid} not found'
            }
            print(f"✗ Sensor {uid} not found")
        
        return self.build_message(response_params)
    
    def adjust_simulation_scenario(self):
        """Dynamically adjust simulation scenarios to create interesting patterns"""
        current_time = time.time()
        elapsed = current_time - self.start_time
        
        # Every 60 seconds, introduce scenario changes
        scenario_phase = int(elapsed / 60) % 6
        
        if scenario_phase == 0:
            # Normal operation
            self.simulation_mode = "normal"
        elif scenario_phase == 1:
            # High activity period (all sensors more active)
            self.simulation_mode = "high_activity"
            for uid in self.sensor_states:
                self.sensor_states[uid]["spike_countdown"] = min(
                    self.sensor_states[uid]["spike_countdown"], 20
                )
        elif scenario_phase == 2:
            # Thermal event (temperature sensors spike)
            self.simulation_mode = "thermal_event"
            for uid in ["S00", "S01", "S02", "cpu_temp"]:
                if uid in self.sensor_states:
                    self.sensor_states[uid]["peak_timer"] = 30
        elif scenario_phase == 3:
            # Environmental disturbance (pressure and light affected)
            self.simulation_mode = "environmental"
            for uid in ["S02", "S15", "cam_001"]:
                if uid in self.sensor_states:
                    self.sensor_states[uid]["dropout_timer"] = 15
        elif scenario_phase == 4:
            # Mechanical vibration (IMU and magnetic sensors affected)
            self.simulation_mode = "vibration"
            for uid in ["imu_001", "S05"]:
                if uid in self.sensor_states:
                    self.sensor_states[uid]["spike_countdown"] = 5
        else:
            # Recovery period (gradual return to normal)
            self.simulation_mode = "recovery"
    
    def handle_config(self, params: Dict[str, str]) -> str:
        """Handle CONFIG method - configure sensor"""
        uid = params.get('id', '')
        print(f"⚙️  CONFIG request for sensor: {uid}")
        
        error = self._require_initialized()
        if error:
            return error

        error = self._require_device_id(uid)
        if error:
            return error
        
        # Extract configuration parameters (exclude 'type' and 'id')
        config_params = {k: v for k, v in params.items() if k not in ['type', 'id']}
        
        if uid:
            # Store configuration
            self.sensor_configs.setdefault(uid, {}).update(config_params)
            response_params = {
                'id': uid,
                'status': '1'
            }
            print(f"✓ Sensor {uid} configured: {config_params}")
        else:
            response_params = {
                'id': uid,
                'status': '0',
                'error': 'Invalid sensor ID'
            }
            print(f"✗ Invalid sensor ID: {uid}")
        
        return self.build_message(response_params)

    def handle_control(self, params: Dict[str, str]) -> str:
        """Handle CONTROL method - apply runtime control values"""
        uid = params.get('id', '')
        print(f"CONTROL request for device: {uid}")

        error = self._require_initialized()
        if error:
            return error

        error = self._require_device_id(uid)
        if error:
            return error

        control_params = {k: v for k, v in params.items() if k not in ['type', 'id']}

        if uid and uid in self.sensor_data:
            value_access = self.sensor_data[uid].get("_value_access", {})
            if value_access:
                invalid = [key for key in control_params if value_access.get(key) != "write"]
                if invalid:
                    return self.build_message({
                        'id': uid,
                        'status': '0',
                        'error': f"Values are not writable through CONTROL: {','.join(invalid)}"
                    })

            self.control_values.setdefault(uid, {}).update(control_params)
            for key, value in control_params.items():
                value_config = self.sensor_data[uid].get(key)
                if isinstance(value_config, dict) and "base" in value_config and self._is_numeric_value(value):
                    value_config["base"] = float(value)
            response_params = {
                'id': uid,
                'status': '1'
            }
            print(f"Device {uid} control applied: {control_params}")
        else:
            response_params = {
                'id': uid,
                'status': '0',
                'error': f'Device {uid} not found'
            }

        return self.build_message(response_params)
    
    def handle_reset(self, params: Dict[str, str]) -> str:
        """Handle RESET method - reset sensor"""
        uid = params.get('id', '')
        print(f"🔄 RESET request for sensor: {uid}")
        
        error = self._require_initialized()
        if error:
            return error
        
        if uid in self.sensor_data or uid == 'all':
            # Reset sensor(s)
            if uid == 'all':
                self.sensor_configs.clear()
                self.control_values.clear()
                self.connected_sensors.clear()
                print("✓ All sensors reset")
            else:
                self.sensor_configs.pop(uid, None)
                self.control_values.pop(uid, None)
                self.connected_sensors.pop(uid, None)
                print(f"✓ Sensor {uid} reset")
            
            response_params = {
                'id': uid,
                'status': '1'
            }
        else:
            response_params = {
                'id': uid,
                'status': '0',
                'error': f'Sensor {uid} not found'
            }
            print(f"✗ Sensor {uid} not found for reset")
        
        return self.build_message(response_params)
    
    def handle_connect(self, params: Dict[str, str]) -> str:
        """Handle CONNECT method - connect sensor to pin"""
        uid = params.get('id', '')
        pins = params.get('pins', '')
        print(f"🔌 CONNECT request: sensor {uid} to pins {pins}")
        
        error = self._require_initialized()
        if error:
            return error

        error = self._require_device_id(uid)
        if error:
            return error
        
        if uid and pins:
            try:
                pins_num = [int(pin) for pin in pins.split(',')]
                # Check if pin is already used
                used_by = None
                for sensor_id, used_pin in self.connected_sensors.items():
                    if set(used_pin).intersection(pins_num):
                        used_by = sensor_id
                        break
                
                if used_by and used_by != uid:
                    response_params = {
                        'id': uid,
                        'status': '0',
                        'error': f'Pins {pins} already used by sensor {used_by}'
                    }
                    print(f"✗ Pins {pins} conflict: used by {used_by}")
                else:
                    self.connected_sensors[uid] = pins_num
                    response_params = {
                        'id': uid,
                        'status': '1'
                    }
                    print(f"✓ Sensor {uid} connected to pins {pins}")

            except ValueError:
                response_params = {
                    'id': uid,
                    'status': '0',
                    'error': f'Invalid pin number: {pins}'
                }
                print(f"✗ Invalid pin number: {pins}")
        else:
            response_params = {
                'id': uid,
                'status': '0',
                'error': 'Missing sensor ID or pin number'
            }
            print(f"✗ Missing parameters: uid={uid}, pins={pins}")
        
        return self.build_message(response_params)
    
    def handle_disconnect(self, params: Dict[str, str]) -> str:
        """Handle DISCONNECT method - disconnect sensor from pin"""
        uid = params.get('id', '')
        print(f"🔌 DISCONNECT request for sensor: {uid}")
        
        error = self._require_initialized()
        if error:
            return error

        error = self._require_device_id(uid)
        if error:
            return error

        self.connected_sensors.pop(uid, None)
        return self.build_message({'id': uid, 'status': '1'})
        
    
    def process_request(self, message: str) -> str:
        """Process incoming protocol request"""
        try:
            params = self.parse_message(message)
            request_type = params.get('type', '').upper()
            if request_type == 'PING':
                response = ping_response(params)
                return self.build_message(response) if response is not None else ''

            # Route to appropriate handler
            handlers = {
                'INIT': self.handle_init,
                'UPDATE': self.handle_update,
                'CONFIG': self.handle_config,
                'CONTROL': self.handle_control,
                'RESET': self.handle_reset,
                'CONNECT': self.handle_connect,
                'DISCONNECT': self.handle_disconnect
            }

            if request_type in handlers:
                return handlers[request_type](params)
            else:
                return self.build_message({
                    'status': '0',
                    'error': f'Unknown request type: {request_type}'
                })
        except Exception as exc:
            print(f"EXCEPTION: Enhanced emulator request handling failed reason={exc} source=VSCPEmulator.process_request")
            traceback.print_exc()
            return self.build_message({
                'status': '0',
                'error': f'Emulator exception: {exc}'
            })
    
    def listen_loop(self):
        """Main listening loop for incoming requests"""
        print("🎧 Listening for protocol requests...")
        buffer = ""
        last_scenario_update = time.time()
        
        while self.running:
            try:
                # Update simulation scenarios periodically
                current_time = time.time()
                if current_time - last_scenario_update > 10:  # Update every 10 seconds
                    self.adjust_simulation_scenario()
                    last_scenario_update = current_time
                
                if self.ser and self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data
                    
                    if data and '\n' in buffer:
                        print(f"DEBUG: Serial data chunk received reason=serial read source=VSCPEmulator.listen_loop data={data!r}")
                    # Process complete messages (ending with newline or containing a VSCP request).
                    while '\n' in buffer or find_vscp_request_start(buffer) != -1:
                        if '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                        else:
                            # If no newline but contains '?', process the whole buffer
                            line = buffer
                            buffer = ""
                        
                        line = line.strip()
                        if is_firmware_log_line(line):
                            request_index = find_vscp_request_start(line)
                            if request_index == -1:
                                print(f"Firmware log: {line}")
                                continue

                            log_line = line[:request_index].strip()
                            if log_line:
                                print(f"Firmware log: {log_line}")
                            line = line[request_index:].strip()
                        else:
                            # Substring from VSCP request start if exists.
                            request_index = find_vscp_request_start(line)
                            if request_index != -1:
                                line = line[request_index:]

                        if line.startswith('?') and self.parse_message(line).get('type'):
                            print(f"📨 Received: {line}")
                            response = self.process_request(line)
                            
                            # Send response
                            if response:
                                self.ser.write((response + '\n').encode('utf-8'))
                                print(f"📤 Sent: {response}")
                
                time.sleep(0.01)  # Small delay to prevent busy waiting
                
            except Exception as e:
                print(f"EXCEPTION: Enhanced emulator listen loop failed reason={e} source=VSCPEmulator.listen_loop")
                traceback.print_exc()
                time.sleep(0.1)
    
    def run(self):
        """Start the emulator"""
        print("🚀 Starting Enhanced VSCP Emulator...")
        print(f"   API Version: {self.API_VERSION}")
        print(f"   DB Version: {self.DB_VERSION}")
        print(f"   Available sensors: {list(self.sensor_data.keys())}")
        print("   🎭 Simulation Features:")
        print("      • Realistic peaks and dropdowns")
        print("      • Environmental patterns (daily cycles, weather)")
        print("      • Dynamic scenarios (thermal events, vibrations, etc.)")
        print("      • Sensor-specific noise and trends")
        print("      • Correlated behavior between related sensors")
        
        if not self.connect_serial():
            return
        
        self.running = True
        
        # Start listening thread
        listen_thread = threading.Thread(target=self.listen_loop, daemon=True)
        listen_thread.start()
        
        try:
            print("\n💡 Enhanced emulator ready! Realistic sensor data patterns active.")
            print("   Example: ?type=INIT&app=board&db=1.0&api=1.5")
            print("   Press Ctrl+C to stop\n")
            
            # Keep main thread alive and show simulation status
            last_status = time.time()
            while True:
                time.sleep(1)
                current_time = time.time()
                if current_time - last_status > 30:  # Show status every 30 seconds
                    elapsed = int(current_time - self.start_time)
                    print(f"   📊 Runtime: {elapsed}s | Mode: {self.simulation_mode}")
                    last_status = current_time
                
        except KeyboardInterrupt:
            print("\n🛑 Shutting down enhanced emulator...")
        
        finally:
            self.running = False
            self.disconnect_serial()

def main():
    """Main entry point"""
    # Get available COM ports
    available_ports = available_serial_ports()
    
    if available_ports:
        default_port = available_ports[0]
        print(f"Available COM ports: {', '.join(available_ports)}")
    else:
        default_port = 'COM8'
        print("No COM ports detected, using default COM8")
    
    port = input(f"Enter serial port (default: {default_port}): ").strip()
    if not port:
        port = default_port
    emulator = VSCPEmulator(port=port, baudrate=115200)
    emulator.run()

if __name__ == "__main__":
    main()

# EduBox HUB Panel Data Formats

This document describes the stable data files and generated artifacts used by EduBox HUB Panel. For behavior-level context, see [Architecture Notes](ARCHITECTURE.md) and [PROTOCOL_EN.md](../PROTOCOL_EN.md).

## Repository Data Sources

| Path | Source of truth | Notes |
| --- | --- | --- |
| [`storage/data/DB.json`](../../storage/data/DB.json) | Yes | Canonical device catalog. |
| [`storage/data/config.json`](../../storage/data/config.json) | Yes | Canonical application config. |
| [`storage/data/pics/`](../../storage/data/pics/) | Yes | Canonical image assets copied next to firmware data. |
| [`ui/data/DB.json`](../../ui/data/DB.json) | Generated mirror | Firmware data-folder copy. |
| [`ui/data/config.json`](../../ui/data/config.json) | Generated mirror | Firmware data-folder copy. |
| [`ui/data/pics/`](../../ui/data/pics/) | Generated mirror | Firmware image assets. |
| [`libraries/engine/src/devices/default_json_db.hpp`](../../libraries/engine/src/devices/default_json_db.hpp) | Generated mirror | Embedded DB fallback string. |
| [`libraries/engine/src/managers/default_json_config.hpp`](../../libraries/engine/src/managers/default_json_config.hpp) | Generated mirror | Embedded config fallback string. |

After changing canonical data, run:

```bash
python storage/sync_config.py
python storage/sync_db.py
```

Firmware version is synchronized separately:

```bash
python storage/sync_version.py
```

## On-Device Storage Layout

In SD mode:

```text
/data/DB.json
/data/config.json
/data/pics/<image files>
/records/<bundle>.csv
```

In SPIFFS mode:

```text
/DB.json
/records/<bundle>.csv
```

SPIFFS mode can seed the embedded default DB when the catalog is missing. SD mode expects the `/data` files to be present on the card.

## Device Catalog: DB.json

Top-level shape:

```json
{
  "version": "1.3",
  "application": "board",
  "devices": {
    "H00": {
      "uid": "H00",
      "role": "hybrid",
      "type": "Temperature Regulator",
      "description": "Hybrid temperature regulator for testing UPDATE, CONTROL and CONFIG.",
      "picture": "placeholder:device",
      "Pins": ["CTRL", "SENSE", "PWM"],
      "allowedPins": [3, 5, 6, 9, 10, 11],
      "values": {},
      "configs": {},
      "default": {
        "values": {},
        "configs": {},
        "pins": {}
      }
    }
  }
}
```

### Top-Level Fields

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| `version` | string | Recommended | Catalog/DB version sent during VSCP `INIT`. |
| `application` | string | Recommended | Application/catalog name sent during VSCP `INIT`, currently `board`. |
| `devices` | object | Yes | Map of device key to device definition. |

The device key and `uid` should match. If `uid` is missing, the parser uses the map key.

### Device Fields

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| `uid` | string | Recommended | Stable protocol ID. |
| `role` | string | No | `sensor`, `actuator`, or `hybrid`. Missing means `sensor`. |
| `type` | string | Yes | User-facing device name. |
| `description` | string | No | Short catalog/wiki text. |
| `picture` | string | No | File in `/data/pics`, or placeholder reference. |
| `image` | string | No | Legacy alias for `picture`. |
| `Pins` | array or CSV string | Yes | Logical pin labels in display/order order. |
| `allowedPins` | array | No | Physical pin numbers allowed for this device. Empty/missing means any virtual pin. |
| `allowed_pins` | array | No | Legacy alias for `allowedPins`. |
| `values` | object | Required if no `configs` | Runtime telemetry/control values. |
| `configs` | object | Required if no `values` | Setup/configuration values sent through `CONFIG`. |
| `default` | object | No | Initial values/configs/pins overriding parameter `value`. |
| `order` | number | No | UI/catalog ordering. Missing keeps source order. |

The parser rejects a device if `type` is missing, if neither `values` nor `configs` exists, or if `Pins` is empty.

### Roles

| Role | Typical channels | Meaning |
| --- | --- | --- |
| `sensor` | `UPDATE` | Read-only telemetry device. |
| `actuator` | `CONTROL`, optional `CONFIG` | Output/control device. |
| `hybrid` | `UPDATE`, `CONTROL`, optional `CONFIG` | Device with both telemetry and control/config behavior. |

### Parameter Fields

Parameters inside `values` and `configs` have the same general shape:

```json
"temp": {
  "value": 20,
  "unit": "C",
  "dtype": "float",
  "access": "read",
  "restrictions": {
    "min": -40,
    "max": 120,
    "step": 0.5,
    "options": ["low", "medium", "high"]
  },
  "order": 10
}
```

| Field | Type | Required | Meaning |
| --- | --- | --- | --- |
| `value` | string/number/bool | No | Initial value if no matching default override exists. |
| `unit` | string | No | Unit label rendered in UI. |
| `dtype` | string | No | `int`, `float`, `double`, or `string`. Missing currently defaults to string. |
| `access` | string | Values only | `read` or `write`; missing means `read`. |
| `direction` | string | Values only | Legacy alias for `access`. |
| `restrictions` | object | No | Validation/editor hints. |
| `order` | number | No | UI ordering within the parameter group. |

Access aliases:

| Canonical | Accepted aliases |
| --- | --- |
| `read` | `in`, `input`, missing |
| `write` | `out`, `output`, `control` |

### Restrictions

| Field | Type | Applies to | Meaning |
| --- | --- | --- | --- |
| `min` | string/number | Numeric values | Minimum allowed value. |
| `max` | string/number | Numeric values | Maximum allowed value. |
| `step` | string/number | Numeric editors | UI increment/slider step. |
| `options` | array | String or enum-like values | Allowed choices. |

Restrictions are used by the device model and UI editors. The VSCP wire protocol still sends all values as strings.

### Defaults

Defaults override parameter `value` during catalog loading:

```json
"default": {
  "values": {
    "set_point": 25,
    "temp": 20
  },
  "configs": {
    "speed": 2
  },
  "pins": {
    "CTRL": 3,
    "SENSE": 5,
    "PWM": 6
  }
}
```

`default.pins` maps logical pin labels from `Pins` to physical pin numbers. If omitted, the user selects pins in the connection screen.

### Picture References

Supported conventions:

| Value | Meaning |
| --- | --- |
| `placeholder:device` | Generic device placeholder. |
| `placeholder:sensor` | Generic sensor placeholder if supported by the UI asset layer. |
| `placeholder:actuator` | Generic actuator placeholder if supported by the UI asset layer. |
| `placeholder:hybrid` | Generic hybrid placeholder if supported by the UI asset layer. |
| `DS18B20.png` | File under `/data/pics/` and mirrored from `storage/data/pics/`. |

Keep image filenames short and ASCII-friendly.

## Application Config: config.json

Current shape:

```json
{
  "defaultCommunication": "ask",
  "appearance": {
    "theme": "light",
    "language": "en"
  }
}
```

Known fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `defaultCommunication` | string | Startup communication preference. `ask` keeps the explicit selection flow. |
| `appearance.theme` | string | UI theme name. |
| `appearance.language` | string | UI language code. |

Keep config changes backward-compatible because older firmware may read the same SD card.

## DataBundle CSV

DataBundle recordings are stored as semicolon-separated CSV files under `/records/`.
Each file represents one recording session for one device.

### File Location and Naming

Runtime storage path:

```text
/records/<safe-device-base>_NN.csv
```

Filename generation:

1. Use `DeviceUid` when available; otherwise use `DeviceName`.
2. Keep only alphanumeric characters, `_`, and `-`.
3. Convert spaces and dots to `_`.
4. Trim trailing `_`.
5. Shorten the base name to 14 characters.
6. Append `_NN.csv`, starting at `_01.csv`; if the file exists, increment the number.

Examples:

| DeviceUid | DeviceName | Example file |
| --- | --- | --- |
| `H00` | `Temperature Regulator` | `/records/H00_01.csv` |
| `cpu_temp` | `CPU Temp` | `/records/cpu_temp_01.csv` |
| empty | `Multi Value Dummy Sensor` | `/records/Multi_Value_D_01.csv` |

### CSV Dialect

| Property | Value |
| --- | --- |
| Delimiter | `;` |
| Header row | Required |
| Encoding | UTF-8-compatible text |
| Line ending | `\n`; parser ignores `\r` |
| Quoting | Not implemented |
| Escaping | Not implemented |
| Decimal separator | `.` |

Because quoting and escaping are not implemented, do not put `;`, `\n`, or `\r` in device names, signal names, or values.

Header:

```csv
DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value
```

### Single-Signal Example

```csv
DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value
Temperature Regulator;H00;0;0;temp;20
Temperature Regulator;H00;1;101;temp;21
Temperature Regulator;H00;2;203;temp;22
```

### Multi-Signal Example

Multi-value devices write one row per saved signal sample. `SampleIndex` is global within the recording session, not per signal.

```csv
DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value
Multi Value Dummy Sensor;S99;0;0;temp;22.5
Multi Value Dummy Sensor;S99;1;0;humi;45
Multi Value Dummy Sensor;S99;2;0;pressure;1013.25
Multi Value Dummy Sensor;S99;3;104;temp;22.6
Multi Value Dummy Sensor;S99;4;104;humi;45
Multi Value Dummy Sensor;S99;5;104;pressure;1013.1
```

### Columns

| Column | Type | Meaning |
| --- | --- | --- |
| `DeviceName` | string | Device `type` at recording time. |
| `DeviceUid` | string | Device `uid` at recording time. |
| `SampleIndex` | unsigned integer | Monotonic sample counter within the recording. |
| `RuntimeMs` | unsigned integer | Milliseconds since recording start. |
| `SignalName` | string | Value key, for example `temp`. |
| `Value` | string | Runtime value as displayed/received. |

### Semantics

- A recording starts with `DataBundleManager::startRecording(deviceName, deviceUid)`.
- A row is buffered with `saveNewDataPoint(signalName, value)` whenever the runtime screen stores a sample.
- `RuntimeMs` is relative to recording start and does not require RTC, NTP, or a wall-clock timestamp.
- `SampleIndex` is monotonic for the whole file. It increments for every row written.
- `Value` is intentionally stored as text. Consumers should parse it according to the device catalog's `dtype` for the matching `SignalName`.
- Empty recordings are discarded and do not create CSV files.
- At most 30 bundles are kept; when storage is full, the oldest listed CSV is pruned.

### Databank Preview Behavior

The on-device Databank preview:

- loads up to 6 bundles per page,
- reads metadata from the first data row,
- finds the `SignalName` and `Value` columns by header name,
- chooses the first signal stream in the file,
- takes the first 10 values for that signal,
- repeats the last available value if fewer than 10 values exist.

This means the preview is intentionally lightweight. Full CSV export/readback should use all rows.

## VSCP Message Format

VSCP messages are URL-like query strings terminated by a newline:

```text
?type=UPDATE&id=H00
?id=H00&status=1&temp=24
```

Rules:

- The first character is `?`.
- Pairs are separated by `&`.
- Key and value are separated by the first `=`.
- Values are strings on the wire.
- Current firmware message building does not fully URL-encode values.
- Avoid `&`, `=`, semicolons, and newlines in catalog keys, protocol values, and CSV values.

For the full command list, see [PROTOCOL_EN.md](../PROTOCOL_EN.md).

## Firmware Build Artifacts

The build workflow expects and publishes:

| File | Meaning |
| --- | --- |
| `ui.ino.bin` | Main firmware app binary. |
| `ui.ino.bootloader.bin` | Bootloader binary. |
| `ui.ino.partitions.bin` | Partition table binary. |
| `ui.ino.merged.bin` | Merged flash image for simpler deployment/update tools. |

Release automation mirrors these into [`bin/latest/`](../../bin/latest/).

## Validation Checklist

Before committing format changes:

- `storage/data/DB.json` parses as JSON.
- Every device has `type`, at least one `values` or `configs` object, and non-empty `Pins`.
- `role`, `dtype`, and `access` use supported values or accepted aliases.
- `default.values` and `default.configs` keys match declared parameter keys.
- Image files referenced by `picture` exist in `storage/data/pics/` unless they use a `placeholder:*` value.
- `python storage/sync_config.py` and `python storage/sync_db.py` run cleanly.
- Firmware and emulator behavior are updated together when protocol-facing semantics change.

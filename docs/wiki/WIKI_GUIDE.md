# EduBox HUB Panel Wiki Guide

The EduBox HUB Panel "wiki" is currently the user-facing knowledge carried by the device catalog: device names, descriptions, images, values, configuration metadata, units, ranges, options, and pin labels. The catalog powers the HMI library, runtime screen, emulator defaults, and generated firmware fallback data.

For the schema reference, see [FORMATS.md](FORMATS.md).

## Where Wiki Content Lives

Canonical files:

- Device catalog: [`storage/data/DB.json`](../../storage/data/DB.json)
- Device images: [`storage/data/pics/`](../../storage/data/pics/)

Generated mirrors:

- [`ui/data/DB.json`](../../ui/data/DB.json)
- [`ui/data/pics/`](../../ui/data/pics/)
- [`libraries/engine/src/devices/default_json_db.hpp`](../../libraries/engine/src/devices/default_json_db.hpp)

Do not edit generated mirrors directly unless you are debugging a sync issue. Edit `storage/data/*`, then run:

```bash
python storage/sync_db.py
```

## Adding a New Device Entry

1. Choose a stable `uid`.
2. Add a new object under `devices` in [`storage/data/DB.json`](../../storage/data/DB.json).
3. Set `role` to `sensor`, `actuator`, or `hybrid`.
4. Write a short `type` and `description`.
5. Define logical `Pins`.
6. Add `values` for runtime telemetry/control values.
7. Add `configs` for setup/configuration values.
8. Add `default.values`, `default.configs`, and optionally `default.pins`.
9. Add a `picture` reference or use a placeholder.
10. Run `python storage/sync_db.py`.
11. Test the catalog in the HMI and emulator.

Minimal sensor example:

```json
"S20": {
  "uid": "S20",
  "role": "sensor",
  "type": "Soil Moisture",
  "description": "Analog soil moisture sensor for relative wetness measurement.",
  "values": {
    "moisture": {
      "value": 0,
      "unit": "%",
      "dtype": "int",
      "access": "read",
      "restrictions": {
        "min": 0,
        "max": 100
      }
    }
  },
  "configs": {},
  "default": {
    "values": {
      "moisture": 0
    },
    "pins": {}
  },
  "picture": "placeholder:device",
  "Pins": ["AOUT"]
}
```

Hybrid example:

```json
"H10": {
  "uid": "H10",
  "role": "hybrid",
  "type": "Fan Controller",
  "description": "Hybrid fan controller with measured speed and writable target power.",
  "values": {
    "rpm": {
      "value": 0,
      "unit": "rpm",
      "dtype": "int",
      "access": "read",
      "restrictions": {
        "min": 0
      }
    },
    "power": {
      "value": 40,
      "unit": "%",
      "dtype": "int",
      "access": "write",
      "restrictions": {
        "min": 0,
        "max": 100,
        "step": 5
      }
    }
  },
  "configs": {
    "mode": {
      "value": "auto",
      "unit": "",
      "dtype": "string",
      "restrictions": {
        "options": ["auto", "manual"]
      }
    }
  },
  "allowedPins": [3, 5, 6, 9, 10, 11],
  "default": {
    "values": {
      "rpm": 0,
      "power": 40
    },
    "configs": {
      "mode": "auto"
    },
    "pins": {}
  },
  "picture": "placeholder:hybrid",
  "Pins": ["TACH", "PWM"]
}
```

## Writing Good Device Text

Use the catalog text as practical HMI copy:

- `type`: short, recognizable label, usually 2 to 5 words.
- `description`: one clear sentence about what the device measures or controls.
- Value keys: stable protocol-friendly identifiers such as `temp`, `humi`, `set_point`, `rpm`.
- Units: short labels such as `C`, `%`, `lux`, `hPa`, `rpm`.
- Pin labels: logical names such as `DQ`, `AOUT`, `PWM`, `SENSE`, not long explanations.

Avoid:

- long tutorial text in `description`
- punctuation-heavy value keys
- spaces in protocol keys
- semicolons or newlines in values that may be recorded to CSV
- changing existing `uid` values once users may have recordings or emulator code tied to them

## Choosing Role and Access

Use this decision table:

| Need | Role | Parameter placement |
| --- | --- | --- |
| Device only reports values | `sensor` | `values` with missing/`read` access. |
| Device only accepts runtime outputs | `actuator` | `values` with `write` access. |
| Device reports values and accepts outputs | `hybrid` | Mix `read` and `write` values. |
| Device has setup knobs | Any role | `configs`. |

Examples:

- Temperature sensor: `sensor`, `temp` as read value.
- PWM LED: `actuator`, `Brightness` as write value, `Enabled` as config.
- Temperature regulator: `hybrid`, `temp` as read value, `set_point` as write value, `speed` as config.

## Restrictions and Editors

Restrictions help the UI choose safe editors and validate input:

| Restriction | Use |
| --- | --- |
| `min` and `max` | Numeric bounds for values/configs. |
| `step` | Slider or stepper increment. |
| `options` | Dropdown/select-like choices. |

For numeric values, set `dtype` to `int`, `float`, or `double`. For option values, use `string` plus `restrictions.options`.

## Images

Put real image assets in [`storage/data/pics/`](../../storage/data/pics/) and reference them by filename:

```json
"picture": "DS18B20.png"
```

Then run:

```bash
python storage/sync_db.py
```

Guidelines:

- Prefer small PNG files that are readable on the 7 inch display.
- Use transparent or clean backgrounds where possible.
- Keep filenames ASCII-friendly and without spaces.
- Use `placeholder:device` until a real image exists.

## Emulator Expectations

The emulator loads the same catalog data and uses it to decide:

- which UIDs exist
- which values are readable through `UPDATE`
- which values are writable through `CONTROL`
- which configs can be stored through `CONFIG`
- which default values to use

If a device needs special simulated behavior, add it in [`emulator/engine/emulator.py`](../../emulator/engine/emulator.py) and, if relevant, [`emulator/engine/emulator_patterns.py`](../../emulator/engine/emulator_patterns.py).

For a generic sensor, the emulator can often use catalog defaults without custom code.

## Sync and Validation Workflow

After editing wiki/catalog content:

```bash
python storage/sync_db.py
```

Recommended checks:

```bash
python -m json.tool storage/data/DB.json > /tmp/edubox-panel-db-check.json
python storage/sync_db.py --dry-run
```

Then run the emulator path you use for development:

```bash
python emulator/virt_runner.py
```

Manual HMI smoke test:

1. Boot the HMI or emulator UI.
2. Open the device library/selection flow.
3. Confirm the new device appears with the expected name, role, image, and pins.
4. Assign required pins.
5. Connect via VSCP.
6. For read values, confirm `UPDATE` returns data.
7. For write values, change the editor and confirm `CONTROL` is sent.
8. For configs, change the config editor and confirm `CONFIG` is sent.
9. Start and stop recording if the device has readable values.

## Compatibility Rules

- Keep existing `uid` values stable.
- Prefer adding new values/configs over renaming existing keys.
- If a protocol-facing key must change, update firmware, emulator, docs, and any course material together.
- Keep `DB.json` backward-readable when possible. Older devices may still carry older SD cards.
- Update [FORMATS.md](FORMATS.md) when adding schema fields.
- Update [PROTOCOL_EN.md](../PROTOCOL_EN.md) when changing command semantics.

## Review Checklist

Before merging a wiki/catalog change:

- Device `uid` is unique and stable.
- `type` and `description` are user-facing and concise.
- `Pins` is non-empty.
- `allowedPins`, if present, contains valid physical pins for the board.
- Every value/config has a sensible `dtype`.
- Writable values use `access: "write"`.
- Read values either omit `access` or use `access: "read"`.
- Defaults match declared keys.
- Restrictions match the declared data type.
- Picture reference resolves or uses a placeholder.
- `storage/sync_db.py` has regenerated mirrors.

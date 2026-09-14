# EduBox HUB Panel Developer Map

This document is an orientation map of the project for developers. Its purpose is not to replace detailed API documentation in headers, but to quickly explain the workflow, component ownership, data flows, and places to modify during typical changes.

## What the project does

EduBox HUB Panel is an LVGL HMI application for an ESP32-S3 7" display. The firmware displays devices defined in a JSON database, can connect selected devices to virtual pins, communicates with an upstream endpoint through the text-based VSCP protocol, and displays runtime values.

Simplified flow:

```text
ESP32 UI
  -> loads DB.json from storage
  -> builds the device catalog
  -> user selects communication, device, and pins
  -> firmware sends VSCP INIT / CONNECT
  -> in the runtime cycle, sends VSCP UPDATE
  -> GUI draws current values, charts, and DataBundle records
```

The upstream endpoint can be a Python emulator, EduBox HUB, or custom hardware implementing VSCP.

## Root directories

| Path | Purpose |
| --- | --- |
| `ui/` | Arduino sketch, hardware init, LVGL display/touch setup, main `setup()` and `loop()`. |
| `libraries/engine/` | Custom application engine: GUI, runtime manager, storage, device catalog, DataBundle. |
| `libraries/vscp/` | Shared VSCP codec, transport adapters, client, and server. |
| `libraries/ui/` | SquareLine/LVGL boot UI generated layer. The application GUI is mainly in `engine/src/gui`. |
| `libraries/expt/` | Logging, splash messages, and exception framework. |
| `libraries/lvgl/`, `libraries/LovyanGFX/`, `libraries/ArduinoJson/` | Vendored third-party libraries. |
| `storage/data/DB.json` | Canonical host/dev copy of the device catalog. |
| `ui/data/DB.json` | Catalog copy for the Arduino/firmware workflow. |
| `emulator/` | Python VSCP emulator, virtual and real sensor runners. |
| `docs/` | Installation, workflow notes, images, and wiki drafts. |
| `bin/` | Exported binaries. |

## Firmware entrypoint

The main sketch is `ui/ui.ino`.

Responsibilities:

- sets up the Elecrow/LovyanGFX RGB display,
- initializes the LVGL draw buffer,
- initializes the touch driver from `ui/touch.h`,
- creates global runtime objects:
  - `DeviceCatalog deviceCatalog`
  - `DeviceBrowserState deviceBrowserState`
  - `DeviceManager deviceManager`
  - `DeviceVisualizationSession deviceVisualizationSession`
  - `DataBundleManager dataBundleManager`
  - `GuiManager guiManager`
- calls `guiManager.init(DEVICE_DB_STORAGE_PATH)`,
- opens the Main Menu after boot,
- calls `guiManager.redraw()` in `loop()`.

`DEVICE_DB_STORAGE_PATH` is derived from `STORAGE_DEFAULT_DEVICE_DB_PATH` in `libraries/engine/src/config.hpp`.

## Build and storage configuration

The main configuration file is `libraries/engine/src/config.hpp`.

Important options:

- `ENABLE_DEBUG`: unified debug switch for the project, default `1`.
- `DEBUG_VERBOSE_LEVEL`: debug log detail level, default `2`.
  - `1`: errors and critical debug information.
  - `2`: errors, warnings, and important operations / process results.
  - `3`: everything, including detailed conversions and low-level debug traces.
- `STORAGE_OPTION_SPIFFS`: default dev mode, catalog is `/DB.json`.
- `STORAGE_OPTION_SD`: catalog is `/data/DB.json`.
- `STORAGE_SEED_DEFAULT_DB_ON_MISSING`: in SPIFFS mode, can create the default DB from an embedded string.
- `FILE_TRANSFER_USB_MSC_ENABLED`: enables the USB MSC bridge for Transfer Mode.
- `FILE_TRANSFER_USB_MSC_VENDOR_ID`, `FILE_TRANSFER_USB_MSC_PRODUCT_ID`, `FILE_TRANSFER_USB_MSC_REVISION`: USB identification strings for the MSC disk.

Debug logs use `debugLogMessage(source, reason, format, ...)` and have the form:

```text
DEBUG: <message> reason=<reason> source=<source>
```

Exceptions do not use the debug logging helper. They are caught in `catch` blocks and printed through `Exception::print()`, which recursively prints inner exceptions as well. This output is not tied to `ENABLE_DEBUG` and has the form:

```text
(<source>) EXCEPTION: <message>
```

The installation and build workflow is in `docs/INSTALL.md`.

Practical build:

1. Open `ui/ui.ino` in Arduino IDE.
2. Set the Sketchbook location to the project root.
3. Board: `ESP32S3 Dev Module`, esp32 core tested with `2.0.17`.
4. Compile or export binary.

## Runtime initialization

`GuiManager::init()` in `libraries/engine/src/gui/gui_manager.cpp` is the main application startup sequence:

1. Initializes the Crash GUI.
2. Mounts storage through `StorageManager`.
3. Initializes `DataBundleManager`.
4. Loads `DeviceCatalog` from the JSON DB.
5. Clears browse/session state.
6. Initializes `DeviceManager`.
7. Initializes core screens through `GuiScreenRegistry`.

If storage or catalog loading fails, the Crash screen is displayed.

## Device catalog and DB.json

The catalog is owned by `DeviceCatalog`:

- `libraries/engine/src/managers/device_catalog.hpp`
- `libraries/engine/src/managers/device_catalog.cpp`

The JSON parser and serializer are here:

- `libraries/engine/src/devices/json_device_builder.hpp`
- `libraries/engine/src/devices/json_device_builder.cpp`

Schema in `DB.json`:

```json
{
  "version": "1.0",
  "application": "board",
  "devices": {
    "cpu_temp": {
      "uid": "cpu_temp",
      "role": "sensor",
      "type": "CPU Temp",
      "description": "...",
      "values": {
        "temp": { "value": 0.0, "unit": "C", "dtype": "float" }
      },
      "configs": {},
      "default": {
        "values": { "temp": 0.0 },
        "pins": ""
      }
    }
  }
}
```

Supported `role` values:

- `sensor`: telemetry values; can be recorded and charted,
- `actuator`: runtime control values; not shown as a standard recording stream,
- `hybrid`: combination according to value/config definition.

Supported `dtype` values:

- `int`
- `float`
- `double`
- `string`

`values` are runtime values. Each value may have an `access` marker:

- `access: "read"` or missing `access`: value is read through `UPDATE`.
- `access: "write"`: value is written through `CONTROL`.

Alias values for the write direction are `out`, `output`, and `control`; for the read direction, `in` and `input`. The parser normalizes them to `read`/`write`.

`configs` are persistent/configuration values sent through `CONFIG`. A hybrid device may have read values, write values, and configs at the same time.

Example hybrid test device:

```json
"H00": {
  "uid": "H00",
  "role": "hybrid",
  "type": "Temperature Regulator",
  "values": {
    "set_point": { "value": 25, "unit": "C", "dtype": "int", "access": "write" },
    "temp": { "value": 20, "unit": "C", "dtype": "int", "access": "read" }
  },
  "configs": {
    "speed": { "value": 2, "dtype": "int", "restrictions": { "min": 1, "max": 5, "step": 1 } }
  }
}
```

## Device model

The basic runtime model is `BaseDevice`:

- `libraries/engine/src/devices/base_device.hpp`
- `libraries/engine/src/devices/base_device.cpp`

Important elements:

- `UID`, `Type`, `Description`, `Role`
- `Values` and `Configs`
- `DeviceParamAccess` for distinguishing `READ`/`WRITE` runtime values
- pin assignment (`assignPin`, `unassignPin`)
- sync flags:
  - `isValuesSync`
  - `isConfigsSync`
  - `isControlsSync`
  - `redrawPending`
- `requestRuntimeUpdate()` marks runtime values as stale so that the next sync sends `UPDATE`.
- `syncValues()`, `syncConfigs()`, and `syncControls()` call the VSCP protocol layer.
- `usesUpdateChannel()` returns true if the device has at least one readable value.
- `usesControlChannel()` returns true if the device has a writable value and is not a pure sensor.

The global helper `syncDevice(BaseDevice*)` wraps `device->synchronize()` into an exception-safe API.

## DeviceManager and pins

The runtime manager is:

- `libraries/engine/src/managers/device_manager.hpp`
- `libraries/engine/src/managers/device_manager.cpp`

Responsibilities:

- holds `PinMap` with `NUM_PINS = 18`,
- assigns/removes devices to/from pins,
- explicitly initializes the VSCP connection,
- during `connect()`:
  1. ensures `INIT`,
  2. disconnects previously assigned devices,
  3. writes selected pins into `BaseDevice`,
  4. sends `CONNECT` for each assigned device,
- during runtime `resync(device)` in running mode:
  1. calls `device->requestRuntimeUpdate()`,
  2. calls `deviceManager.sync(device)`.

The Pin UI works with virtual pins. The pin state is defined by:

- `libraries/engine/src/managers/pin_structure.hpp`

## VSCP protocol

The firmware implementation of VSCP is in `libraries/vscp`.

Main classes/files:

- `libraries/vscp/src/vscp_types.*`
- `libraries/vscp/src/vscp_codec.*`
- `libraries/vscp/src/vscp_client.*`
- `libraries/vscp/src/vscp_server.*`
- `libraries/vscp/src/io/vscp_transport.*`
- `libraries/vscp/src/io/vscp_stream_transport.*`

The current API version is in `vscp::API_VERSION`, currently `1.4`.

The wire format is a URL-like query string:

```text
?type=INIT&app=board&db=1.0&api=1.4
?status=1

?type=CONNECT&id=cpu_temp&pins=1,4
?id=cpu_temp&status=1

?type=UPDATE&id=cpu_temp
?id=cpu_temp&status=1&temp=23.4
```

Supported request types:

- `INIT`: app/db/api handshake.
- `UPDATE`: reads runtime values.
- `CONFIG`: writes configuration.
- `CONTROL`: writes runtime control values for actuator/hybrid writable values.
- `RESET`: resets a device.
- `CONNECT`: connects a device to pins.
- `DISCONNECT`: disconnects a device.

The sketch owns `HardwareSerial vscpSerial`, wraps it in `vscp::StreamTransport`, and injects one `vscp::Client` into `DeviceManager`. Board-specific UART defaults are in `libraries/engine/src/config.hpp`.

## GUI architecture

The GUI layer is in `libraries/engine/src/gui`.

Main components:

| File | Purpose |
| --- | --- |
| `gui_manager.*` | Central router/state manager. Called from `ui.ino`. |
| `gui_state.hpp` | Enum of all screens. |
| `gui_router.hpp` | Interface used by screens for navigation. |
| `gui_navigation_policy.*` | Decides where back/flow actions lead. |
| `gui_runtime_policy.*` | Enables/disables runtime polling depending on the screen. |
| `gui_screen_registry.*` | Owns screen instances, initializes/hides/renders them. |
| `main_menu_gui.*` | Main menu. |
| `communication_selection_gui.*` | Communication selection; Cable performs VSCP INIT, wireless is currently disabled. |
| `device_selection_gui.*` | Device selection and pin assignment. |
| `signals_visualization_gui.*` | Runtime visualization, chart, recording, settings overlay. |
| `signals_*_panel.*` | Smaller panels for the runtime screen: chart, value list, toolbar, settings, feedback. |
| `data_bundle_selection_gui.*` | Databank/DataBundle overview. |
| `file_transfer_gui.*` | Transfer Mode screen for exposing the SD card to a computer through USB MSC. |
| `library_gui.*`, `library_editor_gui.*` | Device catalog editing. |
| `crash_gui.*` | Fatal error screen. |
| `credits_gui.hpp` | Credits. |

### Navigation flow

Main runtime flow:

```text
MAIN_MENU
  -> COMMUNICATION_SELECTION
  -> SELECTION
  -> CONNECTION
  -> VISUALIZATION
```

Notes:

- `GuiNavigationPolicy::beginVisualizationFlow()` returns `COMMUNICATION_SELECTION`.
- `COMMUNICATION_SELECTION` currently supports only Cable.
- After Cable selection, VSCP `INIT` is performed.
- `SELECTION` is used for device selection.
- `CONNECTION` is used for pin assignment.
- `VISUALIZATION` starts runtime polling.

Databank:

```text
MAIN_MENU -> DATA_BUNDLE_SELECTION -> MAIN_MENU
VISUALIZATION -> DATA_BUNDLE_SELECTION -> VISUALIZATION
```

Return behavior is decided by `databankReturnToVisualization` in `GuiNavigationPolicy`.

File Transfer:

```text
MAIN_MENU -> FILE_TRANSFER -> MAIN_MENU
DATA_BUNDLE_SELECTION export -> FILE_TRANSFER -> MAIN_MENU
```

`FILE_TRANSFER` has its own user-action layer. Opening the screen only displays
the white preparation screen. The transfer pipeline starts only after `OK`.
During an active session, `Back` is disabled and the session must be stopped with
the red `End session` button, which stops USB MSC, remounts SD, and returns to
the Main Menu.

### Runtime loop and redraw

`GuiManager::redraw()`:

1. calls `lv_timer_handler()`,
2. approximately every 100 ms calls `deviceManager.resync(currentDevice)`,
3. if the state is `VISUALIZATION`, calls `SignalsVisualizationGui::drawCurrentDevice()`.

`SignalsVisualizationGui` draws only when `currentDevice->getRedrawPending()` is true. The runtime chart keeps a longer internal history and displays a window of size `HISTORY_CAP`. Dragging on the chart shifts the displayed history window. Autoscale is computed for the current window.

## Runtime visualization

Key files:

- `signals_visualization_gui.*`: orchestration of the whole screen.
- `signals_chart_panel.*`: LVGL chart helper.
- `signals_list_panel.*`: list of live values and editors.
- `signals_toolbar_panel.*`: pause/update/record/clear/settings.
- `signals_settings_panel.*`: overlay for Databank, clear bundles, credits.
- `signals_feedback_panel.*`: alerts and confirmation dialogs.

Behavior:

- Normal running: the GUI manager periodically calls `DeviceManager::resync()`, which sends `UPDATE`.
- Pause: stops the running flag. The manual update button sends a single `UPDATE`.
- Values/Configs panel:
  - the runtime screen has a switch between `Values` and `Configs`,
  - read `Values` are live cards,
  - write `Values` are editors sent through `CONTROL` and have an orange background,
  - `Configs` are editors sent through `CONFIG` and have a blue background.
- Chart:
  - float/double values are scaled `x100` for the LVGL integer chart,
  - the chart has a label showing the current scaling (`Scaling 1x`, `Scaling x100`),
  - chart writes use `lv_chart_set_value_by_id`,
  - range is recalculated according to the current window.
- Record:
  - start calls `DataBundleManager::startRecording(device->Type, device->UID)`,
  - new points are stored when runtime samples arrive,
  - stop calls `DataBundleManager::saveRecording()`.

## DataBundle and Databank

Manager:

- `libraries/engine/src/managers/data_bundle_manager.hpp`
- `libraries/engine/src/managers/data_bundle_manager.cpp`
- `libraries/engine/src/managers/data_bundle_types.hpp`

GUI:

- `libraries/engine/src/gui/data_bundle_selection_gui.*`

Storage:

- Bundles are stored under `/records/`.
- CSV format:

```csv
DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value
CPU Temp;cpu_temp;0;0;temp;23.4
CPU Temp;cpu_temp;1;105;temp;23.5
```

Lifecycle:

1. `startRecording(deviceName, deviceUid)` prepares a unique file path.
2. `saveNewDataPoint(signalName, value)` buffers points in RAM.
3. `saveRecording()` writes CSV, refreshes the file list, and clears the active recording.
4. `getBundlePage(page)` refreshes the list and returns 6 preview records.
5. `DataBundleSelectionGui` displays cards and preview charts.

Notes:

- Recording has no purpose without an incoming `UPDATE`; an empty recording is not saved.
- `RuntimeMs` is relative time from the start of recording, so it does not require RTC or NTP.
- The preview chart scales decimal values `x100`, just like the runtime chart.

## Persistent storage

Storage abstraction:

- `libraries/engine/src/managers/storage_manager.hpp`
- `libraries/engine/src/managers/storage_manager.cpp`

Supports:

- SPIFFS
- SD

API:

- `storageManager().init()`
- `exists(path)`
- `ensureDirectory(path)`
- `open(path, mode)`
- `remove(path)`
- `listFiles(directory)`
- `totalBytes()`, `usedBytes()`

SPIFFS nuance:

- it does not have real directories like SD; `ensureDirectory()` returns true.
- `listFiles("/records/")` scans root and filters by prefix.

SD layout:

- `/data/DB.json`: device catalog.
- `/data/pics/`: device images.
- `/data/config.json`: persistent application configuration.
- `/records/`: DataBundle CSV records.

## File Transfer / USB MSC

Transfer Mode exposes the SD card to the computer as a USB Mass Storage Class
disk. SPIFFS is never exposed; it remains an internal demo/debug storage mode.

Key files:

- `libraries/engine/src/gui/file_transfer_gui.*`: preparation screen, state
  colors, `OK` session start, and `End session`.
- `libraries/engine/src/managers/file_transfer_service.*`: transfer state
  machine, SD lock/unlock, and bridge integration.
- `libraries/engine/src/managers/file_transfer_usb_msc_bridge.*`: Arduino
  `USBMSC` backend, raw `onRead`/`onWrite` callbacks over SD sectors.
- `libraries/engine/src/managers/storage_manager.*`: `enterTransferLock()` and
  `exitTransferLock()`.
- `libraries/expt/src/logs/logs.*`: USB CDC aware logging; during MSC session,
  `logMessage()` does not open `Serial` and buffers messages in RAM.

Lifecycle:

1. User opens the Transfer Screen.
2. The screen shows preparation info; SD and USB MSC are not touched yet.
3. After `OK`, the yellow `Connection...` state is drawn and `lv_timer_handler()`
   + `lv_refr_now(nullptr)` are forced.
4. `FileTransferService::start()` checks SD, sets `transferModeActive`, disables
   CDC log output, and calls `storageManager().enterTransferLock()`.
5. Storage manager unmounts SD from the HMI filesystem.
6. `FileTransferUsbMscBridge::start()` remounts SD for raw block access, sets
   `USBMSC` callbacks, calls `usbMsc.begin(blockCount, 512)` and `USB.begin()`.
7. The PC sees the HMI as a Mass Storage disk.
8. `End session` calls `FileTransferService::stop()`: `usbMsc.mediaPresent(false)`,
   `usbMsc.end()`, SD unmount from the bridge, `storageManager().exitTransferLock()`,
   CDC log output restore, and buffered log flush.

The backend is active only when the build meets these conditions:

- `FILE_TRANSFER_USB_MSC_ENABLED == 1`.
- Arduino ESP32 core provides `USB.h` and `USBMSC.h`.
- Board is in ESP32-S3 native USB OTG/TinyUSB mode (`ARDUINO_USB_MODE == 0`).
- USB CDC on boot should remain enabled for the serial/log channel over the same
  connector.

If the conditions are not met, the bridge returns a readable error and the
Transfer Screen switches to the red error state without pretending to be a USB
disk.

## Emulator

The Python emulator is in `emulator/`.

Main files:

| File | Purpose |
| --- | --- |
| `emulator/engine/emulator.py` | VSCP endpoint, parser, INIT/UPDATE/CONFIG/CONTROL/CONNECT/DISCONNECT. |
| `emulator/virt_runner.py` | Virtual serial/emulator runner. |
| `emulator/real_runner.py` | Runner for real sensor extensions. |
| `emulator/virt_patterns_runner.py` | Pattern/simulation runner. |
| `emulator/engine/emulator_patterns.py` | Simulation patterns. |
| `emulator/engine/sensors/` | Sensor modules for real/virtual sources. |
| `emulator/engine/tests/` | Python emulator tests. |

The emulator loads device defaults from:

1. `data/DB.json` if a legacy root copy exists
2. fallback `ui/data/DB.json`
3. fallback hardcoded catalog

The emulator respects `access` on values:

- `UPDATE` returns only readable values.
- `CONTROL` accepts only writable values.
- `CONFIG` stores config parameters for later simulation.

The special test device `H00` / `Temperature Regulator` simulates a real hybrid flow:

```text
?type=UPDATE&id=H00
?id=H00&status=1&temp=22

?type=CONTROL&id=H00&set_point=35
?id=H00&status=1

?type=CONFIG&id=H00&speed=5
?id=H00&status=1
```

On subsequent `UPDATE` calls, `temp` gradually approaches `set_point`. `speed` in the range `1..5` determines the step in degrees per update. The same behavior is implemented in `emulator/engine/emulator.py` and `emulator/engine/emulator_patterns.py`.

Installation:

```bash
pip install -r emulator/requirements.txt
```

Typical run:

```bash
python emulator/virt_runner.py
python emulator/real_runner.py
python emulator/virt_patterns_runner.py
```

Example VSCP console/log output:

```text
Received: ?type=INIT&app=board&db=1.0&api=1.4
Sent: ?status=1
Received: ?type=CONNECT&id=cpu_temp&pins=1,4
Sent: ?id=cpu_temp&status=1
Received: ?type=UPDATE&id=cpu_temp
Sent: ?id=cpu_temp&status=1&temp=0.21
```

## Typical development scenarios

### Add a new device

1. Edit `storage/data/DB.json`.
2. Run `python storage/sync_db.py` to update `ui/data/DB.json`, `ui/data/pics`, and the embedded fallback header.
3. Add a device entry with `uid`, `role`, `type`, `values`, `configs`, and `default`.
4. For write values, set `access: "write"`; otherwise they remain read/UPDATE values.
5. For hybrid devices, verify that readable values, writable values, and configs make sense for the runtime UI.
6. If the emulator does not have a special sensor, default values are loaded automatically from DB.
7. Test selection, connection, and visualization.

### Add a new runtime value/control editor

- Values are defined in DB as `values`.
- Configurations are in DB as `configs`.
- Read values are rendered as live cards; write values are rendered as CONTROL editors.
- The runtime panel can be switched between `Values` and `Configs`.
- UI editor logic is in `SignalsVisualizationGui`:
  - `ensureControlEditor`
  - `syncControlEditorValue`
  - `handleDropdownConfigChanged`
  - `handleSliderConfigChanged`
  - `handleTextConfigSubmitted`

### Modify the VSCP protocol

1. Edit `libraries/vscp/src/vscp_types.*` and `libraries/vscp/src/vscp_codec.*`.
2. Update `VSCP_API_VERSION` if the wire contract changes.
3. Edit `emulator/engine/emulator.py` so it mirrors the firmware protocol.
4. Edit `DeviceManager` or `BaseDevice` if sync semantics change.
5. Test INIT, CONNECT, UPDATE, CONFIG, CONTROL, and DISCONNECT.

### Modify pin assignment

- UI: `device_selection_gui.*`, `connection_gui`/connection screen according to the specific flow.
- Runtime state: `DeviceManager::PinMap`.
- Pin rules: `VirtualPin` in `pin_structure.hpp`.
- Protocol connection: `DeviceManager::connect()`.

### Modify the runtime chart

- `signals_visualization_gui.*`: history, autoscale, pan/drag, chartable key selection.
- `signals_chart_panel.*`: LVGL chart setup and series.
- Watch out for float/double: the LVGL chart uses integer `lv_coord_t`; the project scales decimal values `x100`.

### Modify DataBundle

- Recording flow: `SignalsVisualizationGui::handleRecordButtonClick`.
- Buffer/write/read: `DataBundleManager`.
- Databank UI: `DataBundleSelectionGui`.
- CSV schema: `DeviceName;DeviceUid;SampleIndex;RuntimeMs;SignalName;Value`.

### Modify the storage backend

- Config: `libraries/engine/src/config.hpp`.
- Abstraction: `StorageManager`.
- DB path:
  - SPIFFS: `/DB.json`
  - SD: `/data/DB.json`
- DataBundle path: `/records/`.
- USB MSC transfer: `FileTransferService` + `FileTransferUsbMscBridge`; SD must
  be locked through `StorageManager::enterTransferLock()` before being exposed
  to the computer.

### Modify File Transfer / USB MSC

- UI flow: `file_transfer_gui.*`.
- Transfer lifecycle: `FileTransferService`.
- Raw block backend: `FileTransferUsbMscBridge`.
- Never write to SD through HMI storage while an MSC session is active.
- Logging during transfer must remain CDC-safe: use `setLoggerUsbCdcAvailable(false)`
  and then `flushBufferedLogMessages()` after returning.
- Board/build settings are as important as code: native USB OTG/TinyUSB,
  `USBMSC.h`, `USB.h`, USB CDC on boot.

## Debugging checklist

### Connection fails

- Verify that the Communication screen selected Cable.
- Verify that the upstream endpoint responds to:

```text
?type=INIT&app=board&db=1.0&api=1.4
```

- Firmware path: `CommunicationSelectionGui -> DeviceManager::initializeProtocolConnection -> vscp::Client::init`.

### Pin assignment fails

- Check `DeviceManager::PinMap`.
- Check whether the pin is locked or occupied.
- After confirmation, `DeviceManager::connect()` should send `DISCONNECT`/`CONNECT`.

### Runtime does not send UPDATE

- `GuiRuntimePolicy` must be in the `VISUALIZATION` state.
- `DeviceManager::isRunning()` must be true.
- `GuiManager::redraw()` calls `deviceManager.resync(currentDevice)` after the interval.
- `resync()` must call `requestRuntimeUpdate()`.

### Chart does not redraw

- Check `redrawPending`.
- `BaseDevice::syncValues()` sets `redrawPending` after a successful UPDATE.
- `SignalsVisualizationGui::drawCurrentDevice()` clears the flag after drawing.
- Float values must be scaled into `lv_coord_t`.

### Record does not create a bundle

- At least one runtime sample must arrive between Start and Stop Record.
- `DataBundleManager::saveNewDataPoint()` is called when a runtime sample is appended.
- `saveRecording()` rejects an empty recording.
- Databank refreshes files from storage during `getBundlePage()`.

### Databank shows nothing

- Verify storage mount.
- Verify `/records/`.
- Verify CSV files through `DataBundleManager::listAllBundles()`.
- Verify that `DataBundleSelectionGui::showDataBundles()` calls `updateBundles()`.

### Transfer Mode/USB MSC does not start

- Verify that the build uses `STORAGE_OPTION_SD`; SPIFFS is not exposed through MSC.
- Verify that the SD card is physically inserted and mounts outside transfer.
- Verify Arduino board option: ESP32-S3 native USB OTG/TinyUSB (`ARDUINO_USB_MODE == 0`).
- Verify that the core provides `USB.h` and `USBMSC.h`.
- If the PC does not see a disk, check the USB cable/port: it must be the native
  ESP32-S3 USB connector, not only an external USB-UART bridge.
- After `End session`, the log should show SD remount from `StorageManager::exitTransferLock()`.

## Code conventions and risks

- LVGL event callbacks should read user data from `lv_event_get_current_target(e)` if the data belongs to a specific UI control.
- Do not modify third-party `libraries/lvgl`, `LovyanGFX`, or `ArduinoJson` unless necessary.
- `DeviceCatalog` owns `BaseDevice*`; when the catalog is rebuilt, old instances are deleted.
- `DeviceVisualizationSession` holds the active runtime selection; when editing the catalog, the session must be cleared.
- The Storage API requires absolute paths with `/`.
- SPIFFS and SD behave differently for directories; use `StorageManager`, not direct `SPIFFS`/`SD`.
- During Transfer Mode, do not perform HMI filesystem operations on SD; the card
  is owned by the USB host.
- The VSCP parser is case-sensitive according to config; the emulator should keep the same semantics as the firmware.

## Quick component index

```text
ui/ui.ino
  hardware init, LVGL init, global managers, main loop

libraries/engine/src/config.hpp
  engine config, storage backend, debug flags

libraries/engine/src/engine.hpp
  umbrella include for the firmware sketch

libraries/engine/src/devices/
  BaseDevice, JSON DB parser/builder, embedded default DB

libraries/engine/src/managers/
  DeviceCatalog, DeviceManager, DeviceVisualizationSession,
  DeviceBrowserState, DataBundleManager, StorageManager,
  FileTransferService, FileTransferUsbMscBridge

libraries/engine/src/gui/
  all application screens and LVGL panel helpers

libraries/vscp/src/
  shared VSCP codec, client/server, and transport adapters

libraries/expt/src/
  logging, exceptions, splash messages

emulator/
  Python VSCP emulator and test runners

storage/data/DB.json, ui/data/DB.json
  canonical device catalog and firmware mirror
```

## Document status

This file is intended as a practical map. For larger architectural changes, update:

- navigation flow,
- VSCP command list/API version,
- DB schema,
- DataBundle format,
- emulator workflow,
- storage backend notes.

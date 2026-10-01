# EduBox HUB Panel

![Logo EduBox HUB Panel](assets/logo.svg)

[![Build Firmware](https://github.com/sgtkingo/EduBox-HUB-Panel/actions/workflows/build.yml/badge.svg)](https://github.com/sgtkingo/EduBox-HUB-Panel/actions/workflows/build.yml)
[![Chip: ESP32-S3](https://img.shields.io/badge/chip-ESP32--S3-ef4444)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Platform: ESP32](https://img.shields.io/badge/platform-ESP32-0f766e)](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
[![Language: C++](https://img.shields.io/badge/language-C%2B%2B-00599C)](https://isocpp.org/)
[![HMI: 7 inch RGB TFT Touch](https://img.shields.io/badge/HMI-7%22%20RGB%20TFT%20Touch-7c3aed)](docs/INSTALL.md)
[![GUI: LVGL](https://img.shields.io/badge/GUI-LVGL-00a0b0)](https://lvgl.io/)

**Current development version:** `latest`

**EduBox HUB Panel** is the touch HMI branch of the [EduBox HUB](https://github.com/sgtkingo/EduBox-HUB) ecosystem and a lightweight visualization client for exploring sensor data in real time 🖥️.
It focuses on **raw vs. processed comparisons**, **interactive inspection**, and **data capture** for later analysis
— while educational modules live *above* this layer (e.g., in EduBox HUB or external course content). The Panel is developed as part of the [MTA](https://www.m-ta.cz) EduBox HUB project.

## 🧩 Target Hardware

![EduBox HUB Panel – Use-case](docs/img/panel_apps.png)

EduBox HUB Panel is designed and tested for the **Elecrow ESP32 Display 7" HMI (ESP32-S3 + RGB TFT + Touch, LVGL-ready)**:

- 🔗 [Distributor / purchase link](https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html)

---

## 🚀 Core Capabilities

### 🔎 Sensor Data Exploration
- 📊 Real-time visualization of **raw/real values** and derived/processed values (side-by-side comparison).
- 👀 Interactive browsing of streams and channels for quick inspection and debugging.
- 🌐 Designed as a generic viewer: **theoretically supports any sensor** as long as the upstream platform can provide it.

### 💾 Logging & Export
- 💽 Recording into **DataBundle** for structured capture sessions.
- 📝 Export to **CSV on SD card** for offline analysis (Python/Excel/Matlab workflows).

### 📚 Sensor Knowledge Base (Wiki)
- 📖 Built-in **Wiki** of sensors: quick reference for principles, typical ranges, pitfalls, and usage notes.
- ➕ Extensible content model (add new sensors without coupling to firmware logic).

---

## 🔗 Connectivity Model

EduBox HUB Panel does not read sensors directly in the general case.
It requires an upstream source that speaks our open protocol:

### 🔌 Supported Upstream Sources
- 🖥️ **PC connection (emulator / host tooling)**
- 🧩 **EduBox HUB** platform
- 🔧 **Custom hardware** implementing the same protocol

### 📡 Protocol: VSCP (Virtual Sensors Communication Protocol)
Communication is done via a **text-based, REST-like protocol**:

- 📚 Spec / reference repo: https://github.com/sgtkingo/EduBox-HUB-VSCP

---

## ⚙️ Configuration & I/O (Bidirectional)

Beyond passive viewing, EduBox HUB Panel supports operational control via the upstream platform:

- 🔌 **Dynamic pin mapping**: set which physical pins a real sensor is connected to (runtime configuration).
- 🔁 **Bidirectional messaging**:
  - 📈 sensor reads (telemetry / streaming),
  - 🎚️ actuator control (sending values/commands),
  - 🛠️ configuration pushes (calibration, sampling, modes).

Runtime values can be explicitly marked as **read** or **write** in `DB.json`:

- `access: "read"` values are fetched through `UPDATE` and can be charted/recorded.
- `access: "write"` values are sent through `CONTROL` and shown as editable orange controls.
- `configs` are sent through `CONFIG` and shown separately from values in the runtime panel.

This enables true hybrid devices that combine telemetry, control and configuration in one device entry.
The bundled test catalog includes `H00` / **Temperature Regulator**:

- `temp`: read value returned by `UPDATE`,
- `set_point`: write value sent by `CONTROL`,
- `speed`: config value sent by `CONFIG`, controlling how quickly the emulated temperature reaches the set point.

---

## 📖 Documentation

- 🧩 [Installation & deployment](docs/INSTALL.md) — toolchain, flashing, SD layout, emulator wiring.
- 📡 [Protocol reference CZ](docs/PROTOCOL_CZ.md) / [EN](docs/PROTOCOL_EN.md) — VSCP message model and examples.
- 🧭 [Developer map CZ](docs/dev/DEV_MAP_CZ.md) / [EN](docs/dev/DEV_MAP_EN.md) — architecture, VSCP flow, DB schema, emulator notes.
- 🧱 [Architecture notes](docs/wiki/ARCHITECTURE.md) — dataflow & message types.
- 🗃️ [Data formats](docs/wiki/FORMATS.md) — DataBundle + CSV schema.
- 📝 [Wiki guide](docs/wiki/WIKI_GUIDE.md) — how to add/edit sensor Wiki entries.
- 🚢 [Release workflow rules](docs/dev/RELEASE_WORKFLOW_RULES.md) and [release notes](RELEASE_NOTES.md).
- 📄 [License](LICENCE).

---

## 🧭 Typical Workflow

1. 🔗 Connect EduBox HUB Panel to **PC Emulator** or **EduBox HUB** (or a VSCP-capable custom device).
2. 🔎 Select a sensor/channel and inspect **raw vs. processed** outputs.
3. 💾 Record a session into **DataBundle** and export **CSV to SD**.
4. 📊 Analyze captured data offline (Python/Excel/Matlab).

---

## 🔄 Recommended: Automatic Firmware Updates (Firmupdater)

EduBox HUB Panel is designed for **frequent deployments** (labs, classrooms, hotfix builds), so **automatic firmware updates are strongly recommended** via **Firmupdater**: https://github.com/sgtkingo/EduBox-HUB-Panel-Firmupdater.

Firmupdater automatically checks for newer versions and applies updates **with minimal manual intervention**—perfect for managing multiple devices in parallel. 🚀

📖 Learn more: https://github.com/sgtkingo/EduBox-HUB-Panel-Firmupdater
🌐 Online live-app: https://sgtkingo.github.io/Firmupdater/

---

## Current UI and Storage Notes

- Settings > Appearance > **Use devices pictures** enables storage-backed device pictures by default. Save persists `appearance.useDevicePictures` in `/data/config.json`; older configs without the key default to `true`. Disabling it uses built-in placeholders in the catalog, device selection and editor previews without loading picture files. The default is mirrored into both JSON artifacts and the embedded firmware fallback by `storage/sync_config.py`.
- Visualization **Settings > Update period** adjusts live polling from **10 to 1000 ms**, with a **100 ms** default. Changes apply immediately and remain selected while navigating between devices/screens until reboot. Device response time and rendering load can make the actual interval longer.
- Runtime and DataBundle graphs support two visible signals with separate Y axes, autoscale labels, cursor readout and adjustable visible sample count.
- After a successful INIT, the Panel runtime session checks the link with API 1.5 PING every 3 seconds, on Select device and while online visualization is paused/stopped. Automatic PING is suspended during online Run, where UPDATE/CONFIG/CONTROL failures monitor the session instead. Returning to Pause starts a fresh 3-second PING interval. `ALLOW_PING_INTERRUPT` in `libraries/engine/src/config.hpp` enables this automatic watchdog by default; set it to `0` to disable its timer and outgoing auto-pings while retaining the runtime request safeguards. A failed probe triggers five further attempts spaced 100 ms after each completed attempt (the client's response timeout is 500 ms). Six failed probes stop polling and show DISCONNECT. Returning to Home sends one-way BYE and ends the local session/watchdog even if the UART write fails; the next successful INIT re-arms monitoring. Intentional closure does not show DISCONNECT. The system timer only schedules work; the main loop owns all UART exchanges.
- Five consecutive failed UPDATE/CONFIG/CONTROL requests remain a separate safeguard for a lost device session, which PING alone cannot detect after a Board restart. A successful runtime response resets this counter. The full-width red DISCONNECT panel is managed globally across screens and appears regardless of debug settings; Reconnect performs INIT and restores connected devices with CONNECT before resuming (INIT alone is sufficient when no device is connected yet), and a failed reconnect displays an expt splash message. History and the current recording are retained. Both emulators support API 1.5 PING with side/sequence validation.
- Tiny and very large float ranges are shown with compact axis scaling labels, so micro-scale and high-value signals stay readable.
- Visualization Settings offers Auto/Manual Y scaling. In Manual mode, Manual Scale opens a range slider for minimum and maximum Y (signal values shared by both plotted series). Apply commits the limits; Cancel leaves them unchanged. Y finer/wider adjusts slider precision and reach. X zoom and history panning continue to work by gesture in both modes. Manual Y limits survive mode changes until another device is selected; they are not saved across reboot. Opening online Settings pauses polling and drawing; closing Settings resumes Run. Click either enlarged line-color swatch to cycle the same palette as the DataBundle viewer.
- Transfer Mode exposes SD-card data through the USB transfer workflow. After closing an active transfer session, the HMI can prompt for restart so changed files are reloaded cleanly.
- The Main Screen shows `v<version>` in the lower-left corner and `Storage: SD` or `Storage: SPIFFS` in the lower-right corner.

---

## Versioning

Project builds use `MAJOR.MINOR.PATCH.BUILD`.

- Current development build: `check latest`.
- `MAJOR` stays at `1` for this product line.
- `MINOR` was bumped for the refactoring branch because it adds major runtime, DataBundle, device catalog, settings, transfer-mode and protocol-facing functionality.
- `BUILD` is the number of commits on the current development branch since `main` (`63` at the time of this version bump).

---

## 🗂️ Repository Structure (Recommended)

- 🖼️ `ui/` — firmware + UI logic + .INO file (LVGL app)
- 📚 `libraries` — all headers and libraries (engine) 
- 🐍 `emulator` — Python-based emulator for testing
- 📄 `docs/` — diagrams, screenshots, Wiki sources, installation instruction
- 📦 `bin` — exported binary files
- 🧾 `data` — data files files (configurations, CSV)
- 📝 [`RELEASE_NOTES.md`](RELEASE_NOTES.md) — latest release notes
- 📄 [`LICENCE`](LICENCE) — MIT

---

## 🐞 Troubleshooting 

- ❌ If you can connect but see no data: verify the upstream device speaks **VSCP** and is streaming the expected channels.
- 💽 If SD export fails: check card formatting and required folder structure (see [`docs/INSTALL.md`](docs/INSTALL.md)).

---

## 🤝 Contributing

Contributions are welcome, especially:
- 📈 additional visualizations (plots, trend views, event markers),
- 🧱 DataBundle/CSV improvements and schema stability,
- 📖 new sensor Wiki pages.

📨 Please include: device/source type (PC/EduBox HUB/custom), VSCP message example, and expected output.

---

## Bluetooth bridge

The Panel can communicate with an EduBox Board over secure Bluetooth LE. A Board
and Panel are paired once; the Panel then remembers the authenticated Board and
reconnects to it automatically in later sessions. In a classroom with several
Boards and Panels, always compare the **Board ID shown in the Panel with the ID
printed on the Board label**.

### Recommended: automatic pairing with a cable

![Default automatic Bluetooth pairing: Panel connected by commissioning cable to Board](docs/img/bluetooth_cable_pairing.svg)

This is the default and recommended commissioning method. The cable is used only
to identify and authorize the correct Board. Normal communication switches to
Bluetooth after pairing, so the cable can be removed when the success screen is
shown.

1. Power on the **Panel** and the **Board**.
2. On the Panel, open **Communication** and tap **Wireless (BLE)**.
3. If the Panel has no remembered Board, it asks for a cable connection. Connect
   the Board directly to the Panel with the UART commissioning cable.
4. Wait while the Panel reads the Board ID and PIN, scans for that exact Board,
   and completes secure Bluetooth pairing. No PIN entry is required.
5. When **Success!** appears, disconnect the commissioning cable and tap
   **Continue**.

The Panel remembers the Board. On subsequent visits, tapping **Wireless (BLE)**
starts the Bluetooth connection automatically and shows a progress indicator.

If the connected Board is already paired with a different Panel, choose
**Forget Board & replace pairing** when prompted. Keep the cable connected while
the previous pairing is removed from both devices.

### Manual pairing with Board ID and PIN

Use manual pairing when a commissioning cable is not available. The Board must
be unpaired and advertising within its pairing window. The exact **Board ID** and
six-digit **PIN** are printed on the Board label; they are also written to the
Board UART log at startup.

1. Power on the Board and keep it close to the Panel. Restart an unpaired Board
   if its pairing window has expired.
2. On the Panel, open **Communication**.
3. Tap the small **settings (gear)** button next to **Wireless (BLE)**.
4. Wait for discovery to finish. If necessary, tap **Scan** again.
5. Open the Board dropdown and select the entry whose Board ID exactly matches
   the physical label.
6. Tap the green **Connect** button.
7. Enter the six-digit PIN in the floating PIN dialog and confirm it with the
   keyboard's confirmation key.
8. Wait for **Success!**, then tap **Continue**.

When a pairing is already stored, **Connect** is disabled and **Forget pairing**
is enabled. With no stored pairing, the states are reversed. After discovery,
the remembered Board is selected automatically when it is present.

### Forget or replace a pairing

Pairing information is stored on both the Panel and the Board. It must therefore
be removed from both devices; deleting only the Panel record would leave the
Board locked to the previous peer.

1. Open **Communication**, then tap the **settings (gear)** button next to
   **Wireless (BLE)**.
2. Connect the paired Board directly to the Panel with the UART commissioning
   cable and make sure the Board is powered.
3. Tap the red **Forget pairing** button in the lower-right corner.
4. Wait until the Panel confirms that pairing was removed from both Board and
   Panel.
5. Return to BLE Settings to scan and pair again, or close the screen.

### Bluetooth troubleshooting

| Message or symptom | What to check |
| --- | --- |
| `No PAIR response in 5 seconds` | Check Board power and the direct UART commissioning connection, including TX, RX and GND. Reconnect the cable and tap **Retry**. |
| `This Board is already paired` | Keep the cable connected and choose **Forget Board & replace pairing**, or cancel if the existing pairing should remain. |
| Board ID is not found during scanning | Confirm that the Board is powered, nearby and advertising. Compare the dropdown ID with the label, then restart an unpaired Board and tap **Scan** again. |
| `Pairing failed: check PIN / BOOT reset` | Verify all six PIN digits against the Board label or UART startup log. Prefer automatic cable pairing if available. |
| **Connect** is greyed out | The Panel already remembers a Board. Use the normal **Wireless (BLE)** button, or remove the old pairing with **Forget pairing** first. |
| **Forget pairing** is greyed out | This Panel has no remembered Board, so there is no local pairing to remove. |
| `Remembered Board is unavailable` | Check that the paired Board is powered and in range. If this Panel must be assigned to another Board, use the cable-based Forget workflow. |
| Forget or replacement fails | Power-cycle the Board, reconnect the UART commissioning cable, and retry. Make sure it is the Board whose ID is stored in the Panel. If the failure persists, hold the Board **BOOT** button for at least 3 seconds, then run **Forget pairing** over the cable again so the Panel record is cleared too. |

Bluetooth pairing uses authenticated Secure Connections. There is no insecure
legacy-pairing fallback, and the Panel never stores the six-digit commissioning
PIN after pairing succeeds.

## 📄 License

MIT — see [`LICENCE`](LICENCE).

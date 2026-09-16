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

- Visualization **Settings > Update period** adjusts live polling from **10 to 1000 ms**, with a **100 ms** default. Changes apply immediately and remain selected while navigating between devices/screens until reboot. Device response time and rendering load can make the actual interval longer.
- Runtime and DataBundle graphs support two visible signals with separate Y axes, autoscale labels, cursor readout and adjustable visible sample count.
- Tiny and very large float ranges are shown with compact axis scaling labels, so micro-scale and high-value signals stay readable.
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

## 📄 License

MIT — see [`LICENCE`](LICENCE).

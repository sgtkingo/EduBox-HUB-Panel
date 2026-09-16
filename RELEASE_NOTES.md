# RELEASE_NOTES

## 1.2.0.76 - Shared VSCP client/server update

### Protocol architecture

* Replaced the legacy static `Protocol` API and global UART messenger with the
  modular `vscp` library version `2.0.0`.
* Updated the wire protocol to VSCP API `1.4`.
* Added a transport-independent shared `Codec`, request `Client`, handler-based
  `Server`, common request/response types, and explicit `Transport` interface.
* Added transport adapters for Arduino `Stream`, C++ iostreams, and C `FILE*`
  streams.
* Removed the `expt` dependency from the VSCP transport layer. Diagnostics now
  use an optional injected `LogSink`, kept separate from the protocol channel.
* Moved board-specific UART ownership and configuration into the application.
  The sketch constructs `HardwareSerial`, `StreamTransport`, and one shared
  `Client`, which is injected into `DeviceManager`.
* Renamed application UART settings from the legacy `UART1_*` names to
  `EDUBOX_HUB_PANEL_VSCP_UART_*` settings in `libraries/engine/src/config.hpp`.

### Firmware and emulator interoperability

* Migrated `DeviceManager`, `BaseDevice`, and runtime GUI synchronization from
  removed `Protocol::*` calls to the injected `vscp::Client`.
* Restored compatible UART framing behavior:
  * RX and TX frames remove bytes outside printable ASCII (`32..126`).
  * Surrounding whitespace is trimmed.
  * Arduino writes include a separator newline and are flushed as complete
    frames.
* VSCP requests now serialize `type` first, for example:

  ```text
  ?type=INIT&api=1.4&app=board&db=1.3
  ```

* Updated both emulator implementations so request detection no longer depends
  on parameter order. Requests such as
  `?api=1.4&app=board&db=1.3&type=INIT` remain valid.
* Emulator framing now recognizes VSCP requests after a firmware log prefix
  while keeping log messages separate from protocol messages.
* Updated Czech and English protocol/developer documentation for the new API
  and module boundaries.

### Verification

* Added a native in-memory `vscp::Client` to `vscp::Server` integration test
  covering `INIT`, `CONNECT`, `UPDATE`, `CONFIG`, `CONTROL`, `RESET`, and
  `DISCONNECT`.
* Added validation for pre-INIT commands, failed requests, response UID
  mismatch, parameter ordering, and non-printable serial bytes.
* Added emulator protocol tests for both the basic and pattern servers.
* Verified 24 pattern-emulator `UPDATE` exchanges across four devices and all
  six scenarios: `normal`, `high_activity`, `thermal_event`, `environmental`,
  `vibration`, and `recovery`.
* Verified that scenario-specific values materially differ from normal mode
  while responses retain a successful status and matching device ID.
* Verified the complete ESP32-S3 build with the `Huge APP` partition profile:
  51% program storage and 50% global memory usage.

### Migration notes

* Code using static `Protocol::*` methods must construct a transport and
  `vscp::Client`, then pass the client to `DeviceManager`.
* Board integrations overriding `UART1_PORT`, `UART1_BAUDRATE`, `UART1_RX`, or
  `UART1_TX` must migrate to the corresponding `EDUBOX_HUB_PANEL_VSCP_UART_*` names.
* Serial Monitor and the emulator cannot normally open the same Windows COM
  port simultaneously; close Serial Monitor before starting the emulator.
* Firmware size exceeds the default ESP32-S3 application partition. Use the
  bundled custom partition table or a compatible large-app partition profile.

---

## Picture preview and storage update

### Runtime, VSCP and DB schema

* Added explicit `access` semantics for runtime values:
  * `read` values are fetched through `UPDATE`.
  * `write` values are sent through `CONTROL`.
  * `configs` continue to be sent through `CONFIG`.
* Runtime visualization now separates `Values` and `Configs` with a toggle.
* Writable/control values are shown as editable orange controls; config controls remain blue.
* Added hybrid test device `H00` / `Temperature Regulator`:
  * `temp` is a read value returned by `UPDATE`.
  * `set_point` is a write value sent by `CONTROL`.
  * `speed` is a config value sent by `CONFIG`.
* Updated the emulator and pattern emulator so `H00.temp` gradually follows `set_point`; `speed` controls the update step.
* Migrated VERSION to `config.hpp` as `EDUBOX_HUB_PANEL_FIRMWARE_VERSION`.

### Developer tooling and diagnostics

* Debug logging is centralized behind `ENABLE_DEBUG`.
* Added `DEBUG_VERBOSE_LEVEL` with levels `1..3`; default level is `2`.
* Emulator log parsing recognizes firmware `DEBUG`, `WARNING` and `EXCEPTION` lines while keeping VSCP command parsing separate.

### DataBundle and visualization fixes

* DataBundle CSV includes `DeviceName`, `DeviceUid`, `SampleIndex`, `RuntimeMs`, `SignalName`, `Value`.
* Recording uses runtime-relative milliseconds, so no RTC/NTP dependency is required.
* Runtime chart supports autoscaling, history browsing and visible scaling labels.
* Runtime and DataBundle graphs now support two visible signals with separate Y axes.
* Runtime sampling can be adjusted dynamically up to 100 visible samples.
* DataBundle Viewer includes graph and CSV modes, touch history browsing, cursor readout, autoscale and selectable signal blocks.
* Runtime and DataBundle axis labels now use compact formatting, max three decimals and engineering-style scale labels for tiny and very large values.
* DataBundle Viewer scale labels are shown directly near the graph and removed from local graph settings.
* Multi-value recording now records all device values, independent of which values are currently charted.

### Device catalog, settings and transfer mode

* Device catalog and selection screens were redesigned with richer device details, role-aware placeholder pictures and clearer pin state indication.
* Device definitions now load pictures from the explicit `picture` field instead of deriving image names from entity IDs.
* Library detail and Entity Edit screens now render fixed-size device picture previews with placeholder fallback when the referenced asset is missing.
* Device pin schema now uses named `Pins` definitions and default pin maps by tag.
* App settings can be saved to `/data/config.json`, with default communication and appearance placeholders persisted.
* Added SD-card-oriented Transfer Mode screen and service scaffolding for USB MSC bridge workflows.
* Transfer Mode now asks whether to restart the HMI after closing an active transfer session, so SD-card edits can be reloaded immediately.
* Main Screen now shows the active storage backend (`Storage: SD` or `Storage: SPIFFS`) next to the existing version label.

---

## Highlights

* Real-time **raw vs. processed** data inspection.
* **DataBundle** recording and **CSV export to SD** for offline analysis.
* Built-in **Sensor Wiki** as a quick reference layer.
* **VSCP** (text-based, REST-like) connectivity to PC emulator, EduBoxHub, or custom HW.
* **Runtime pin mapping** and **bidirectional I/O** (telemetry + actuator/config writes).

---

## Build / Flash artifacts (binaries)

Release binaries are provided for Arduino/ESP32 flashing workflows:

* `ui.ino.merged.bin`
  Single “all-in-one” image intended for the simplest flashing path.

* `ui.ino.bin`
  Application binary (firmware payload).

* `ui.ino.bootloader.bin`
  Bootloader image.

* `ui.ino.partitions.bin`
  Partition table image.

Developer artifacts (for debugging/profiling):

* `ui.ino.elf` 
* `ui.ino.map` 

> Flashing instructions and offsets are documented in `docs/INSTALL.md`.

---

## Compatibility matrix 

Upstream connectivity is based on **VSCP** (Virtual Sensors Communication Protocol):

* **PC Emulator**: supported (VSCP over the emulator/host link).
* **EduBoxHub**: supported (VSCP upstream integration).
* **Custom HW**: supported if it implements the open-source VSCP spec.

Protocol reference:

* VSCP repository: [https://github.com/sgtkingo/VSCP](https://github.com/sgtkingo/VSCP)

---

## Target hardware

* Verified target: **Elecrow ESP32 Display 7" HMI (ESP32-S3 + RGB TFT + Touch, LVGL-ready)**.

---

## Known limitations

* Visualization widgets and rendering policies depend on upstream channel metadata and current UI maturity.
* Heavy UI effects (e.g., excessive alpha/shadows) may be constrained by embedded rendering budget (frame time).
* “Supports any sensor” is bounded by what the upstream source can expose over VSCP and how channels are described.

---

## Next (planned)

* Expanded visualization widgets (trend views, event timeline, markers).
* DataBundle/CSV schema stabilization and validation tooling.
* Improved sensor Wiki authoring workflow + contribution guidelines.

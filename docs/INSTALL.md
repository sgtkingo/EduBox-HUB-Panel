# Arduino project for Elecrow DIS08070H ESP32 HMI with 7" Resistive Touch Display


## Prerequisites

1. Arduino-IDE should be installed on your system. (The build and board was tested with v1.8.19.)
2. In `Tools`/`Boards Manager` select and install `esp32` board-support package. (Contains the ESP toolchains for the board's Tensilica Xtensa ESP32 MCU. The current firmware was verified with `esp32` 3.1.1 on `ESP32S3 Dev Module`.)
3. (The essence of `LovyanGFX` with the display-driver is included in the local `libraries` folder of this board-template, but if you want to us Arduino's built-in one you can delete it and install to Arduino-IDE with `Tools`/`Manage Libraries`.)


## Board settings (retained between Arduino-IDE sessions)

1. In `Tools` select `ESP32 Arduino` / `ESP32S3 Dev Module` as board
2. Set the parameters for your board: Flash-mode to `QIO 80MHz`, Flash-size to `4MB`, PSRAM to `OPI PSRAM`, (Partition-scheme to `Huge App` with 3MB app and 1MB SPIFFS partitions)
3. If you'll want to upload to the device from Arduino-IDE, select the programmer tool and port in `Tools` menu: Arduino-IDE uses the `esptool` of ESP-IDF in the background.
4. (The `partitions.csv` file in `ui` subfolder of this sketch-folder beside `ui.ino` should override the `Huge App`/etc. partitioning scheme setting, for example to give bigger app-partition. But with v2 of esp32 package this seems not effective.)

### USB / Transfer Mode settings

Transfer Mode uses the ESP32-S3 native USB device stack and exposes the SD card as
a USB Mass Storage Class disk. It does not expose SPIFFS.

For Transfer Mode builds:

1. Use an ESP32-S3 board/profile with native USB support.
2. Set USB mode to native USB OTG/TinyUSB (`ARDUINO_USB_MODE == 0`). In Arduino
   IDE this is usually the `USB-OTG (TinyUSB)` option.
3. Keep USB CDC enabled on boot if the same USB connector is also used for
   Serial/log output.
4. Use the native ESP32-S3 USB connector for the PC cable. A separate USB-UART
   bridge can provide serial logs, but it will not expose the SD card as MSC.
5. Build with `STORAGE_OPTION_SD` when you want PC file transfer. SPIFFS is kept
   for demo/debug storage and is not shared over USB.

Relevant config flags live in [`libraries/engine/src/config.hpp`](../libraries/engine/src/config.hpp):

- `FILE_TRANSFER_USB_MSC_ENABLED`: enables the USB MSC bridge.
- `FILE_TRANSFER_USB_MSC_VENDOR_ID`
- `FILE_TRANSFER_USB_MSC_PRODUCT_ID`
- `FILE_TRANSFER_USB_MSC_REVISION`

If the selected Arduino core/board mode does not provide `USB.h` and `USBMSC.h`,
the firmware still builds, but Transfer Mode will show a backend-unavailable
error instead of pretending to mount a drive.


## Compilation

1. Open the `.ino` file in Arduino
2. Go to File/Preferences and set Sketchbook location to the path of your UI project (where this README is located). The setting is retained between Arduino-IDE sessions.
3. Update the libraries accourding *Libraries versions*
4. Build the project

## Persistent storage

The runtime storage backend is selected in [`libraries/engine/src/config.hpp`](../libraries/engine/src/config.hpp) via `STORAGE_OPTION`.

- `STORAGE_OPTION_SD`: persistent data is read from the SD card. The device catalog is expected at `/data/DB.json`.
- `STORAGE_OPTION_SPIFFS`: persistent data is read from internal flash SPIFFS. The device catalog is expected at `/DB.json`.

SD mode uses this shared layout:

- `/data/DB.json`: device catalog.
- `/data/pics/`: device pictures.
- `/data/config.json`: persistent application configuration.
- `/records/`: DataBundle CSV records.

When `STORAGE_OPTION_SPIFFS` is active:

- `STORAGE_SPIFFS_FORMAT_ON_FAIL` controls whether the filesystem is formatted automatically when mount fails.
- `STORAGE_SEED_DEFAULT_DB_ON_MISSING` is enabled by default and will create `/DB.json` from the embedded default catalog on first boot if the file is missing.

This means SPIFFS mode does not require a separate filesystem upload just to bootstrap the default device catalog.

## Device pictures

Device pictures are resolved from the `picture` field in the device catalog entry.

Supported values:

- `placeholder:device`
  Uses the built-in placeholder for the device role.
- `filename.png`
  Resolved relative to `STORAGE_DEVICE_PICTURE_DIR`, which defaults to `/data/pics`.
- `/data/pics/filename.png`
  Uses the explicit storage path directly.

Example:

```json
"picture": "DS18B20.png"
```

Expected storage location in SD mode:

- `/data/pics/DS18B20.png`

Expected storage location in SPIFFS mode:

- `/data/pics/DS18B20.png`

Notes:

- Library detail and Entity Edit use fixed-size previews, so the source image does not need to match the widget size exactly.
- PNG is the recommended format for static device pictures.
- If the picture file is missing, the UI falls back to the role placeholder and shows a warning marker in editor/detail preview.

## Transfer Mode behavior

Transfer Mode is a locked session:

1. Opening the Transfer screen only shows a preparation page.
2. After pressing `OK`, the HMI checks for an SD card and locks SD access from the
   application.
3. The USB MSC backend remounts the SD card for raw block read/write callbacks.
4. The PC should see the HMI as a USB storage device.
5. Pressing `End session` stops USB MSC, remounts SD for the HMI, restores USB CDC
   logging, and returns to the Main Menu.

Do not write to SD through HMI code while Transfer Mode is active. During that
period the USB host owns the card. Logs are buffered in RAM while USB CDC is not
available and are flushed after the session ends.

# Libraries versions

1. LovyanGFX: 1.20
2. LVGL: 8.3.11 (depends on SquareLine studio)

# Boards versions

1. Arduino ESP32 Boards: 2.0.18 
2. esp32: 3.1.1 

## Burning

If the built-in Arduino-IDE chip-programmer is working you can go with it.

(If you want to make a backup before overwriting the flash in the device, you can use esptool's read command like: `esptool.py -b 460800 read_flash 0 ALL Backup.bin`.)

If it's not working on your system, there are two ways depending on esp32 package version used:

### esp32-3.0.0 (and above):
You can find the built merged .bin file in the folder (like `tmp`) where Arduino-IDE puts the builds.
The whole .bin (the merged 4MB one containing all partitions) can be burnt to the board by the ESP-IDF command: `esptool.py -b 460800 write_flash 0 ui.ino.merged.bin`

### A faster way, working with esp32-2.0.17 too (flashing just the application):
You can find the built ELF's flashable version in a `ui.ino.bin` file in the folder (like `tmp`) where Arduino-IDE puts the builds.
This `ui.ino.bin` file can be burnt to the board's app0 partition at offset 0x10000 by the ESP-IDF command: `esptool.py  write_flash  0x10000 ui.ino.bin`. (Offset might differ with other patition scheme, get app0 offset by `esptool.py read_flash 0x8000 0xc00 ptable.img` and `gen_esp32part.py ptable.img`.)

> For Arduino IDE:
1. Open .INO file (in `ui`)
2. In `File->Preferences->Sketchbook location` set up the project root directory
3. Start `Verify/Compile` (`CTRL+R`)
4. Export binaries by `Export Compiled Binary` (`Alt+Ctrl+S`) in `Sketch` barlist


## Misc. notes

With this board-template you might encounter some Parallel-RGB vs LVGL-flush synchronization issues if you use computing-heavy or alpha-channel graphic contents like big shadows. According to our research it comes from low-level IO code in Arduino-IDE related to the display-driver, probably GDMA-priority related pixeldata-delays to the ESP32S3's LCD-controller. (ESP-IDF version doesn't have this problem.)


2024 SquareLine, 2026 EduBox HUB Panel (MTA)


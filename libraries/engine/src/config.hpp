/**
 * @file config.h
 * @brief Configuration file for platform-specific settings.
 * 
 * This file defines macros to select the execution environment (Arduino or standard console).
 * Uncomment the desired macro to enable the respective environment.
 * 
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny
 */

#ifndef CONFIG_ENGINE_H
#define CONFIG_ENGINE_H

/// Uncomment to enable Arduino-based environments
#ifndef ARDUINO_H_ENV
#define ARDUINO_H_ENV
#endif

/// Uncomment to enable standard console applications (PC/Linux)
//#define STDIO_H_ENV

// Uncomment to enable LVGL support
#ifndef USE_LVGL
#define USE_LVGL
#endif

// Uncomment to enable ESP32 platform
//#define ESP_PLATFORM

// Unified project debug logging switch. Exceptions use error level 1.
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 0
#endif

// Debug verbosity:
// 0 = disabled, 1 = errors only, 2 = warnings and important operations, 3 = all debug details.
#ifndef DEBUG_VERBOSE_LEVEL
#define DEBUG_VERBOSE_LEVEL 0
#endif

// Application-owned UART wiring for the injected VSCP transport.
#ifndef EDUBOX_HUB_PANEL_VSCP_UART_PORT
#define EDUBOX_HUB_PANEL_VSCP_UART_PORT 0
#endif

#ifndef EDUBOX_HUB_PANEL_VSCP_UART_BAUDRATE
#define EDUBOX_HUB_PANEL_VSCP_UART_BAUDRATE 115200
#endif

#ifndef EDUBOX_HUB_PANEL_VSCP_UART_RX
#define EDUBOX_HUB_PANEL_VSCP_UART_RX -1
#endif

#ifndef EDUBOX_HUB_PANEL_VSCP_UART_TX
#define EDUBOX_HUB_PANEL_VSCP_UART_TX -1
#endif

#ifndef CASE_SENSITIVE
#define CASE_SENSITIVE true
#endif

// Firmware version mirrored from the root VERSION file by storage/sync_version.py.
#ifndef EDUBOX_HUB_PANEL_FIRMWARE_VERSION
#define EDUBOX_HUB_PANEL_FIRMWARE_VERSION "1.2.2.0"
#endif

// Storage backend selection for all persistent data access.
#define STORAGE_OPTION_SD 1
#define STORAGE_OPTION_SPIFFS 2

#ifndef STORAGE_OPTION
// Default to SPIFFS for development boards without an SD card installed.
#define STORAGE_OPTION STORAGE_OPTION_SD
#endif

// When enabled, SPIFFS will be formatted automatically if mount fails.
#ifndef STORAGE_SPIFFS_FORMAT_ON_FAIL
#define STORAGE_SPIFFS_FORMAT_ON_FAIL 1
#endif

#if STORAGE_OPTION == STORAGE_OPTION_SPIFFS
// SPIFFS stores the boot catalog in the filesystem root.
#define STORAGE_DEFAULT_DEVICE_DB_PATH "/DB.json"
// Automatically create the default embedded DB on first boot when missing.
#define STORAGE_SEED_DEFAULT_DB_ON_MISSING 1
#else
// SD mode keeps the catalog under the shared /data folder on the card.
#define STORAGE_DEFAULT_DEVICE_DB_PATH "/data/DB.json"
#define STORAGE_SEED_DEFAULT_DB_ON_MISSING 0
#endif

// Shared storage layout. In SD mode these paths are visible to PC transfer mode.
#ifndef STORAGE_DATA_DIR
#define STORAGE_DATA_DIR "/data"
#endif

#ifndef STORAGE_DEVICE_PICTURE_DIR
#define STORAGE_DEVICE_PICTURE_DIR "/data/pics"
#endif

#ifndef STORAGE_APP_CONFIG_PATH
#define STORAGE_APP_CONFIG_PATH "/data/config.json"
#endif

#ifndef STORAGE_RECORDS_DIR
#define STORAGE_RECORDS_DIR "/records"
#endif

// SD card wiring used by the display board.
#ifndef STORAGE_SD_PIN_MOSI
#define STORAGE_SD_PIN_MOSI 11
#endif

#ifndef STORAGE_SD_PIN_MISO
#define STORAGE_SD_PIN_MISO 13
#endif

#ifndef STORAGE_SD_PIN_SCK
#define STORAGE_SD_PIN_SCK 12
#endif

#ifndef STORAGE_SD_PIN_CS
#define STORAGE_SD_PIN_CS 10
#endif

// USB Mass Storage transfer bridge. Requires ESP32-S3 native USB OTG/TinyUSB
// support in the Arduino board configuration. Keep USB CDC enabled on boot if
// the same connector is also used as the runtime UART/log channel.
#ifndef FILE_TRANSFER_USB_MSC_ENABLED
#define FILE_TRANSFER_USB_MSC_ENABLED 0
#endif

#ifndef FILE_TRANSFER_USB_MSC_VENDOR_ID
#define FILE_TRANSFER_USB_MSC_VENDOR_ID "MTA"
#endif

#ifndef FILE_TRANSFER_USB_MSC_PRODUCT_ID
#define FILE_TRANSFER_USB_MSC_PRODUCT_ID "EduBox HUB Panel"
#endif

#ifndef FILE_TRANSFER_USB_MSC_REVISION
#define FILE_TRANSFER_USB_MSC_REVISION "1.0"
#endif

#endif // CONFIG_ENGINE_H

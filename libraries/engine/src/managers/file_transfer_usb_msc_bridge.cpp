#include "file_transfer_usb_msc_bridge.hpp"

#include "../config.hpp"
#include "expt.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

#if FILE_TRANSFER_USB_MSC_ENABLED && defined(ARDUINO_H_ENV) && defined(ARDUINO) && defined(ARDUINO_USB_MODE) && (ARDUINO_USB_MODE == 0) && __has_include(<USB.h>) && __has_include(<USBMSC.h>)
#include <SD.h>
#include <SPI.h>
#include <USB.h>
#include <USBMSC.h>
#define EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND 1
#else
#define EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND 0
#endif

namespace
{
constexpr uint16_t MSC_BLOCK_SIZE = 512;

#if EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND
USBMSC usbMsc;

bool sdMountedForMsc = false;

bool mountSdForMsc()
{
    SPI.begin(STORAGE_SD_PIN_SCK, STORAGE_SD_PIN_MISO, STORAGE_SD_PIN_MOSI, STORAGE_SD_PIN_CS);
    sdMountedForMsc = SD.begin(STORAGE_SD_PIN_CS);
    return sdMountedForMsc;
}

void unmountSdForMsc()
{
    if (sdMountedForMsc) {
        SD.end();
        sdMountedForMsc = false;
    }
}

bool readSector(uint32_t sector, uint8_t *buffer)
{
    return SD.readRAW(buffer, sector);
}

bool writeSector(uint32_t sector, const uint8_t *buffer)
{
    return SD.writeRAW(const_cast<uint8_t *>(buffer), sector);
}

int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    if (!sdMountedForMsc || !buffer) {
        return -1;
    }

    uint8_t *dest = static_cast<uint8_t *>(buffer);
    uint8_t sectorBuffer[MSC_BLOCK_SIZE];
    uint32_t processed = 0;

    while (processed < bufsize) {
        const uint32_t absoluteOffset = offset + processed;
        const uint32_t sector = lba + (absoluteOffset / MSC_BLOCK_SIZE);
        const uint32_t sectorOffset = absoluteOffset % MSC_BLOCK_SIZE;
        const uint32_t chunk = std::min<uint32_t>(MSC_BLOCK_SIZE - sectorOffset, bufsize - processed);

        if (sectorOffset == 0 && chunk == MSC_BLOCK_SIZE) {
            if (!readSector(sector, dest + processed)) {
                return -1;
            }
        } else {
            if (!readSector(sector, sectorBuffer)) {
                return -1;
            }
            std::memcpy(dest + processed, sectorBuffer + sectorOffset, chunk);
        }

        processed += chunk;
    }

    return static_cast<int32_t>(bufsize);
}

int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    if (!sdMountedForMsc || !buffer) {
        return -1;
    }

    uint8_t sectorBuffer[MSC_BLOCK_SIZE];
    uint32_t processed = 0;

    while (processed < bufsize) {
        const uint32_t absoluteOffset = offset + processed;
        const uint32_t sector = lba + (absoluteOffset / MSC_BLOCK_SIZE);
        const uint32_t sectorOffset = absoluteOffset % MSC_BLOCK_SIZE;
        const uint32_t chunk = std::min<uint32_t>(MSC_BLOCK_SIZE - sectorOffset, bufsize - processed);

        if (sectorOffset == 0 && chunk == MSC_BLOCK_SIZE) {
            if (!writeSector(sector, buffer + processed)) {
                return -1;
            }
        } else {
            if (!readSector(sector, sectorBuffer)) {
                return -1;
            }
            std::memcpy(sectorBuffer + sectorOffset, buffer + processed, chunk);
            if (!writeSector(sector, sectorBuffer)) {
                return -1;
            }
        }

        processed += chunk;
    }

    return static_cast<int32_t>(bufsize);
}

bool onStartStop(uint8_t powerCondition, bool start, bool loadEject)
{
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT,
                    "FileTransferUsbMscBridge::onStartStop",
                    "usb msc event",
                    "power=%u start=%d eject=%d",
                    powerCondition,
                    start ? 1 : 0,
                    loadEject ? 1 : 0);
    return true;
}
#endif
}

FileTransferUsbMscBridge &FileTransferUsbMscBridge::instance()
{
    static FileTransferUsbMscBridge bridge;
    return bridge;
}

FileTransferUsbMscBridge &fileTransferUsbMscBridge()
{
    return FileTransferUsbMscBridge::instance();
}

bool FileTransferUsbMscBridge::isSupported() const
{
#if EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND
    return true;
#else
    return false;
#endif
}

bool FileTransferUsbMscBridge::start(std::string &error)
{
    if (active) {
        return true;
    }

#if !EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND
    error = "USB MSC bridge backend is not available. Enable ESP32-S3 native USB OTG/TinyUSB mode and USBMSC support.";
    return false;
#else
    if (!mountSdForMsc()) {
        error = "Failed to mount SD card for raw USB MSC access.";
        return false;
    }

    const uint64_t sectors = SD.numSectors();
    if (sectors == 0) {
        unmountSdForMsc();
        error = "SD card reports zero sectors.";
        return false;
    }

    const uint32_t blockCount = static_cast<uint32_t>(std::min<uint64_t>(sectors, std::numeric_limits<uint32_t>::max()));
    usbMsc.vendorID(FILE_TRANSFER_USB_MSC_VENDOR_ID);
    usbMsc.productID(FILE_TRANSFER_USB_MSC_PRODUCT_ID);
    usbMsc.productRevision(FILE_TRANSFER_USB_MSC_REVISION);
    usbMsc.onRead(onRead);
    usbMsc.onWrite(onWrite);
    usbMsc.onStartStop(onStartStop);
    usbMsc.mediaPresent(true);

    if (!usbMsc.begin(blockCount, MSC_BLOCK_SIZE)) {
        unmountSdForMsc();
        error = "USB MSC begin failed.";
        return false;
    }

    if (!USB.begin()) {
        usbMsc.mediaPresent(false);
        usbMsc.end();
        unmountSdForMsc();
        error = "USB device stack begin failed.";
        return false;
    }

    active = true;
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT,
                    "FileTransferUsbMscBridge::start",
                    "usb msc start",
                    "sectors=%llu blockSize=%u",
                    sectors,
                    MSC_BLOCK_SIZE);
    return true;
#endif
}

bool FileTransferUsbMscBridge::stop(std::string &error)
{
    if (!active) {
#if EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND
        unmountSdForMsc();
#endif
        return true;
    }

#if !EDUBOX_HUB_PANEL_HAS_USB_MSC_BACKEND
    active = false;
    return true;
#else
    usbMsc.mediaPresent(false);
    usbMsc.end();
    unmountSdForMsc();
    active = false;
    error.clear();
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "FileTransferUsbMscBridge::stop", "usb msc stop", "stopped");
    return true;
#endif
}

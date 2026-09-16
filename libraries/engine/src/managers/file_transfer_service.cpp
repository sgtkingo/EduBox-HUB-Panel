#include "file_transfer_service.hpp"

#include "file_transfer_usb_msc_bridge.hpp"
#include "storage_manager.hpp"
#include "expt.hpp"

#if defined(ARDUINO_H_ENV) && defined(ARDUINO)
#include <SD.h>
#include <SPI.h>
#endif

bool FileTransferService::isSdCardPresent() const
{
#if !defined(ARDUINO_H_ENV) || !defined(ARDUINO)
    return false;
#elif STORAGE_OPTION == STORAGE_OPTION_SD
    if (storageManager().isAvailable() || storageManager().isTransferLocked())
    {
        return true;
    }

    SPI.begin(STORAGE_SD_PIN_SCK, STORAGE_SD_PIN_MISO, STORAGE_SD_PIN_MOSI, STORAGE_SD_PIN_CS);
    const bool mounted = SD.begin(STORAGE_SD_PIN_CS);
    if (mounted)
    {
        SD.end();
    }
    return mounted;
#else
    SPI.begin(STORAGE_SD_PIN_SCK, STORAGE_SD_PIN_MISO, STORAGE_SD_PIN_MOSI, STORAGE_SD_PIN_CS);
    const bool mounted = SD.begin(STORAGE_SD_PIN_CS);
    if (mounted)
    {
        SD.end();
    }
    return mounted;
#endif
}

bool FileTransferService::start()
{
    try
    {
        if (transferModeActive) {
            return state == FileTransferState::READY;
        }

        // Reject incompatible USB configurations before touching the mounted SD
        // filesystem or disabling logging. Hardware CDC cannot provide USB MSC.
        if (!fileTransferUsbMscBridge().isSupported()) {
            state = FileTransferState::UNSUPPORTED;
            lastMessage = "USB MSC bridge backend is not available. Enable ESP32-S3 native USB OTG/TinyUSB mode and USBMSC support.";
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::start", "transfer unsupported", "%s", lastMessage.c_str());
            return false;
        }

        if (!isSdCardPresent())
        {
            state = FileTransferState::MISSING_SD;
            lastMessage = "SD card is not detected.";
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::start", "transfer sd missing", "%s", lastMessage.c_str());
            return false;
        }

        state = FileTransferState::CONNECTING;
        lastMessage = "Connecting...";
        transferModeActive = true;
        setLoggerUsbCdcAvailable(false);

#if STORAGE_OPTION == STORAGE_OPTION_SD
        if (!storageManager().enterTransferLock())
        {
            setLoggerUsbCdcAvailable(true);
            flushBufferedLogMessages();
            transferModeActive = false;
            state = FileTransferState::ERROR;
            lastMessage = "Failed to lock SD card for transfer.";
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::start", "transfer lock failed", "%s", lastMessage.c_str());
            return false;
        }
#endif

        std::string bridgeError;
        if (!fileTransferUsbMscBridge().start(bridgeError))
        {
            state = FileTransferState::ERROR;
            lastMessage = bridgeError.empty() ? "USB MSC bridge backend failed to start." : bridgeError;
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::start", "transfer backend failed", "%s", lastMessage.c_str());
#if STORAGE_OPTION == STORAGE_OPTION_SD
            storageManager().exitTransferLock();
#endif
            setLoggerUsbCdcAvailable(true);
            flushBufferedLogMessages();
            transferModeActive = false;
            return false;
        }

        state = FileTransferState::READY;
        lastMessage = "Transfer ready";
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "FileTransferService::start", "transfer ready", "%s", lastMessage.c_str());
        return true;
    }
    catch (const Exception &ex)
    {
        ex.print();
        state = FileTransferState::ERROR;
        lastMessage = ex.flush(0);
        std::string bridgeError;
        fileTransferUsbMscBridge().stop(bridgeError);
#if STORAGE_OPTION == STORAGE_OPTION_SD
        storageManager().exitTransferLock();
#endif
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        return false;
    }
    catch (const std::exception &ex)
    {
        Exception("FileTransferService::start", ex.what()).print();
        state = FileTransferState::ERROR;
        lastMessage = ex.what();
        std::string bridgeError;
        fileTransferUsbMscBridge().stop(bridgeError);
#if STORAGE_OPTION == STORAGE_OPTION_SD
        storageManager().exitTransferLock();
#endif
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        return false;
    }
    catch (...)
    {
        Exception("FileTransferService::start", "Unknown exception").print();
        state = FileTransferState::ERROR;
        lastMessage = "Unknown exception while starting transfer.";
        std::string bridgeError;
        fileTransferUsbMscBridge().stop(bridgeError);
#if STORAGE_OPTION == STORAGE_OPTION_SD
        storageManager().exitTransferLock();
#endif
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        return false;
    }
}

bool FileTransferService::stop()
{
    try
    {
        if (!transferModeActive)
        {
            setLoggerUsbCdcAvailable(true);
            state = FileTransferState::IDLE;
            lastMessage = "Transfer mode is idle.";
            return true;
        }

        std::string bridgeError;
        const bool bridgeStopped = fileTransferUsbMscBridge().stop(bridgeError);

#if STORAGE_OPTION == STORAGE_OPTION_SD
        if (!storageManager().exitTransferLock())
        {
            setLoggerUsbCdcAvailable(true);
            flushBufferedLogMessages();
            transferModeActive = false;
            state = FileTransferState::ERROR;
            lastMessage = bridgeStopped ? "Failed to remount SD card after transfer." : bridgeError + " Failed to remount SD card after transfer.";
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::stop", "transfer unlock failed", "%s", lastMessage.c_str());
            return false;
        }
#endif

        if (!bridgeStopped)
        {
            setLoggerUsbCdcAvailable(true);
            flushBufferedLogMessages();
            transferModeActive = false;
            state = FileTransferState::ERROR;
            lastMessage = bridgeError.empty() ? "USB MSC bridge failed to stop." : bridgeError;
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "FileTransferService::stop", "transfer backend stop failed", "%s", lastMessage.c_str());
            return false;
        }

        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        state = FileTransferState::IDLE;
        lastMessage = "Transfer mode stopped. SD card is mounted back in HMI.";
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "FileTransferService::stop", "transfer unlock", "%s", lastMessage.c_str());
        return true;
    }
    catch (const Exception &ex)
    {
        ex.print();
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        state = FileTransferState::ERROR;
        lastMessage = ex.flush(0);
        return false;
    }
    catch (const std::exception &ex)
    {
        Exception("FileTransferService::stop", ex.what()).print();
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        state = FileTransferState::ERROR;
        lastMessage = ex.what();
        return false;
    }
    catch (...)
    {
        Exception("FileTransferService::stop", "Unknown exception").print();
        setLoggerUsbCdcAvailable(true);
        flushBufferedLogMessages();
        transferModeActive = false;
        state = FileTransferState::ERROR;
        lastMessage = "Unknown exception while stopping transfer.";
        return false;
    }
}

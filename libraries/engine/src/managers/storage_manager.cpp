/**
 * @file storage_manager.cpp
 * @brief Storage backend abstraction for persistent project data.
 */

#include "storage_manager.hpp"

#include "expt.hpp"

#ifdef USE_LVGL
#include "lvgl.h"
#endif

#include <algorithm>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

namespace
{
std::string normalizeDirectoryPath(std::string path)
{
    if (path.empty()) {
        return "/";
    }

    if (path.front() != '/') {
        path.insert(path.begin(), '/');
    }

    if (path.size() > 1 && path.back() != '/') {
        path.push_back('/');
    }

    return path;
}

std::string ensureAbsolutePath(std::string path)
{
    if (path.empty()) {
        return "/";
    }

    if (path.front() != '/') {
        path.insert(path.begin(), '/');
    }

    return path;
}

std::string buildListedPath(const char *rawName, const std::string &currentDirectory)
{
    if (!rawName || rawName[0] == '\0') {
        return "";
    }

    std::string path = rawName;
    if (path.front() == '/') {
        return ensureAbsolutePath(path);
    }

    return normalizeDirectoryPath(currentDirectory) + path;
}

std::string fileNamePart(const std::string &path)
{
    const size_t separator = path.find_last_of('/');
    if (separator == std::string::npos) {
        return path;
    }
    return path.substr(separator + 1);
}

void collectMatchingFilesRecursive(File &directory,
                                   const std::string &currentDirectory,
                                   const std::string &matchPrefix,
                                   fs::FS *filesystem,
                                   std::vector<std::string> &filePaths)
{
    directory.rewindDirectory();
    while (true) {
        File entry = directory.openNextFile();
        if (!entry) {
            break;
        }

        const std::string fullPath = buildListedPath(entry.name(), currentDirectory);
        if (entry.isDirectory()) {
            const std::string childDirectory = normalizeDirectoryPath(fullPath);
            collectMatchingFilesRecursive(entry, childDirectory, matchPrefix, filesystem, filePaths);
        } else if (!fullPath.empty() && fullPath.rfind(matchPrefix, 0) == 0) {
            filePaths.push_back(fullPath);
        } else if (filesystem && matchPrefix != "/") {
            const std::string prefixedPath = normalizeDirectoryPath(matchPrefix) + fileNamePart(fullPath);
            if (filesystem->exists(prefixedPath.c_str())) {
                debugLogMessage("StorageManager::listFiles", "storage path normalized", "raw=%s normalized=%s", fullPath.c_str(), prefixedPath.c_str());
                filePaths.push_back(prefixedPath);
            }
        }

        entry.close();
    }
}
}

StorageManager &StorageManager::instance()
{
    static StorageManager manager;
    return manager;
}

StorageManager &storageManager()
{
    return StorageManager::instance();
}

bool StorageManager::init()
{
    if (initialized) {
        debugLogMessage("StorageManager::init", "storage init skipped", "already initialized available=%d", available);
        return available;
    }

#if STORAGE_OPTION == STORAGE_OPTION_SD
    activeFilesystem = &SD;
    activeStorageLabel = "SD";
    debugLogMessage("StorageManager::init", "storage mount", "mounting SD backend");
    SPI.begin(STORAGE_SD_PIN_SCK, STORAGE_SD_PIN_MISO, STORAGE_SD_PIN_MOSI, STORAGE_SD_PIN_CS);
    available = SD.begin(STORAGE_SD_PIN_CS);
#elif STORAGE_OPTION == STORAGE_OPTION_SPIFFS
    activeFilesystem = &SPIFFS;
    activeStorageLabel = "SPIFFS";
    debugLogMessage(
        "StorageManager::init",
        "storage mount",
        "mounting SPIFFS backend formatOnFail=%d",
        STORAGE_SPIFFS_FORMAT_ON_FAIL
    );
    available = SPIFFS.begin(STORAGE_SPIFFS_FORMAT_ON_FAIL != 0);
#endif

    initialized = true;

    if (!available) {
#if STORAGE_OPTION == STORAGE_OPTION_SD
        debugLogMessage("StorageManager::init", "storage mount failed", "SD mount failed");
#elif STORAGE_OPTION == STORAGE_OPTION_SPIFFS
        debugLogMessage(
            "StorageManager::init",
            "storage mount failed",
            "SPIFFS mount failed. Check that the selected partition scheme contains a SPIFFS partition."
        );
#endif
        return false;
    }

    debugLogMessage(
        "StorageManager::init",
        "storage mount",
        "%s mounted successfully total=%llu used=%llu",
        activeStorageLabel,
        totalBytes(),
        usedBytes()
    );

    return available;
}

bool StorageManager::enterTransferLock()
{
#if STORAGE_OPTION != STORAGE_OPTION_SD
    debugLogMessage(DEBUG_VERBOSE_ERRORS, "StorageManager::enterTransferLock", "transfer lock unsupported", "storage=%s", activeStorageLabel);
    return false;
#else
    if (transferLocked) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "StorageManager::enterTransferLock", "transfer lock", "already locked");
        return true;
    }

    if (!initialized) {
        init();
    }

    if (!available) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "StorageManager::enterTransferLock", "transfer lock failed", "SD unavailable");
        return false;
    }

    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "StorageManager::enterTransferLock", "transfer lock", "unmounting SD for USB transfer");
#ifdef USE_LVGL
    // Image decoders can keep File handles cached even for hidden screens.
    // Close them while FATFS is still mounted, before SD.end() frees it.
    lv_img_cache_invalidate_src(nullptr);
#endif
    SD.end();
    available = false;
    transferLocked = true;
    return true;
#endif
}

bool StorageManager::exitTransferLock()
{
#if STORAGE_OPTION != STORAGE_OPTION_SD
    return true;
#else
    if (!transferLocked) {
        return true;
    }

    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "StorageManager::exitTransferLock", "transfer unlock", "remounting SD after USB transfer");
    SPI.begin(STORAGE_SD_PIN_SCK, STORAGE_SD_PIN_MISO, STORAGE_SD_PIN_MOSI, STORAGE_SD_PIN_CS);
    activeFilesystem = &SD;
    activeStorageLabel = "SD";
    available = SD.begin(STORAGE_SD_PIN_CS);
    transferLocked = false;
    debugLogMessage(
        available ? DEBUG_VERBOSE_IMPORTANT : DEBUG_VERBOSE_ERRORS,
        "StorageManager::exitTransferLock",
        available ? "transfer unlock" : "transfer unlock failed",
        "SD remount result=%d",
        available ? 1 : 0);
    return available;
#endif
}

bool StorageManager::exists(const std::string &path) const
{
    if (!available || !activeFilesystem) {
        debugLogMessage("StorageManager::exists", "storage unavailable", "exists skipped path=%s", path.c_str());
        return false;
    }

    const std::string absolutePath = ensureAbsolutePath(path);
#if defined(ARDUINO_ARCH_ESP32)
    // Do not walk TLSF pools here: largest-block/info queries hold the heap lock
    // with interrupts disabled and the diagnostic itself can trip the IWDT.
    debugLogMessage("StorageManager::exists", "storage probe", "path=%s internalFree=%u psramFree=%u",
                    absolutePath.c_str(),
                    static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                    static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
#endif
    const bool result = activeFilesystem->exists(absolutePath.c_str());
    debugLogMessage("StorageManager::exists", "storage read", "path=%s exists=%d", absolutePath.c_str(), result);
    return result;
}

bool StorageManager::ensureDirectory(const std::string &path)
{
    if (!available || !activeFilesystem) {
        debugLogMessage("StorageManager::ensureDirectory", "storage unavailable", "ensure directory skipped path=%s", path.c_str());
        return false;
    }

#if STORAGE_OPTION == STORAGE_OPTION_SPIFFS
    debugLogMessage("StorageManager::ensureDirectory", "spiffs virtual directory", "path=%s", path.c_str());
    return true;
#else
    const std::string normalized = normalizeDirectoryPath(path);
    if (activeFilesystem->exists(normalized.c_str())) {
        debugLogMessage("StorageManager::ensureDirectory", "storage read", "directory already exists path=%s", normalized.c_str());
        return true;
    }
    const bool created = activeFilesystem->mkdir(normalized.c_str());
    debugLogMessage("StorageManager::ensureDirectory", "storage write", "mkdir path=%s result=%d", normalized.c_str(), created);
    return created;
#endif
}

bool StorageManager::remove(const std::string &path)
{
    if (!available || !activeFilesystem) {
        debugLogMessage("StorageManager::remove", "storage unavailable", "remove skipped path=%s", path.c_str());
        return false;
    }

    const std::string absolutePath = ensureAbsolutePath(path);
    const bool removed = activeFilesystem->remove(absolutePath.c_str());
    debugLogMessage("StorageManager::remove", "storage write", "remove path=%s result=%d", absolutePath.c_str(), removed);
    return removed;
}

File StorageManager::open(const std::string &path, const char *mode) const
{
    if (!available || !activeFilesystem) {
        debugLogMessage("StorageManager::open", "storage unavailable", "open skipped path=%s mode=%s", path.c_str(), mode ? mode : "-");
        return File();
    }

    const std::string absolutePath = ensureAbsolutePath(path);
    debugLogMessage("StorageManager::open", "storage read/write", "open path=%s mode=%s", absolutePath.c_str(), mode ? mode : "-");
    return activeFilesystem->open(absolutePath.c_str(), mode);
}

std::vector<std::string> StorageManager::listFiles(const std::string &directoryPath) const
{
    std::vector<std::string> filePaths;

    if (!available || !activeFilesystem) {
        debugLogMessage("StorageManager::listFiles", "storage unavailable", "list skipped directory=%s", directoryPath.c_str());
        return filePaths;
    }

    const std::string normalizedDirectory = normalizeDirectoryPath(directoryPath);
    debugLogMessage("StorageManager::listFiles", "storage read", "listing directory=%s", normalizedDirectory.c_str());

#if STORAGE_OPTION == STORAGE_OPTION_SPIFFS
    File directory = activeFilesystem->open(normalizedDirectory.c_str(), FILE_READ);
    if (directory && directory.isDirectory()) {
        collectMatchingFilesRecursive(directory, normalizedDirectory, normalizedDirectory, activeFilesystem, filePaths);
        directory.close();
    } else if (directory) {
        directory.close();
    }

    if (filePaths.empty()) {
        File root = activeFilesystem->open("/", FILE_READ);
        if (!root || !root.isDirectory()) {
            if (root) {
                root.close();
            }
            debugLogMessage("StorageManager::listFiles", "storage read failed", "cannot open SPIFFS root");
            return filePaths;
        }

        collectMatchingFilesRecursive(root, "/", normalizedDirectory, activeFilesystem, filePaths);
        root.close();
    }
#else
    File directory = activeFilesystem->open(normalizedDirectory.c_str(), FILE_READ);
    if (!directory || !directory.isDirectory()) {
        debugLogMessage("StorageManager::listFiles", "storage read failed", "cannot open directory=%s", normalizedDirectory.c_str());
        return filePaths;
    }

    directory.rewindDirectory();
    while (true) {
        File entry = directory.openNextFile();
        if (!entry) {
            break;
        }

        const char *rawName = entry.name();
        std::string fullPath = rawName ? std::string(rawName) : std::string();
        if (!fullPath.empty() && fullPath.front() != '/') {
            fullPath = normalizedDirectory + fullPath;
        } else if (fullPath.empty() && rawName) {
            fullPath = normalizedDirectory + rawName;
        }

        if (!fullPath.empty() && !entry.isDirectory()) {
            filePaths.push_back(fullPath);
        }

        entry.close();
    }
    directory.close();
#endif

    std::sort(filePaths.begin(), filePaths.end());
    filePaths.erase(std::unique(filePaths.begin(), filePaths.end()), filePaths.end());
    debugLogMessage("StorageManager::listFiles", "storage read", "listed directory=%s count=%u", normalizedDirectory.c_str(), static_cast<unsigned int>(filePaths.size()));
    return filePaths;
}

uint64_t StorageManager::totalBytes() const
{
    if (!available || !activeFilesystem) {
        return 0;
    }

#if STORAGE_OPTION == STORAGE_OPTION_SD
    return SD.totalBytes();
#else
    return SPIFFS.totalBytes();
#endif
}

uint64_t StorageManager::usedBytes() const
{
    if (!available || !activeFilesystem) {
        return 0;
    }

#if STORAGE_OPTION == STORAGE_OPTION_SD
    return SD.usedBytes();
#else
    return SPIFFS.usedBytes();
#endif
}

#include "app_config_manager.hpp"

#include "../config.hpp"
#include "default_json_config.hpp"
#include "storage_manager.hpp"
#include "expt.hpp"

#include <ArduinoJson.h>

bool AppConfigManager::devicePicturesEnabled = true;

namespace
{
std::string normalize(std::string value)
{
    for (char &ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

void applyConfigDocument(JsonDocument &doc, AppConfig &config)
{
    config.defaultCommunication = AppConfigManager::communicationFromString(doc["defaultCommunication"] | "ask");
    JsonObjectConst appearance = doc["appearance"].as<JsonObjectConst>();
    config.theme = AppConfigManager::themeFromString(appearance["theme"] | "light");
    config.language = AppConfigManager::languageFromString(appearance["language"] | "en");
    config.useDevicePictures = appearance["useDevicePictures"] | true;
}
}

const char *AppConfigManager::toString(DefaultCommunicationMode mode)
{
    switch (mode) {
    case DefaultCommunicationMode::CABLE:
        return "cable";
    case DefaultCommunicationMode::WIRELESS_AUTO:
        return "wireless_auto";
    case DefaultCommunicationMode::WIRELESS_MANUAL:
        return "wireless_manual";
    case DefaultCommunicationMode::ASK:
    default:
        return "ask";
    }
}

const char *AppConfigManager::toString(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? "dark" : "light";
}

const char *AppConfigManager::toString(LanguageMode mode)
{
    switch (mode) {
    case LanguageMode::CZECH:
        return "cz";
    case LanguageMode::GERMAN:
        return "de";
    case LanguageMode::ENGLISH:
    default:
        return "en";
    }
}

DefaultCommunicationMode AppConfigManager::communicationFromString(const std::string &value)
{
    const std::string normalized = normalize(value);
    if (normalized == "cable" || normalized == "uart") {
        return DefaultCommunicationMode::CABLE;
    }
    if (normalized == "wireless_auto") {
        return DefaultCommunicationMode::WIRELESS_AUTO;
    }
    if (normalized == "wireless_manual") {
        return DefaultCommunicationMode::WIRELESS_MANUAL;
    }
    return DefaultCommunicationMode::ASK;
}

ThemeMode AppConfigManager::themeFromString(const std::string &value)
{
    return normalize(value) == "dark" ? ThemeMode::DARK : ThemeMode::LIGHT;
}

LanguageMode AppConfigManager::languageFromString(const std::string &value)
{
    const std::string normalized = normalize(value);
    if (normalized == "cz" || normalized == "cs" || normalized == "czech") {
        return LanguageMode::CZECH;
    }
    if (normalized == "de" || normalized == "german") {
        return LanguageMode::GERMAN;
    }
    return LanguageMode::ENGLISH;
}

bool AppConfigManager::load(AppConfig &config, std::string &error)
{
    error.clear();
    config = AppConfig();
    devicePicturesEnabled = config.useDevicePictures;

    if (!storageManager().exists(STORAGE_APP_CONFIG_PATH)) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT,
                        "AppConfigManager::load",
                        "storage read",
                        "config missing path=%s using embedded default",
                        STORAGE_APP_CONFIG_PATH);
        JsonDocument doc;
        const DeserializationError jsonError = deserializeJson(doc, DEFAULT_JSON_CONFIG);
        if (jsonError) {
            error = jsonError.c_str();
            debugLogMessage(DEBUG_VERBOSE_ERRORS,
                            "AppConfigManager::load",
                            "json parse failed",
                            "embedded default config error=%s",
                            error.c_str());
            return false;
        }

        applyConfigDocument(doc, config);
        devicePicturesEnabled = config.useDevicePictures;
        return true;
    }

    File file = storageManager().open(STORAGE_APP_CONFIG_PATH, FILE_READ);
    if (!file) {
        error = "Cannot open app config.";
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "AppConfigManager::load", "storage read failed", "path=%s", STORAGE_APP_CONFIG_PATH);
        return false;
    }

    JsonDocument doc;
    const DeserializationError jsonError = deserializeJson(doc, file);
    file.close();

    if (jsonError) {
        error = jsonError.c_str();
        debugLogMessage(DEBUG_VERBOSE_ERRORS,
                        "AppConfigManager::load",
                        "json parse failed",
                        "path=%s error=%s",
                        STORAGE_APP_CONFIG_PATH,
                        error.c_str());
        return false;
    }

    applyConfigDocument(doc, config);
    devicePicturesEnabled = config.useDevicePictures;

    debugLogMessage(DEBUG_VERBOSE_IMPORTANT,
                    "AppConfigManager::load",
                    "storage read",
                    "firmwareVersion=%s defaultCommunication=%s theme=%s language=%s",
                    EDUBOX_HUB_PANEL_FIRMWARE_VERSION,
                    toString(config.defaultCommunication),
                    toString(config.theme),
                    toString(config.language));
    return true;
}

bool AppConfigManager::save(const AppConfig &config, std::string &error)
{
    error.clear();

    if (!storageManager().ensureDirectory(STORAGE_DATA_DIR)) {
        error = "Cannot prepare app config directory.";
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "AppConfigManager::save", "storage write failed", "directory=%s", STORAGE_DATA_DIR);
        return false;
    }

    if (storageManager().exists(STORAGE_APP_CONFIG_PATH)) {
        storageManager().remove(STORAGE_APP_CONFIG_PATH);
    }

    File file = storageManager().open(STORAGE_APP_CONFIG_PATH, FILE_WRITE);
    if (!file) {
        error = "Cannot open app config for writing.";
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "AppConfigManager::save", "storage write failed", "path=%s", STORAGE_APP_CONFIG_PATH);
        return false;
    }

    JsonDocument doc;
    doc["defaultCommunication"] = toString(config.defaultCommunication);
    JsonObject appearance = doc["appearance"].to<JsonObject>();
    appearance["theme"] = toString(config.theme);
    appearance["language"] = toString(config.language);
    appearance["useDevicePictures"] = config.useDevicePictures;

    const size_t written = serializeJsonPretty(doc, file);
    file.close();

    if (written == 0) {
        error = "App config write produced no data.";
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "AppConfigManager::save", "storage write failed", "path=%s empty write", STORAGE_APP_CONFIG_PATH);
        return false;
    }

    devicePicturesEnabled = config.useDevicePictures;
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT,
                    "AppConfigManager::save",
                    "storage write",
                    "path=%s firmwareVersion=%s defaultCommunication=%s theme=%s language=%s",
                    STORAGE_APP_CONFIG_PATH,
                    EDUBOX_HUB_PANEL_FIRMWARE_VERSION,
                    toString(config.defaultCommunication),
                    toString(config.theme),
                    toString(config.language));
    return true;
}

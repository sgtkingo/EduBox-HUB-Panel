#ifndef APP_CONFIG_MANAGER_HPP
#define APP_CONFIG_MANAGER_HPP

#include "../gui/app_settings.hpp"

#include <string>

struct AppConfig
{
    DefaultCommunicationMode defaultCommunication = DefaultCommunicationMode::ASK;
    ThemeMode theme = ThemeMode::LIGHT;
    LanguageMode language = LanguageMode::ENGLISH;
    bool useDevicePictures = true;
};

class AppConfigManager
{
    static bool devicePicturesEnabled;
public:
    static bool useDevicePictures() { return devicePicturesEnabled; }
    static bool load(AppConfig &config, std::string &error);
    static bool save(const AppConfig &config, std::string &error);

    static const char *toString(DefaultCommunicationMode mode);
    static const char *toString(ThemeMode mode);
    static const char *toString(LanguageMode mode);

    static DefaultCommunicationMode communicationFromString(const std::string &value);
    static ThemeMode themeFromString(const std::string &value);
    static LanguageMode languageFromString(const std::string &value);
};

#endif // APP_CONFIG_MANAGER_HPP

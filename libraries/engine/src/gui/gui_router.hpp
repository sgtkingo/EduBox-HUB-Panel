#ifndef GUI_ROUTER_HPP
#define GUI_ROUTER_HPP

#include <string>
#include <cstdint>

#include "app_settings.hpp"
#include "../devices/json_device_builder.hpp"

class GuiRouter
{
public:
    virtual ~GuiRouter() = default;

    virtual void showConnection() = 0;
    virtual void showMainMenu() = 0;
    virtual void showVisualization() = 0;
    virtual void showSelection() = 0;
    virtual void showLibrary() = 0;
    virtual void showLibraryEditor() = 0;
    virtual void showSettings() = 0;
    virtual void showFileTransfer() = 0;
    virtual void showCreditsScreen() = 0;
    virtual void showAppSelectionScreen() = 0;
    virtual void showCommunicationSelectionScreen() = 0;

    virtual void openVisualizationFlow() = 0;
    virtual void completeCommunicationSelection(DefaultCommunicationMode mode) = 0;
    virtual void openDatabankFromMainMenu() = 0;
    virtual void openDatabankFromVisualization() = 0;
    virtual void navigateBackFromDatabank() = 0;
    virtual void prepareNewLibraryEntity() = 0;
    virtual bool saveLibraryDraft(const DeviceDefinitionSchema &draft,
                                  const std::string &originalUid,
                                  bool isNewEntity,
                                  std::string &error) = 0;
    virtual bool deleteLibraryEntity(const std::string &uid, std::string &error) = 0;
    virtual bool saveCatalogMetadata(const std::string &application,
                                     const std::string &version,
                                     std::string &error) = 0;
    virtual bool saveAppSettings(DefaultCommunicationMode defaultCommunication,
                                 ThemeMode theme,
                                 LanguageMode language,
                                 bool useDevicePictures,
                                 std::string &error) = 0;

    virtual bool shouldSelectionBackGoToMainMenu() const = 0;
    virtual DefaultCommunicationMode getDefaultCommunicationMode() const = 0;
    virtual DefaultCommunicationMode consumeCommunicationAutoStartMode() = 0;
    virtual void setDefaultCommunicationMode(DefaultCommunicationMode mode) = 0;
    virtual ThemeMode getThemeMode() const = 0;
    virtual void setThemeMode(ThemeMode mode) = 0;
    virtual LanguageMode getLanguageMode() const = 0;
    virtual void setLanguageMode(LanguageMode mode) = 0;
    virtual bool getUseDevicePictures() const = 0;
    virtual std::string getAppVersion() const = 0;
    virtual uint32_t getVisualizationUpdatePeriodMs() const = 0;
    virtual void setVisualizationUpdatePeriodMs(uint32_t periodMs) = 0;
    virtual std::string getCatalogApplication() const = 0;
    virtual std::string getCatalogVersion() const = 0;
};

#endif

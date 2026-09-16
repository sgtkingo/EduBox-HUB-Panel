/**
 * @file gui_manager.hpp
 * @brief Navigation orchestrator that coordinates routing, runtime policy and screen registry.
 */

#ifndef GUI_MANAGER_HPP
#define GUI_MANAGER_HPP

#include "../managers/data_bundle_manager.hpp"
#include "../managers/device_browser_state.hpp"
#include "../managers/device_catalog.hpp"
#include "../managers/device_manager.hpp"
#include "../managers/device_visualization_session.hpp"
#include "app_settings.hpp"
#include "gui_navigation_policy.hpp"
#include "gui_router.hpp"
#include "gui_runtime_policy.hpp"
#include "gui_screen_registry.hpp"
#include "gui_state.hpp"
#include "visualization_poll_schedule.hpp"

class GuiManager : public GuiRouter
{
private:
    DeviceCatalog &deviceCatalog;
    DeviceBrowserState &deviceBrowserState;             ///< Shared browse/focus state for catalog screens
    DeviceManager &deviceManager;                       ///< Runtime connection/synchronization manager
    DeviceVisualizationSession &visualizationSession;  ///< Active visualization session
    DataBundleManager &dataBundleManager;              ///< Recording/databank manager
    GuiNavigationPolicy navigationPolicy;              ///< Navigation and back-flow rules
    GuiRuntimePolicy runtimePolicy;                    ///< Side effects when entering/leaving GUI states
    GuiScreenRegistry screenRegistry;                  ///< Screen ownership and rendering registry

    std::string appVersion;                            ///< Firmware version embedded in the build
    GuiState currentState;
    bool initialized;
    VisualizationPollSchedule visualizationPollSchedule;
    uint32_t lastVisualizationDrawMs = 0;

    void hideAllComponents();
    void applyRuntimePolicy(GuiState targetState);
    void renderState(GuiState targetState);

public:
    explicit GuiManager(DeviceCatalog &catalog, DeviceBrowserState &browserState, DeviceManager &manager, DeviceVisualizationSession &visualizationSession, DataBundleManager &dataBundleManager);

    bool init();
    bool init(std::string configFile);
    bool isInitialized() const { return initialized; }

    GuiState getCurrentState() const { return currentState; }

    void showMainMenu() override;
    void showConnection() override;
    void showVisualization() override;
    void showDataBundleSelection();
    void showSelection() override;
    void showLibrary() override;
    void showLibraryEditor() override;
    void showSettings() override;
    void showFileTransfer() override;
    void showCrashScreen(const std::string &reason = "Unexpected error");
    void showCreditsScreen() override;
    void showAppSelectionScreen() override;
    void showCommunicationSelectionScreen() override;

    void navigateTo(GuiState targetState);
    void navigateBack();
    void switchContent(GuiState targetState);
    void redraw();

    void openVisualizationFlow() override;
    void completeCommunicationSelection(DefaultCommunicationMode mode) override;
    void openDatabankFromMainMenu() override;
    void openDatabankFromVisualization() override;
    void navigateBackFromDatabank() override;
    void prepareNewLibraryEntity() override;
    bool saveLibraryDraft(const DeviceDefinitionSchema &draft,
                          const std::string &originalUid,
                          bool isNewEntity,
                          std::string &error) override;
    bool deleteLibraryEntity(const std::string &uid, std::string &error) override;
    bool saveCatalogMetadata(const std::string &application,
                             const std::string &version,
                             std::string &error) override;
    bool saveAppSettings(DefaultCommunicationMode defaultCommunication,
                         ThemeMode theme,
                         LanguageMode language,
                         std::string &error) override;
    bool shouldSelectionBackGoToMainMenu() const override { return navigationPolicy.shouldSelectionBackGoToMainMenu(); }

    DefaultCommunicationMode getDefaultCommunicationMode() const override { return navigationPolicy.getDefaultCommunicationMode(); }
    DefaultCommunicationMode consumeCommunicationAutoStartMode() override { return navigationPolicy.consumeCommunicationAutoStartMode(); }
    void setDefaultCommunicationMode(DefaultCommunicationMode mode) override { navigationPolicy.setDefaultCommunicationMode(mode); }
    ThemeMode getThemeMode() const override { return navigationPolicy.getThemeMode(); }
    void setThemeMode(ThemeMode mode) override { navigationPolicy.setThemeMode(mode); }
    LanguageMode getLanguageMode() const override { return navigationPolicy.getLanguageMode(); }
    void setLanguageMode(LanguageMode mode) override { navigationPolicy.setLanguageMode(mode); }
    std::string getAppVersion() const override { return appVersion; }
    uint32_t getVisualizationUpdatePeriodMs() const override { return visualizationPollSchedule.periodMs(); }
    void setVisualizationUpdatePeriodMs(uint32_t periodMs) override;
    std::string getCatalogApplication() const override { return deviceCatalog.getApplication(); }
    std::string getCatalogVersion() const override { return deviceCatalog.getVersion(); }

    ConnectionGui &getConnectionGui() { return screenRegistry.getConnectionGui(); }
    SignalsVisualizationGui &getVisualizationGui() { return screenRegistry.getVisualizationGui(); }
    DataBundleSelectionGui &getDataBundleSelectionGui() { return screenRegistry.getDataBundleSelectionGui(); }
    DeviceSelectionGui &getSelectionGui() { return screenRegistry.getSelectionGui(); }
    CreditsGui &getCreditsGui() { return screenRegistry.getCreditsGui(); }
    DeviceCatalog &getDeviceCatalog() { return deviceCatalog; }
    DeviceBrowserState &getDeviceBrowserState() { return deviceBrowserState; }
    DeviceManager &getDeviceManager() { return deviceManager; }
    DataBundleManager &getDataBundleManager() { return dataBundleManager; }
    AppSelectionGui &getAppSelectionGui() { return screenRegistry.getAppSelectionGui(); }
    CommunicationSelectionGui &getCommunicationSelectionGui() { return screenRegistry.getCommunicationSelectionGui(); }
};

#endif // GUI_MANAGER_HPP

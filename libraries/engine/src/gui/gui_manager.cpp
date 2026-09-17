/**
 * @file gui_manager.cpp
 * @brief Implementation of the GuiManager class
 */

#include "gui_manager.hpp"

#include "../config.hpp"
#include "../helpers.hpp"
#include "../managers/app_config_manager.hpp"
#include "../managers/storage_manager.hpp"
#include "expt.hpp"

const int FPS = 60;
const int CYCLE_DRAW_MS = (1000 / FPS);

GuiManager::GuiManager(DeviceCatalog &catalog, DeviceBrowserState &browserState, DeviceManager &manager, DeviceVisualizationSession &visualizationSession, DataBundleManager &dataBundleManager)
    : deviceCatalog(catalog),
      deviceBrowserState(browserState),
      deviceManager(manager),
      visualizationSession(visualizationSession),
      dataBundleManager(dataBundleManager),
      navigationPolicy(),
      runtimePolicy(deviceManager),
      screenRegistry(deviceCatalog, deviceBrowserState, *this, deviceManager, visualizationSession, dataBundleManager),
      currentState(GuiState::NONE),
      initialized(false)
{
    appVersion = EDUBOX_HUB_PANEL_FIRMWARE_VERSION;
}

bool GuiManager::init(std::string configFile)
{
    initialized = false;
    currentState = GuiState::NONE;
    debugLogMessage("GuiManager::init", "gui init", "configFile=%s", configFile.c_str());

    try {
        screenRegistry.getCrashGui().init();
        debugLogMessage("GuiManager::init", "gui init", "crash GUI initialized");

        if (!storageManager().init()) {
            const std::string reason =
                std::string(storageManager().getStorageLabel()) +
                " storage mount failed.\nCheck the configured partition scheme and restart the device.";
            screenRegistry.getCrashGui().showCrash(reason.c_str(), "STORAGE ERROR", LV_SYMBOL_SD_CARD);
            return false;
        }

        if (!dataBundleManager.init()) {
            const std::string reason = std::string("Persistent storage is not ready on ") + storageManager().getStorageLabel() + ".";
            screenRegistry.getCrashGui().showCrash(reason.c_str(), "STORAGE ERROR", LV_SYMBOL_SD_CARD);
            return false;
        }

        AppConfig appConfig;
        std::string appConfigError;
        if (AppConfigManager::load(appConfig, appConfigError)) {
            navigationPolicy.setDefaultCommunicationMode(appConfig.defaultCommunication);
            navigationPolicy.setThemeMode(appConfig.theme);
            navigationPolicy.setLanguageMode(appConfig.language);
        } else {
            debugLogMessage(DEBUG_VERBOSE_ERRORS, "GuiManager::init", "app config load failed", "%s", appConfigError.c_str());
        }

        if (!deviceCatalog.init(configFile)) {
            screenRegistry.getCrashGui().showCrash("DeviceCatalog initialization failed!");
            return false;
        }

        deviceBrowserState.clear();
        visualizationSession.clear();
        debugLogMessage("GuiManager::init", "runtime state reset", "browser and visualization sessions cleared");

        if (!deviceManager.init()) {
            screenRegistry.getCrashGui().showCrash("DeviceManager initialization failed!");
            return false;
        }

        screenRegistry.initializeCoreScreens();
    } catch (const Exception &e) {
        e.print();
        showCrashScreen(e.flush());
        return false;
    } catch (const std::exception &e) {
        Exception("GuiManager::init", e.what()).print();
        showCrashScreen(e.what());
        return false;
    } catch (...) {
        Exception("GuiManager::init", "Unknown exception during GUI initialization").print();
        showCrashScreen("Unknown exception during GUI initialization!");
        return false;
    }

    currentState = GuiState::READY;
    initialized = true;
    debugLogMessage("GuiManager::init", "gui init", "initialization completed");
    return initialized;
}

bool GuiManager::init()
{
    return init("");
}

void GuiManager::hideAllComponents()
{
    debugLogMessage("GuiManager::hideAllComponents", "gui operation", "hiding all components");
    screenRegistry.hideAll();
}

void GuiManager::showMainMenu()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::MAIN_MENU);
}

void GuiManager::showConnection()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::CONNECTION);
}

void GuiManager::showVisualization()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::VISUALIZATION);
}

void GuiManager::showDataBundleSelection()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::DATA_BUNDLE_SELECTION);
}

void GuiManager::showSelection()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::SELECTION);
}

void GuiManager::showLibrary()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::LIBRARY);
}

void GuiManager::showLibraryEditor()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::LIBRARY_EDITOR);
}

void GuiManager::showSettings()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::SETTINGS);
}

void GuiManager::showFileTransfer()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::FILE_TRANSFER);
}

void GuiManager::showCrashScreen(const std::string &reason)
{
    debugLogMessage("GuiManager::showCrashScreen", "gui crash", "%s", reason.c_str());
    if (!initialized) {
        deviceManager.setRunning(false);
        hideAllComponents();
        screenRegistry.getCrashGui().showCrash(reason);
        currentState = GuiState::CRASH;
        return;
    }

    navigateTo(GuiState::CRASH);
    screenRegistry.getCrashGui().showCrash(reason);
}

void GuiManager::showCreditsScreen()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::CREDITS);
}

void GuiManager::showAppSelectionScreen()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::APP_SELECTION);
}

void GuiManager::showCommunicationSelectionScreen()
{
    if (!initialized) {
        return;
    }

    navigateTo(GuiState::COMMUNICATION_SELECTION);
}

void GuiManager::applyRuntimePolicy(GuiState targetState)
{
    runtimePolicy.apply(targetState);
}

void GuiManager::renderState(GuiState targetState)
{
    if (!initialized) {
        return;
    }

    debugLogMessage("GuiManager::renderState", "gui operation", "targetState=%d", static_cast<int>(targetState));
    screenRegistry.render(targetState);
}

void GuiManager::navigateTo(GuiState targetState)
{
    if (!initialized) {
        return;
    }

    if (currentState == targetState) {
        return;
    }

    debugLogMessage("GuiManager::navigateTo", "gui navigation", "from=%d to=%d", static_cast<int>(currentState), static_cast<int>(targetState));
    connectionFeedback.hideDisconnect();
    applyRuntimePolicy(targetState);
    if (targetState == GuiState::MAIN_MENU) {
        deviceManager.endProtocolSession();
    }
    renderState(targetState);
    currentState = targetState;
}

void GuiManager::navigateBack()
{
    navigateTo(navigationPolicy.resolveBackTarget(currentState));
}

void GuiManager::switchContent(GuiState targetState)
{
    navigateTo(targetState);
}

void GuiManager::openVisualizationFlow()
{
    navigateTo(navigationPolicy.beginVisualizationFlow());
    if (currentState == GuiState::COMMUNICATION_SELECTION) {
        screenRegistry.getCommunicationSelectionGui().applyDefaultCommunicationMode();
    }
}

void GuiManager::completeCommunicationSelection(DefaultCommunicationMode mode)
{
    navigateTo(navigationPolicy.finishCommunicationSelection(mode));
}

void GuiManager::openDatabankFromMainMenu()
{
    navigateTo(navigationPolicy.openDatabankFromMainMenu());
}

void GuiManager::openDatabankFromVisualization()
{
    navigateTo(navigationPolicy.openDatabankFromVisualization());
}

void GuiManager::navigateBackFromDatabank()
{
    navigateBack();
}

void GuiManager::prepareNewLibraryEntity()
{
    deviceBrowserState.beginNewLibraryDraft();
}

bool GuiManager::saveLibraryDraft(const DeviceDefinitionSchema &draft,
                                  const std::string &originalUid,
                                  bool isNewEntity,
                                  std::string &error)
{
    try {
        deviceCatalog.saveDraft(draft, originalUid, isNewEntity);
        debugLogMessage("GuiManager::saveLibraryDraft", "storage write", "uid=%s originalUid=%s isNew=%d", draft.uid.c_str(), originalUid.c_str(), isNewEntity);
        deviceManager.erase();
        visualizationSession.clear();
        deviceBrowserState.clear();
        deviceBrowserState.setLibraryDevice(deviceCatalog.getDevice(draft.uid));
        deviceBrowserState.setLibraryDraft(draft, false);
        return true;
    } catch (const std::exception &e) {
        Exception("GuiManager::saveLibraryDraft", e.what()).print();
        error = e.what();
        return false;
    } catch (...) {
        Exception("GuiManager::saveLibraryDraft", "Unknown error while saving device draft.").print();
        error = "Unknown error while saving device draft.";
        return false;
    }
}

bool GuiManager::deleteLibraryEntity(const std::string &uid, std::string &error)
{
    try {
        deviceCatalog.deleteDevice(uid);
        debugLogMessage("GuiManager::deleteLibraryEntity", "storage write", "uid=%s", uid.c_str());
        deviceManager.erase();
        visualizationSession.clear();
        deviceBrowserState.clear();
        return true;
    } catch (const std::exception &e) {
        Exception("GuiManager::deleteLibraryEntity", e.what()).print();
        error = e.what();
        return false;
    } catch (...) {
        Exception("GuiManager::deleteLibraryEntity", "Unknown error while deleting device entity.").print();
        error = "Unknown error while deleting device entity.";
        return false;
    }
}

bool GuiManager::saveCatalogMetadata(const std::string &application,
                                     const std::string &version,
                                     std::string &error)
{
    try {
        BaseDevice *selectedSelectionDevice = deviceBrowserState.getSelectionDevice();
        BaseDevice *selectedLibraryDevice = deviceBrowserState.getLibraryDevice();
        const std::string selectionUid = selectedSelectionDevice ? selectedSelectionDevice->UID : "";
        const std::string libraryUid = selectedLibraryDevice ? selectedLibraryDevice->UID : "";

        deviceCatalog.saveMetadata(application, version);
        debugLogMessage("GuiManager::saveCatalogMetadata", "storage write", "application=%s version=%s", application.c_str(), version.c_str());
        deviceManager.erase();
        visualizationSession.clear();
        deviceBrowserState.clear();
        if (!selectionUid.empty()) {
            deviceBrowserState.setSelectionDevice(deviceCatalog.getDevice(selectionUid));
        }
        if (!libraryUid.empty()) {
            deviceBrowserState.setLibraryDevice(deviceCatalog.getDevice(libraryUid));
        }
        return true;
    } catch (const std::exception &e) {
        Exception("GuiManager::saveCatalogMetadata", e.what()).print();
        error = e.what();
        return false;
    } catch (...) {
        Exception("GuiManager::saveCatalogMetadata", "Unknown error while saving catalog metadata.").print();
        error = "Unknown error while saving catalog metadata.";
        return false;
    }
}

bool GuiManager::saveAppSettings(DefaultCommunicationMode defaultCommunication,
                                 ThemeMode theme,
                                 LanguageMode language,
                                 bool useDevicePictures,
                                 std::string &error)
{
    try {
        AppConfig config;
        config.defaultCommunication = defaultCommunication;
        config.theme = theme;
        config.language = language;
        config.useDevicePictures = useDevicePictures;

        if (!AppConfigManager::save(config, error)) {
            return false;
        }

        navigationPolicy.setDefaultCommunicationMode(defaultCommunication);
        navigationPolicy.setThemeMode(theme);
        navigationPolicy.setLanguageMode(language);
        return true;
    } catch (const std::exception &e) {
        Exception("GuiManager::saveAppSettings", e.what()).print();
        error = e.what();
        return false;
    } catch (...) {
        Exception("GuiManager::saveAppSettings", "Unknown error while saving app settings.").print();
        error = "Unknown error while saving app settings.";
        return false;
    }
}

void GuiManager::setVisualizationUpdatePeriodMs(uint32_t periodMs)
{
    visualizationPollSchedule.setPeriod(periodMs, lv_tick_get());
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "GuiManager::setVisualizationUpdatePeriodMs", "update period changed", "periodMs=%u",
                    static_cast<unsigned>(visualizationPollSchedule.periodMs()));
}

void GuiManager::updateConnectionFeedback()
{
    if (!deviceManager.hasLostConnection()) {
        connectionFeedback.hideDisconnect();
        return;
    }
    deviceManager.setRunning(false);
    connectionFeedback.showDisconnect(lv_scr_act(), this, [](lv_event_t *event) {
        auto *self = static_cast<GuiManager*>(lv_event_get_user_data(event));
        self->reconnectProtocolSession();
    });
}

void GuiManager::reconnectProtocolSession()
{
    const bool connected = currentState == GuiState::VISUALIZATION
        ? screenRegistry.getVisualizationGui().reconnectAfterDisconnect()
        : deviceManager.reconnectProtocolLink();
    if (!connected) {
        splashMessage("Reconnect failed. Check Board power and selected UART/BLE link, then retry.");
        return;
    }
    connectionFeedback.hideDisconnect();
    visualizationPollSchedule.reset(lv_tick_get());
}

void GuiManager::redraw()
{
    lv_timer_handler();

    if (!initialized) {
        delay_ms(1);
        return;
    }

    const uint32_t now = lv_tick_get();
    if (currentState == GuiState::VISUALIZATION && deviceManager.isRunning()) {
        if (visualizationPollSchedule.due(now)) {
            deviceManager.resync(visualizationSession.getCurrentDevice());
        }
    } else {
        visualizationPollSchedule.reset(now);
    }

    switch (currentState) {
    case GuiState::VISUALIZATION:
        if (screenRegistry.getVisualizationGui().isInitialized() &&
            static_cast<uint32_t>(now - lastVisualizationDrawMs) >= CYCLE_DRAW_MS) {
            lastVisualizationDrawMs = now;
            screenRegistry.getVisualizationGui().drawCurrentDevice();
        }
        break;
    default:
        break;
    }
    // Link loss is global, including selection and a paused online visualization.
    updateConnectionFeedback();
    // Yield to the RTOS without imposing the display frame period on polling.
    delay_ms(1);
}

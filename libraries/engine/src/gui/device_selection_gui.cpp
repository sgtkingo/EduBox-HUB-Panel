#include "device_selection_gui.hpp"

#include "../helpers.hpp"

#ifndef LV_SYMBOL_PLUG
#define LV_SYMBOL_PLUG LV_SYMBOL_USB
#endif

namespace
{
std::string buildPinProgressLabel(const BaseDevice *device)
{
    if (!device) {
        return "";
    }

    return std::to_string(device->getAssignedPinCount()) + "/" + std::to_string(device->getRequiredPinCount()) + " pins";
}

uint32_t getDeviceAssignmentColor(const BaseDevice *device)
{
    if (!device || device->getAssignedPinCount() == 0) {
        return 0xFFFFFF;
    }
    return device->isPinAssignmentComplete() ? 0xE6F4EA : 0xFFF8D6;
}

uint32_t getDeviceAssignmentBorderColor(const BaseDevice *device)
{
    if (!device || device->getAssignedPinCount() == 0) {
        return 0xD9DDE3;
    }
    return device->isPinAssignmentComplete() ? 0x2EAD5F : 0xF2C94C;
}

void addConnectedIcon(lv_obj_t *button)
{
    if (!button) {
        return;
    }

    lv_obj_t *icon = lv_label_create(button);
    lv_label_set_text(icon, LV_SYMBOL_PLUG);
    lv_obj_align(icon, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x1B8F4D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
}
}

DeviceSelectionGui::DeviceSelectionGui(DeviceCatalog &deviceCatalog, DeviceBrowserState &browserState, GuiRouter &router, DeviceManager &deviceManager, DeviceVisualizationSession &visualizationSession)
    : catalogBrowser(deviceCatalog), browserState(browserState), router(router), deviceManager(deviceManager), visualizationSession(visualizationSession)
{
}

void DeviceSelectionGui::buildSelectionGui()
{
    const DeviceCatalogBrowserLayout layout = DeviceCatalogBrowserLayoutFactory::createLayout({
        "Please select devices to connect.",
        0xF5F5F5,
        false
    });

    ui_SelectionWidget = layout.root;
    ui_AvailableList = layout.primaryList;
    ui_SelectedList = layout.secondaryList;
    lv_obj_t *infoPanel = layout.detailPanel;
    ui_DeviceDescription = layout.detailLabel;

    constexpr lv_coord_t contentY = 54;
    constexpr lv_coord_t contentHeight = 310;
    lv_obj_set_size(ui_AvailableList, 300, contentHeight);
    lv_obj_set_pos(ui_AvailableList, 12, contentY);
    lv_obj_set_size(infoPanel, 410, contentHeight);
    lv_obj_set_pos(infoPanel, 320, contentY);
    ui_DeviceDescription = infoPanel;

    ui_btnBack = DeviceCatalogBrowserLayoutFactory::createFooterButton(ui_SelectionWidget, "Back", LV_ALIGN_BOTTOM_LEFT, 12, -16);
    lv_obj_set_size(ui_btnBack, 90, 36);
    lv_obj_add_event_cb(ui_btnBack, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }
        auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
        self->handleBackButtonClick();
    }, LV_EVENT_ALL, this);

    ui_btnShowPinsMap = DeviceCatalogBrowserLayoutFactory::createFooterButton(ui_SelectionWidget, "Show Pins Map", LV_ALIGN_TOP_RIGHT, -12, 10);
    lv_obj_set_size(ui_btnShowPinsMap, 132, 34);
    lv_obj_set_style_bg_color(ui_btnShowPinsMap, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_btnShowPinsMap, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }
        auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
        self->handleShowPinsMapButtonClick();
    }, LV_EVENT_ALL, this);

    ui_btnConfigure = DeviceCatalogBrowserLayoutFactory::createFooterButton(ui_SelectionWidget, "Config", LV_ALIGN_BOTTOM_LEFT, 110, -16);
    lv_obj_set_size(ui_btnConfigure, 100, 36);
    lv_obj_set_style_bg_color(ui_btnConfigure, lv_color_hex(0x8C939D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_btnConfigure, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }
        auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
        self->handleConfigureButtonClick();
    }, LV_EVENT_ALL, this);

    ui_btnPins = DeviceCatalogBrowserLayoutFactory::createFooterButton(ui_SelectionWidget, "Connect", LV_ALIGN_BOTTOM_LEFT, 250, -16);
    lv_obj_set_size(ui_btnPins, 130, 36);
    ui_btnPinsLabel = lv_obj_get_child(ui_btnPins, 0);
    lv_obj_add_event_cb(ui_btnPins, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }
        auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
        self->handleConnectActionButtonClick();
    }, LV_EVENT_ALL, this);

    ui_btnStart = DeviceCatalogBrowserLayoutFactory::createFooterButton(ui_SelectionWidget, "Start Visualization", LV_ALIGN_BOTTOM_RIGHT, -10, -16);
    lv_obj_set_size(ui_btnStart, 150, 46);
    lv_obj_add_event_cb(ui_btnStart, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }
        auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
        self->handleStartButtonClick();
    }, LV_EVENT_ALL, this);
}

void DeviceSelectionGui::populateAvailableList()
{
    if (!ui_AvailableList) {
        return;
    }

    lv_obj_clean(ui_AvailableList);
    const auto &devices = catalogBrowser.getDevices();
    for (size_t i = 0; i < devices.size(); ++i) {
        BaseDevice *device = devices[i];
        const std::string label = DeviceCatalogBrowserFormatter::buildDeviceListLabel(device);
        lv_obj_t *button = lv_list_add_btn(ui_AvailableList, nullptr, label.c_str());
        DeviceCatalogBrowserRenderer::styleDeviceListButton(button);
        lv_obj_set_style_bg_color(button, lv_color_hex(getDeviceAssignmentColor(device)), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(button, lv_color_hex(getDeviceAssignmentBorderColor(device)), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(button, lv_color_hex(0x1C1F23), LV_PART_MAIN | LV_STATE_DEFAULT);
        if (device && device->isPinConnectionActive()) {
            addConnectedIcon(button);
        }
        lv_obj_set_user_data(button, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(button, [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
                return;
            }

            auto *self = static_cast<DeviceSelectionGui *>(lv_event_get_user_data(e));
            int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
            self->handleDeviceSelection(index);
        }, LV_EVENT_ALL, this);
    }
}

void DeviceSelectionGui::populateSelectedList()
{
    // Pin state is now visible directly in the main device list and in the Pins map preview.
}

void DeviceSelectionGui::updateDeviceInfo()
{
    BaseDevice *device = catalogBrowser.getSelectedDevice();
    if (!device) {
        DeviceCatalogBrowserRenderer::renderDeviceDetail(ui_DeviceDescription, nullptr);
        updateConnectActionButtonState();
        return;
    }

    browserState.setSelectionDevice(device);
    DeviceCatalogBrowserRenderer::renderDeviceDetail(ui_DeviceDescription, device, buildPinProgressLabel(device));
    updateConnectActionButtonState();
}

void DeviceSelectionGui::updateStartButtonState()
{
    if (!ui_btnStart) {
        return;
    }

    if (deviceManager.hasConnectedAssignedDevices()) {
        lv_obj_clear_state(ui_btnStart, LV_STATE_DISABLED);
        return;
    }

    lv_obj_add_state(ui_btnStart, LV_STATE_DISABLED);
}

void DeviceSelectionGui::updateConnectActionButtonState()
{
    if (!ui_btnPins || !ui_btnPinsLabel) {
        return;
    }

    BaseDevice *device = getSelectedDevice();
    if (!device) {
        lv_label_set_text(ui_btnPinsLabel, "Connect");
        lv_obj_set_style_bg_color(ui_btnPins, lv_color_hex(0x8C939D), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_state(ui_btnPins, LV_STATE_DISABLED);
        return;
    }

    lv_obj_clear_state(ui_btnPins, LV_STATE_DISABLED);
    if (device->isPinConnectionActive()) {
        lv_label_set_text(ui_btnPinsLabel, "Disconnect");
        lv_obj_set_style_bg_color(ui_btnPins, lv_color_hex(0xD96464), LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }

    lv_label_set_text(ui_btnPinsLabel, "Connect");
    lv_obj_set_style_bg_color(ui_btnPins, lv_color_hex(0x2EAD5F), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void DeviceSelectionGui::handleDeviceSelection(int deviceIndex)
{
    catalogBrowser.setSelectedDeviceIndex(deviceIndex);
    updateDeviceInfo();
}

void DeviceSelectionGui::handleConfigureButtonClick()
{
    BaseDevice *device = getSelectedDevice();
    if (!device) {
        splashMessage("Select a device first.");
        return;
    }

    browserState.setLibraryDevice(device);
    if (!browserState.beginLibraryDraftFromLibraryDevice()) {
        splashMessage("Cannot open selected entity in Library.");
        return;
    }
    router.showLibraryEditor();
}

void DeviceSelectionGui::handleConnectActionButtonClick()
{
    BaseDevice *device = getSelectedDevice();
    if (!device) {
        splashMessage("Select a device first.");
        return;
    }

    if (device->isPinConnectionActive()) {
        if (!deviceManager.disconnectAndUnassignDevice(device)) {
            const std::string error = deviceManager.getLastError();
            splashMessage(error.empty() ? "DISCONNECT failed without an error detail from the Board." : error.c_str());
            return;
        }

        populateSelectedList();
        populateAvailableList();
        updateDeviceInfo();
        updateStartButtonState();
        return;
    }

    browserState.setPinMapPreviewMode(false);
    browserState.setSelectionDevice(device);
    router.showConnection();
}

void DeviceSelectionGui::handleStartButtonClick()
{
    deviceManager.setRunning(false);
    browserState.setSelectionDevice(nullptr);

    if (!deviceManager.hasCompleteAssignedDevices()) {
        const auto incompleteDevices = deviceManager.getIncompleteAssignedDevices();
        if (!incompleteDevices.empty()) {
            BaseDevice *device = incompleteDevices.front();
            const std::string message = device->getTypeName() + ": missing " +
                                        std::to_string(device->getMissingPinCount()) + " of " +
                                        std::to_string(device->getRequiredPinCount()) + " required pins.";
            splashMessage(message.c_str());
        } else {
            splashMessage("No devices fully assigned.");
        }
        return;
    }

    const auto connectedDevices = deviceManager.getConnectedAssignedDevices();
    if (connectedDevices.empty()) {
        splashMessage("Open Pins and press Connect first.");
        return;
    }

    visualizationSession.setDevices(connectedDevices);
    router.showVisualization();
}

void DeviceSelectionGui::handleShowPinsMapButtonClick()
{
    browserState.setSelectionDevice(nullptr);
    browserState.setPinMapPreviewMode(true);
    router.showConnection();
}

void DeviceSelectionGui::handleBackButtonClick()
{
    if (router.shouldSelectionBackGoToMainMenu()) {
        router.showMainMenu();
        return;
    }

    router.showCommunicationSelectionScreen();
}

void DeviceSelectionGui::init()
{
    if (initialized) {
        return;
    }

    buildSelectionGui();
    initialized = true;
}

void DeviceSelectionGui::showSelection()
{
    if (!initialized) {
        return;
    }

    populateAvailableList();
    populateSelectedList();
    updateDeviceInfo();
    updateStartButtonState();
    updateConnectActionButtonState();
    lv_obj_clear_flag(ui_SelectionWidget, LV_OBJ_FLAG_HIDDEN);
}

void DeviceSelectionGui::hideSelection()
{
    if (!initialized || !ui_SelectionWidget) {
        return;
    }

    lv_obj_add_flag(ui_SelectionWidget, LV_OBJ_FLAG_HIDDEN);
}

BaseDevice *DeviceSelectionGui::getSelectedDevice()
{
    return catalogBrowser.getSelectedDevice();
}

void DeviceSelectionGui::setSelectedDevice(int index)
{
    catalogBrowser.setSelectedDeviceIndex(index);
    updateDeviceInfo();
}

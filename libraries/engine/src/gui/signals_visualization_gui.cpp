/**
 * @file signals_visualization_gui.cpp
 * @brief Implementation of the SignalsVisualizationGui class
 *
 * This source file implements the SignalsVisualizationGui functionality for
 * active device visualization, data display, and navigation.
 *
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny, Ondřej Wrubel
 */

#include "signals_visualization_gui.hpp"
#include "../helpers.hpp"
#include "./images/ui_images.h"
#include "expt.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>

extern int touch_point_count __attribute__((weak));
extern int touch_last_x __attribute__((weak));
extern int touch_last_y __attribute__((weak));
extern int touch_second_x __attribute__((weak));
extern int touch_second_y __attribute__((weak));

namespace
{
constexpr uint32_t SIGNAL_CARD_COLORS[] = {
    0x009BFF,
    0xFF6B35,
    0x00B894,
    0x9B51E0,
    0xF2C94C,
    0xEB5757
};

std::vector<std::string> splitOptionsCsv(const std::string &options)
{
    std::vector<std::string> result;
    for (const auto &item : splitString(options, ',')) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    return result;
}
}

SignalsVisualizationGui::SignalsVisualizationGui(GuiRouter &router, DeviceManager &deviceManager, DeviceVisualizationSession &visualizationSession, DataBundleManager &dataBundleManager)
                                              : router(router), deviceManager(deviceManager), visualizationSession(visualizationSession), dataBundleManager(dataBundleManager)
{
    // Initialize all GUI pointers to nullptr
    ui_DeviceWidget = nullptr;
    ui_DeviceLabel = nullptr;
    ui_ListModeButton = nullptr;
    ui_ListModeButtonLabel = nullptr;
    ui_ListModeTitle = nullptr;
    ui_LogoGroup = nullptr;
    ui_LogoCornerBottomLeft = nullptr;
    ui_LogoCornerFillBottomLeft = nullptr;
    ui_LogoCornerTopRight = nullptr;
    ui_LogoCornerFillBottomRight = nullptr;
    ui_LogoOutlay = nullptr;
    ui_LogoImage = nullptr;
}

void SignalsVisualizationGui::init()
{
    if (initialized)
    {
        debugLogMessage("SignalsVisualizationGui::init", "gui init skipped", "already initialized");
        return;
    }

    try
    {
        debugLogMessage("SignalsVisualizationGui::init", "gui init", "initializing");
        constructVisualization();
        initialized = true;
        debugLogMessage("SignalsVisualizationGui::init", "gui init", "initialization completed");
    }
    catch (const std::exception &e)
    {
        Exception("SignalsVisualizationGui::init", e.what()).print();
        initialized = false;
    }
}

void SignalsVisualizationGui::createMainWidget()
{
    ui_DeviceWidget = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_DeviceWidget);
    lv_obj_set_width(ui_DeviceWidget, 760);
    lv_obj_set_height(ui_DeviceWidget, 440);
    lv_obj_set_align(ui_DeviceWidget, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_DeviceWidget, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                           LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                                           LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_radius(ui_DeviceWidget, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_DeviceWidget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DeviceWidget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_DeviceWidget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_DeviceWidget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_DeviceWidget, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void SignalsVisualizationGui::createTitleLabel()
{
    ui_DeviceLabel = lv_label_create(ui_DeviceWidget);
    lv_obj_set_width(ui_DeviceLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_DeviceLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_DeviceLabel, 0);
    lv_obj_set_y(ui_DeviceLabel, 10);
    lv_obj_set_align(ui_DeviceLabel, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_DeviceLabel, "DHT11");
    lv_obj_clear_flag(ui_DeviceLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE |
                                          LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM |
                                          LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_text_color(ui_DeviceLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_DeviceLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DeviceLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void SignalsVisualizationGui::createSignalScrollPanel()
{
    signalListPanel.create(
        ui_DeviceWidget,
        this,
        [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
                return;
            }

            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
            self->handleDropdownConfigChanged(static_cast<size_t>(index));
        },
        [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
                return;
            }

            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
            self->handleSliderConfigChanged(static_cast<size_t>(index));
        },
        [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_READY && lv_event_get_code(e) != LV_EVENT_DEFOCUSED) {
                return;
            }

            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
            self->handleTextConfigSubmitted(static_cast<size_t>(index));
        });
}

void SignalsVisualizationGui::createListModeToggle()
{
    ui_ListModeTitle = lv_label_create(ui_DeviceWidget);
    lv_obj_set_width(ui_ListModeTitle, 160);
    lv_obj_align(ui_ListModeTitle, LV_ALIGN_TOP_LEFT, 36, 56);
    lv_obj_set_style_text_font(ui_ListModeTitle, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_ListModeTitle, lv_color_hex(0x24415E), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ListModeButton = lv_btn_create(ui_DeviceWidget);
    lv_obj_set_size(ui_ListModeButton, 34, 30);
    lv_obj_align(ui_ListModeButton, LV_ALIGN_TOP_LEFT, 198, 50);
    lv_obj_add_event_cb(ui_ListModeButton, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
        self->toggleListMode();
    }, LV_EVENT_ALL, this);

    ui_ListModeButtonLabel = lv_label_create(ui_ListModeButton);
    lv_label_set_text(ui_ListModeButtonLabel, LV_SYMBOL_LOOP);
    lv_obj_center(ui_ListModeButtonLabel);
}

void SignalsVisualizationGui::createChartPanel()
{
    chartPanel.create(ui_DeviceWidget);
    lv_obj_t *chart = chartPanel.getChart();
    if (chart) {
        lv_obj_add_flag(chart, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(chart, [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleChartDrag(e);
        }, LV_EVENT_ALL, this);
    }
}

void SignalsVisualizationGui::constructVisualization()
{
    debugLogMessage("SignalsVisualizationGui::constructVisualization", "gui operation", "constructing visualization widgets");

    createMainWidget();
    createTitleLabel();
    createSignalScrollPanel();
    createListModeToggle();
    createChartPanel();
    createToolbarPanel();
    addLogoPanelToWidget(ui_DeviceWidget);

    debugLogMessage("SignalsVisualizationGui::constructVisualization", "gui operation", "visualization widgets constructed");
}

void SignalsVisualizationGui::createToolbarPanel()
{
    toolbarPanel.create(
        ui_DeviceWidget,
        this,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->goToPreviousDevice();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->goToNextDevice();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleBackButtonClick();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handlePauseButtonClick();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleSyncButtonClick();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleRecordButtonClick(nullptr);
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleClearButtonClick();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleSettingsButtonClick(self->toolbarPanel.getRecordGroup(), self->toolbarPanel.getSettingsButton(), self->getParentWidget());
        });
}

void SignalsVisualizationGui::showShadowOverlay()
{
    feedbackPanel.showShadowOverlay();
}

void SignalsVisualizationGui::hideShadowOverlay()
{
    feedbackPanel.hideShadowOverlay();
}

void SignalsVisualizationGui::addLogoPanelToWidget(lv_obj_t *parentWidget)
{
    ui_LogoGroup = lv_obj_create(parentWidget);
    lv_obj_remove_style_all(ui_LogoGroup);
    lv_obj_set_width(ui_LogoGroup, 100);
    lv_obj_set_height(ui_LogoGroup, 20);
    lv_obj_set_x(ui_LogoGroup, 9);
    lv_obj_set_y(ui_LogoGroup, 0);
    lv_obj_set_align(ui_LogoGroup, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_clear_flag(ui_LogoGroup, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_LogoCornerBottomLeft = lv_obj_create(ui_LogoGroup);
    lv_obj_remove_style_all(ui_LogoCornerBottomLeft);
    lv_obj_set_width(ui_LogoCornerBottomLeft, 10);
    lv_obj_set_height(ui_LogoCornerBottomLeft, 10);
    lv_obj_set_x(ui_LogoCornerBottomLeft, 10);
    lv_obj_set_y(ui_LogoCornerBottomLeft, 0);
    lv_obj_set_align(ui_LogoCornerBottomLeft, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_clear_flag(ui_LogoCornerBottomLeft, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_bg_color(ui_LogoCornerBottomLeft, lv_color_hex(0x055DA9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LogoCornerBottomLeft, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui_LogoCornerBottomLeft, false, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LogoCornerTopRight = lv_obj_create(ui_LogoGroup);
    lv_obj_remove_style_all(ui_LogoCornerTopRight);
    lv_obj_set_width(ui_LogoCornerTopRight, 10);
    lv_obj_set_height(ui_LogoCornerTopRight, 10);
    lv_obj_set_x(ui_LogoCornerTopRight, -9);
    lv_obj_set_y(ui_LogoCornerTopRight, 0);
    lv_obj_set_align(ui_LogoCornerTopRight, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_LogoCornerTopRight, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_bg_color(ui_LogoCornerTopRight, lv_color_hex(0x055DA9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LogoCornerTopRight, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui_LogoCornerTopRight, false, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LogoOutlay = lv_obj_create(ui_LogoGroup);
    lv_obj_remove_style_all(ui_LogoOutlay);
    lv_obj_set_width(ui_LogoOutlay, 80);
    lv_obj_set_height(ui_LogoOutlay, 20);
    lv_obj_set_align(ui_LogoOutlay, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_LogoOutlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_obj_set_style_radius(ui_LogoOutlay, 1000, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LogoOutlay, lv_color_hex(0x055DA9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LogoOutlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui_LogoOutlay, false, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LogoImage = lv_img_create(ui_LogoGroup);
    lv_img_set_src(ui_LogoImage, &ui_img_mtalogo_png);
    lv_obj_set_width(ui_LogoImage, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_LogoImage, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(ui_LogoImage, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_LogoImage, LV_OBJ_FLAG_ADV_HITTEST);  /// Flags
    lv_obj_clear_flag(ui_LogoImage, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    lv_img_set_zoom(ui_LogoImage, 70);
}

void SignalsVisualizationGui::drawCurrentDevice(bool force)
{
    if (deviceManager.hasLostConnection()) {
        paused = true;
        deviceManager.setRunning(false);
        toolbarPanel.setPaused(true);
        return;
    }

    if (!currentDevice)
    {
        debugLogMessage("SignalsVisualizationGui::drawCurrentDevice", "gui redraw skipped", "no current device");
        return;
    }

    if (!force && !currentDevice->getRedrawPending())
    {
        return;
    }

    if (!force && paused)
    {
        return;
    }

    updateDeviceDataDisplay();
    updateChart(force);
    debugLogMessage("SignalsVisualizationGui::drawCurrentDevice", "gui redraw", "device=%s force=%d", currentDevice->UID.c_str(), force);
    currentDevice->setRedrawPending(false);
}

uint32_t SignalsVisualizationGui::getSignalAccentColor(size_t index)
{
    return SIGNAL_CARD_COLORS[index % (sizeof(SIGNAL_CARD_COLORS) / sizeof(SIGNAL_CARD_COLORS[0]))];
}

bool SignalsVisualizationGui::isNumericType(DeviceDataType dtype)
{
    return dtype == DeviceDataType::INT || dtype == DeviceDataType::FLOAT || dtype == DeviceDataType::DOUBLE;
}

bool SignalsVisualizationGui::hasSelectableOptions(const DeviceParam &param)
{
    return !param.Restrictions.Options.empty();
}

bool SignalsVisualizationGui::supportsSliderInput(const DeviceParam &param)
{
    return param.DType == DeviceDataType::INT &&
           !param.Restrictions.Min.empty() &&
           !param.Restrictions.Max.empty();
}

std::string SignalsVisualizationGui::buildUnitText(const std::string &unit, const char *fallbackText)
{
    if (unit.empty()) {
        return fallbackText ? std::string(fallbackText) : std::string();
    }

    return "[" + unit + "]";
}

bool SignalsVisualizationGui::currentDeviceSupportsRecording() const
{
    return currentDevice && !getRecordableValueKeys().empty();
}

void SignalsVisualizationGui::updateListModeToggleState()
{
    if (ui_ListModeTitle) {
        lv_label_set_text(ui_ListModeTitle, showingConfigPanel ? "Configs" : "Values");
    }
}

void SignalsVisualizationGui::toggleListMode()
{
    showingConfigPanel = !showingConfigPanel;
    updateDeviceDataDisplay();
}

void SignalsVisualizationGui::ensureSignalCards(size_t count)
{
    signalListPanel.ensureSignalCards(count);
}

void SignalsVisualizationGui::clearUnusedSignalCards(size_t usedCount)
{
    signalListPanel.clearUnusedSignalCards(usedCount);
}

void SignalsVisualizationGui::ensureConfigControls(size_t count)
{
    signalListPanel.ensureConfigControls(count);
}

void SignalsVisualizationGui::clearUnusedConfigControls(size_t usedCount)
{
    signalListPanel.clearUnusedConfigControls(usedCount);
}

void SignalsVisualizationGui::updateDeviceTitle()
{
    if (!currentDevice || !ui_DeviceLabel) {
        return;
    }

    const std::string title = currentDevice->getName() + " [" + currentDevice->getRoleLabel() + "]";
    lv_label_set_text(ui_DeviceLabel, title.c_str());
}

void SignalsVisualizationGui::updateSignalCards(const std::unordered_map<std::string, DeviceParam> &values,
                                                const std::vector<std::string> &valueKeys)
{
    ensureSignalCards(valueKeys.size());

    for (size_t i = 0; i < valueKeys.size(); ++i) {
        const auto &key = valueKeys[i];
        auto it = values.find(key);
        if (it == values.end()) {
            continue;
        }

        const std::string units = buildUnitText(currentDevice->getValueUnits(key), "Live value");
        signalListPanel.setSignalCard(i, getSignalAccentColor(i), key, it->second.Value, units);
    }

    clearUnusedSignalCards(valueKeys.size());
}

void SignalsVisualizationGui::ensureControlEditor(size_t controlIndex, const DeviceParam &param)
{
    auto *control = signalListPanel.getConfigControl(controlIndex);
    if (!control) {
        return;
    }

    if (hasSelectableOptions(param)) {
        std::string optionsText;
        int selectedIndex = 0;
        int currentIndex = 0;
        for (const auto &option : splitOptionsCsv(param.Restrictions.Options)) {
            if (!optionsText.empty()) {
                optionsText += "\n";
            }
            optionsText += option;
            if (option == param.Value) {
                selectedIndex = currentIndex;
            }
            ++currentIndex;
        }
        signalListPanel.ensureDropdownEditor(controlIndex, optionsText, static_cast<uint16_t>(selectedIndex));
    } else if (supportsSliderInput(param)) {
        signalListPanel.ensureSliderEditor(
            controlIndex,
            convertStringToType<int>(param.Restrictions.Min),
            convertStringToType<int>(param.Restrictions.Max),
            convertStringToType<int>(param.Value));
    } else {
        signalListPanel.ensureTextEditor(controlIndex, param.Value);
    }
}

void SignalsVisualizationGui::syncControlEditorValue(size_t controlIndex, const DeviceParam &param)
{
    ensureControlEditor(controlIndex, param);
}

void SignalsVisualizationGui::updateEditableControls(const std::unordered_map<std::string, DeviceParam> &params,
                                                     const std::vector<std::string> &keys,
                                                     bool isValueControl)
{
    ensureConfigControls(keys.size());

    for (size_t i = 0; i < keys.size(); ++i) {
        const auto &key = keys[i];
        const auto editableIt = params.find(key);
        if (editableIt == params.end() || i >= signalListPanel.getConfigControlCount()) {
            continue;
        }

        const DeviceParam &param = editableIt->second;
        signalListPanel.setControlVisual(
            i,
            getSignalAccentColor(i),
            key,
            param.Value,
            buildUnitText(
                isValueControl ? currentDevice->getValueUnits(key) : currentDevice->getConfigUnits(key),
                isValueControl ? "Queued via CONTROL" : "Queued via CONFIG"),
            key,
            isValueControl);

        ensureControlEditor(i, param);
        syncControlEditorValue(i, param);
    }

    clearUnusedConfigControls(keys.size());
}

bool SignalsVisualizationGui::buildNumericHistoryForKey(const std::string &key, double *history, bool appendSample)
{
    if (!currentDevice || !history) {
        return false;
    }

    auto values = currentDevice->getValues();
    auto it = values.find(key);
    if (it == values.end()) {
        return false;
    }

    switch (it->second.DType)
    {
    case DeviceDataType::INT:
        buildDeviceHistory<int>(currentDevice, key, history, appendSample);
        return true;
    case DeviceDataType::FLOAT:
        buildDeviceHistory<float>(currentDevice, key, history, appendSample);
        return true;
    case DeviceDataType::DOUBLE:
        buildDeviceHistory<double>(currentDevice, key, history, appendSample);
        return true;
    default:
        return false;
    }
}

void SignalsVisualizationGui::recordCurrentSamples(const std::vector<std::string> &valueKeys)
{
    if (!recording || !currentDevice)
    {
        return;
    }

    const auto values = currentDevice->getValues();
    unsigned int recordedCount = 0;
    for (const auto &key : valueKeys)
    {
        auto it = values.find(key);
        if (it == values.end())
        {
            debugLogMessage(
                DEBUG_VERBOSE_ERRORS,
                "SignalsVisualizationGui::recordCurrentSamples",
                "recording sample skipped",
                "device=%s key=%s reason=value not found",
                currentDevice->UID.c_str(),
                key.c_str());
            continue;
        }

        if (dataBundleManager.saveNewDataPoint(key, it->second.Value))
        {
            ++recordedCount;
        }
    }

    debugLogMessage(
        DEBUG_VERBOSE_IMPORTANT,
        "SignalsVisualizationGui::recordCurrentSamples",
        "recording sample",
        "device=%s keyCount=%u sampleCount=%u",
        currentDevice->UID.c_str(),
        static_cast<unsigned int>(valueKeys.size()),
        recordedCount);
}

std::vector<std::string> SignalsVisualizationGui::getRecordableValueKeys() const
{
    std::vector<std::string> valueKeys;
    if (!currentDevice) {
        return valueKeys;
    }

    const auto values = currentDevice->getValues();
    for (const auto &key : currentDevice->getValuesKeys()) {
        if (values.find(key) != values.end()) {
            valueKeys.push_back(key);
        }
    }

    return valueKeys;
}

std::vector<std::string> SignalsVisualizationGui::getAvailableChartValueKeys() const
{
    std::vector<std::string> chartKeys;
    if (!currentDevice) {
        return chartKeys;
    }

    if (currentDevice->getRole() == DeviceRole::ACTUATOR) {
        return chartKeys;
    }

    const auto values = currentDevice->getValues();
    for (const auto &key : currentDevice->getValuesKeys()) {
        auto it = values.find(key);
        if (it == values.end()) {
            continue;
        }

        if (it->second.Access == DeviceParamAccess::READ && isNumericType(it->second.DType)) {
            chartKeys.push_back(key);
        }
    }

    return chartKeys;
}

void SignalsVisualizationGui::ensureActiveChartValueKeys()
{
    if (!currentDevice) {
        chartSelectedDeviceId.clear();
        chartSelectedValueKeys.clear();
        return;
    }

    const std::string deviceId = currentDevice->getId();
    const auto availableKeys = getAvailableChartValueKeys();
    if (chartSelectedDeviceId != deviceId) {
        manualScaleConfigured = false;
        if (manualScaleEnabled) {
            manualScaleEnabled = false;
            manualScalePanel.hide();
            refreshScaleModeControls();
        }
        chartSelectedDeviceId = deviceId;
        chartSelectedValueKeys.clear();
    }

    std::vector<std::string> filteredKeys;
    for (const auto &selectedKey : chartSelectedValueKeys) {
        if (std::find(availableKeys.begin(), availableKeys.end(), selectedKey) != availableKeys.end()) {
            filteredKeys.push_back(selectedKey);
            if (filteredKeys.size() == 2) {
                break;
            }
        }
    }

    if (filteredKeys.empty()) {
        for (const auto &availableKey : availableKeys) {
            if (filteredKeys.size() == 2) {
                break;
            }
            filteredKeys.push_back(availableKey);
        }
    }

    chartSelectedValueKeys = filteredKeys;
}

std::vector<std::string> SignalsVisualizationGui::getActiveChartValueKeys()
{
    ensureActiveChartValueKeys();
    return chartSelectedValueKeys;
}

void SignalsVisualizationGui::toggleChartValueSelection(size_t valueIndex)
{
    const auto availableKeys = getAvailableChartValueKeys();
    if (valueIndex >= availableKeys.size()) {
        return;
    }

    ensureActiveChartValueKeys();
    const std::string &key = availableKeys[valueIndex];
    auto selectedIt = std::find(chartSelectedValueKeys.begin(), chartSelectedValueKeys.end(), key);
    if (selectedIt != chartSelectedValueKeys.end()) {
        if (chartSelectedValueKeys.size() > 1) {
            chartSelectedValueKeys.erase(selectedIt);
        }
    } else if (chartSelectedValueKeys.size() < 2) {
        chartSelectedValueKeys.push_back(key);
    } else {
        chartSelectedValueKeys[1] = key;
    }

    chartHistoryOffset = 0;
    chartDragAccumulatorPx = 0;
    chartPinchLastDistancePx = 0;
    chartPinchAccumulatorPx = 0;
    chartPinchActive = false;
    updateChart(true);
    settingsPanel.updateChartValueBlocks(availableKeys, chartSelectedValueKeys);
}

std::string SignalsVisualizationGui::buildChartScalingText(const std::string &chartKey,
                                                           const std::unordered_map<std::string, DeviceParam> &values)
{
    if (chartKey.empty()) {
        return "";
    }

    auto valueIt = values.find(chartKey);
    if (valueIt == values.end()) {
        return "";
    }

    return manualScaleEnabled ? "Manual scale" : "Auto scale";
}

void SignalsVisualizationGui::showEmptyChartState(const char *message)
{
    chartPanel.showEmptyState(message);
}

void SignalsVisualizationGui::hideEmptyChartState()
{
    chartPanel.hideEmptyState();
}

std::pair<double, double> SignalsVisualizationGui::computeChartRange(const double *history)
{
    return computeChartRange(history, HISTORY_CAP);
}

std::pair<double, double> SignalsVisualizationGui::computeChartRange(const double *history, int sampleCount)
{
    if (!history || sampleCount <= 0) {
        return std::pair<double, double>(-1.0, 1.0);
    }

    double minValue = history[0];
    double maxValue = history[0];
    for (int i = 1; i < sampleCount; ++i) {
        if (history[i] < minValue) {
            minValue = history[i];
        }
        if (history[i] > maxValue) {
            maxValue = history[i];
        }
    }

    if (minValue == maxValue) {
        const double absValue = std::fabs(minValue);
        const double delta = absValue == 0.0 ? 1.0 : std::max(absValue * 0.1, 1e-9);
        minValue -= delta;
        maxValue += delta;
    }

    const double span = maxValue - minValue;
    const double magnitude = std::max(std::fabs(minValue), std::fabs(maxValue));
    const double minPad = magnitude == 0.0 ? 1e-9 : std::max(magnitude * 0.01, 1e-9);
    const double pad = std::max(std::fabs(span / 10.0), minPad);
    debugLogMessage("SignalsVisualizationGui::computeChartRange", "math scaling", "min=%.9f max=%.9f pad=%.9f", minValue, maxValue, pad);
    return std::pair<double, double>(minValue - pad, maxValue + pad);
}

void SignalsVisualizationGui::mapHistoryToPlot(const double *rawHistory, lv_coord_t *plotHistory, int sampleCount, double minValue, double maxValue)
{
    if (!rawHistory || !plotHistory || sampleCount <= 0) {
        return;
    }

    const double range = maxValue - minValue;
    for (int i = 0; i < sampleCount; ++i) {
        if (range == 0.0) {
            plotHistory[i] = (CHART_PLOT_MAX - CHART_PLOT_MIN) / 2;
            continue;
        }

        double normalized = manualScaleEnabled ? manualScale.normalizedY(rawHistory[i])
                                                : (rawHistory[i] - minValue) / range;
        if (!std::isfinite(normalized)) {
            plotHistory[i] = LV_CHART_POINT_NONE;
            continue;
        }
        if (normalized < 0.0) {
            normalized = 0.0;
        } else if (normalized > 1.0) {
            normalized = 1.0;
        }

        plotHistory[i] = static_cast<lv_coord_t>(std::lround(CHART_PLOT_MIN + normalized * (CHART_PLOT_MAX - CHART_PLOT_MIN)));
    }
}

int SignalsVisualizationGui::getMaxChartHistoryOffset(const std::vector<std::string> &chartKeys) const
{
    int maxOffset = 0;
    for (const auto &key : chartKeys) {
        auto countIt = historyCountMap.find(makeHistoryBufferKey(currentDevice, key));
        if (countIt == historyCountMap.end()) {
            continue;
        }

        const int offset = countIt->second > chartVisibleSampleCount ? countIt->second - chartVisibleSampleCount : 0;
        if (offset > maxOffset) {
            maxOffset = offset;
        }
    }

    return maxOffset;
}

void SignalsVisualizationGui::panChartHistory(int steps)
{
    if (steps == 0) {
        return;
    }

    const auto chartKeys = getActiveChartValueKeys();
    if (chartKeys.empty()) {
        return;
    }

    const int maxOffset = getMaxChartHistoryOffset(chartKeys);
    int nextOffset = chartHistoryOffset + steps;
    if (nextOffset < 0) {
        nextOffset = 0;
    }
    if (nextOffset > maxOffset) {
        nextOffset = maxOffset;
    }

    if (nextOffset == chartHistoryOffset) {
        return;
    }

    chartHistoryOffset = nextOffset;
    debugLogMessage("SignalsVisualizationGui::panChartHistory", "gui chart pan", "steps=%d nextOffset=%d maxOffset=%d", steps, nextOffset, maxOffset);
    updateChart(true);
}

void SignalsVisualizationGui::adjustChartVisibleSamples(int deltaSamples)
{
    if (deltaSamples == 0 || !chartPanel.isReady()) {
        return;
    }

    int nextSampleCount = chartVisibleSampleCount + deltaSamples;
    if (nextSampleCount < CHART_MIN_VISIBLE_SAMPLES) {
        nextSampleCount = CHART_MIN_VISIBLE_SAMPLES;
    }
    if (nextSampleCount > CHART_MAX_VISIBLE_SAMPLES) {
        nextSampleCount = CHART_MAX_VISIBLE_SAMPLES;
    }
    nextSampleCount = (nextSampleCount / CHART_SAMPLE_STEP) * CHART_SAMPLE_STEP;
    if (nextSampleCount < CHART_MIN_VISIBLE_SAMPLES) {
        nextSampleCount = CHART_MIN_VISIBLE_SAMPLES;
    }

    if (nextSampleCount == chartVisibleSampleCount) {
        return;
    }

    chartVisibleSampleCount = nextSampleCount;
    chartPanel.setVisibleSampleCount(chartVisibleSampleCount);
    updateChartSampleLabel();

    const auto chartKeys = getActiveChartValueKeys();
    const int maxOffset = getMaxChartHistoryOffset(chartKeys);
    if (chartHistoryOffset > maxOffset) {
        chartHistoryOffset = maxOffset;
    }

    chartDragAccumulatorPx = 0;
    debugLogMessage(
        DEBUG_VERBOSE_IMPORTANT,
        "SignalsVisualizationGui::adjustChartVisibleSamples",
        "gui chart samples",
        "visibleSamples=%d maxOffset=%d",
        chartVisibleSampleCount,
        maxOffset);
    updateChart(true);
}

void SignalsVisualizationGui::updateChartSampleLabel()
{
    char label[24];
    std::snprintf(label, sizeof(label), "Samples %d", chartVisibleSampleCount);
    chartPanel.setSamplesText(label);
}

bool SignalsVisualizationGui::handleChartPinchGesture()
{
    if (&touch_point_count == nullptr || &touch_last_x == nullptr || &touch_last_y == nullptr ||
        &touch_second_x == nullptr || &touch_second_y == nullptr || touch_point_count < 2) {
        chartPinchActive = false;
        chartPinchLastDistancePx = 0;
        chartPinchAccumulatorPx = 0;
        return false;
    }

    const int dx = touch_last_x - touch_second_x;
    const int dy = touch_last_y - touch_second_y;
    const int distancePx = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
    if (!chartPinchActive) {
        chartPinchActive = true;
        chartPinchLastDistancePx = distancePx;
        chartPinchAccumulatorPx = 0;
        chartDragAccumulatorPx = 0;
        return true;
    }

    chartPinchAccumulatorPx += chartPinchLastDistancePx - distancePx;
    chartPinchLastDistancePx = distancePx;

    const int pixelsPerStep = 18;
    int sampleSteps = 0;
    while (chartPinchAccumulatorPx >= pixelsPerStep) {
        ++sampleSteps;
        chartPinchAccumulatorPx -= pixelsPerStep;
    }
    while (chartPinchAccumulatorPx <= -pixelsPerStep) {
        --sampleSteps;
        chartPinchAccumulatorPx += pixelsPerStep;
    }

    if (sampleSteps != 0) {
        adjustChartVisibleSamples(sampleSteps * CHART_SAMPLE_STEP);
    }

    return true;
}

void SignalsVisualizationGui::handleChartDrag(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        chartDragAccumulatorPx = 0;
        chartPinchLastDistancePx = 0;
        chartPinchAccumulatorPx = 0;
        chartPinchActive = false;
        return;
    }

    if (code != LV_EVENT_PRESSING)
    {
        return;
    }

    if (handleChartPinchGesture()) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev)
    {
        return;
    }

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    chartDragAccumulatorPx += vect.x;

    const int pixelsPerStep = 24;
    int steps = 0;
    while (chartDragAccumulatorPx >= pixelsPerStep)
    {
        ++steps;
        chartDragAccumulatorPx -= pixelsPerStep;
    }
    while (chartDragAccumulatorPx <= -pixelsPerStep)
    {
        --steps;
        chartDragAccumulatorPx += pixelsPerStep;
    }

    panChartHistory(steps);
}

bool SignalsVisualizationGui::beginDeviceNavigation(bool requireIdleRecording, bool &wasRunning)
{
    if (requireIdleRecording && recording) {
        return false;
    }

    wasRunning = deviceManager.isRunning();
    deviceManager.setRunning(false);
    return true;
}

void SignalsVisualizationGui::finishDeviceNavigation(bool wasRunning, BaseDevice *nextDevice)
{
    currentDevice = nextDevice;
    chartHistoryOffset = 0;
    chartDragAccumulatorPx = 0;
    chartPinchLastDistancePx = 0;
    chartPinchAccumulatorPx = 0;
    chartPinchActive = false;
    if (currentDevice) {
        currentDevice->setRedrawPending(true);
    }

    delay_ms(10);
    deviceManager.setRunning(wasRunning);
    drawCurrentDevice(true);
}

void SignalsVisualizationGui::updateDeviceDataDisplay()
{
    if (!currentDevice || !signalListPanel.getContainer())
        return;

    const auto values = currentDevice->getValues();
    const auto valueKeys = currentDevice->getValuesKeys();
    const auto configs = currentDevice->getConfigs();
    const auto configKeys = currentDevice->getConfigsKeys();
    std::vector<std::string> readableValueKeys;
    std::vector<std::string> writableValueKeys;
    for (const auto &key : valueKeys) {
        auto it = values.find(key);
        if (it == values.end()) {
            continue;
        }
        if (it->second.Access == DeviceParamAccess::WRITE && currentDevice->getRole() != DeviceRole::SENSOR) {
            writableValueKeys.push_back(key);
        } else {
            readableValueKeys.push_back(key);
        }
    }

    if (showingConfigPanel && configKeys.empty() && !valueKeys.empty()) {
        showingConfigPanel = false;
    } else if (!showingConfigPanel && valueKeys.empty() && !configKeys.empty()) {
        showingConfigPanel = true;
    }

    updateDeviceTitle();
    updateListModeToggleState();
    if (showingConfigPanel) {
        clearUnusedSignalCards(0);
        updateEditableControls(configs, configKeys, false);
    } else {
        updateSignalCards(values, readableValueKeys);
        updateEditableControls(values, writableValueKeys, true);
    }
    updateActionButtonsState();
}

void SignalsVisualizationGui::updateChart(bool force)
{
    if (!currentDevice || !chartPanel.isReady())
        return;

    if (!force && !currentDevice->getRedrawPending())
        return;

    const bool appendSample = !force && currentDevice->getRedrawPending();
    if (appendSample)
    {
        recordCurrentSamples(getRecordableValueKeys());
    }

    const auto chartKeys = getActiveChartValueKeys();
    if (chartKeys.empty())
    {
        chartPanel.setScalingText("");
        chartPanel.setSamplesText("");
        showEmptyChartState(currentDevice->getRole() == DeviceRole::ACTUATOR
                                ? "Control-only device"
                                : "No numeric signal available");
        return;
    }

    try
    {
        hideEmptyChartState();
        const auto values = currentDevice->getValues();
        const std::string primaryScalingText = buildChartScalingText(chartKeys[0], values);
        const std::string secondaryScalingText = chartKeys.size() > 1
                                                     ? buildChartScalingText(chartKeys[1], values)
                                                     : "";
        chartPanel.setScalingText(primaryScalingText.c_str(), secondaryScalingText.c_str());
        updateChartSampleLabel();

        double rawHistoryPrimary[CHART_MAX_VISIBLE_SAMPLES];
        lv_coord_t plotHistoryPrimary[CHART_MAX_VISIBLE_SAMPLES];
        if (!buildNumericHistoryForKey(chartKeys[0], rawHistoryPrimary, appendSample)) {
            return;
        }

        auto primaryRange = manualScaleEnabled ? std::make_pair(manualScale.yMin, manualScale.yMax)
                                               : computeChartRange(rawHistoryPrimary, chartVisibleSampleCount);
        mapHistoryToPlot(rawHistoryPrimary, plotHistoryPrimary, chartVisibleSampleCount, primaryRange.first, primaryRange.second);

        bool haveSecond = false;
        double rawHistorySecondary[CHART_MAX_VISIBLE_SAMPLES];
        lv_coord_t plotHistorySecondary[CHART_MAX_VISIBLE_SAMPLES];
        std::pair<double, double> secondaryRange = primaryRange;
        if (chartKeys.size() > 1) {
            haveSecond = buildNumericHistoryForKey(chartKeys[1], rawHistorySecondary, appendSample);
            if (haveSecond) {
                secondaryRange = manualScaleEnabled ? primaryRange
                                                    : computeChartRange(rawHistorySecondary, chartVisibleSampleCount);
                mapHistoryToPlot(rawHistorySecondary, plotHistorySecondary, chartVisibleSampleCount, secondaryRange.first, secondaryRange.second);
            }
        }

        if (!manualScaleEnabled) {
            autoYMin = haveSecond ? std::min(primaryRange.first, secondaryRange.first) : primaryRange.first;
            autoYMax = haveSecond ? std::max(primaryRange.second, secondaryRange.second) : primaryRange.second;
        }
        chartPanel.setRange(primaryRange.first, primaryRange.second, secondaryRange.first, secondaryRange.second, haveSecond);
        chartPanel.clearSeries();
        chartPanel.populatePrimarySeries(plotHistoryPrimary, chartVisibleSampleCount);

        if (haveSecond) {
            chartPanel.populateSecondarySeries(plotHistorySecondary, chartVisibleSampleCount);
        } else {
            chartPanel.hideSecondarySeries();
        }

        chartPanel.refresh();
    }
    catch (const std::exception &e)
    {
        Exception("SignalsVisualizationGui::updateChart", e.what()).print();
    }
}

void SignalsVisualizationGui::updateActionButtonsState()
{
    toolbarPanel.setRecordingState(currentDeviceSupportsRecording(), recording);
}

bool SignalsVisualizationGui::applyEditableValue(bool isValueControl, const std::string &key, const std::string &value)
{
    if (!currentDevice || key.empty()) {
        return false;
    }

    try {
        if (isValueControl) {
            currentDevice->setValue(key, value);
        } else {
            currentDevice->setConfig(key, value);
        }

        debugLogMessage(
            DEBUG_VERBOSE_IMPORTANT,
            "SignalsVisualizationGui::applyEditableValue",
            isValueControl ? "runtime control" : "runtime config",
            "device=%s key=%s value=%s",
            currentDevice->UID.c_str(),
            key.c_str(),
            value.c_str());

        if (!deviceManager.ensureProtocolInitialized()) {
            debugLogMessage(
                DEBUG_VERBOSE_ERRORS,
                "SignalsVisualizationGui::applyEditableValue",
                "protocol init failed",
                "device=%s key=%s",
                currentDevice->UID.c_str(),
                key.c_str());
            showAlert("Protocol init failed");
            return false;
        }

        if (!deviceManager.sync(currentDevice)) {
            const std::string syncError = currentDevice->getError();
            debugLogMessage(
                DEBUG_VERBOSE_ERRORS,
                "SignalsVisualizationGui::applyEditableValue",
                isValueControl ? "runtime control failed" : "runtime config failed",
                "device=%s key=%s error=%s",
                currentDevice->UID.c_str(),
                key.c_str(),
                syncError.c_str());
            showAlert(isValueControl
                          ? (syncError.empty() ? "Control failed" : syncError.c_str())
                          : (syncError.empty() ? "Config failed" : syncError.c_str()));
            return false;
        }

        return true;
    } catch (const std::exception &e) {
        Exception("SignalsVisualizationGui::applyEditableValue", e.what()).print();
        showAlert(isValueControl ? "Failed to send control value" : "Failed to send config value");
        return false;
    } catch (...) {
        Exception("SignalsVisualizationGui::applyEditableValue", "Unknown exception").print();
        showAlert(isValueControl ? "Failed to send control value" : "Failed to send config value");
        return false;
    }
}

void SignalsVisualizationGui::handleDropdownConfigChanged(size_t controlIndex)
{
    auto *control = signalListPanel.getConfigControl(controlIndex);
    if (!control || !control->editor || control->key.empty()) {
        return;
    }

    char selected[64] = {0};
    lv_dropdown_get_selected_str(control->editor, selected, sizeof(selected));
    if (applyEditableValue(control->isValueControl, control->key, selected) && control->valueLabel) {
        lv_label_set_text(control->valueLabel, selected);
    }
}

void SignalsVisualizationGui::handleSliderConfigChanged(size_t controlIndex)
{
    auto *control = signalListPanel.getConfigControl(controlIndex);
    if (!control || !control->editor || control->key.empty()) {
        return;
    }

    const std::string value = std::to_string(lv_slider_get_value(control->editor));
    if (applyEditableValue(control->isValueControl, control->key, value) && control->valueLabel) {
        lv_label_set_text(control->valueLabel, value.c_str());
    }
}

void SignalsVisualizationGui::handleTextConfigSubmitted(size_t controlIndex)
{
    auto *control = signalListPanel.getConfigControl(controlIndex);
    if (!control || !control->editor || control->key.empty()) {
        return;
    }

    const char *value = lv_textarea_get_text(control->editor);
    if (applyEditableValue(control->isValueControl, control->key, value) && control->valueLabel) {
        lv_label_set_text(control->valueLabel, value);
    }
}

void SignalsVisualizationGui::handleBackButtonClick(){
    if(recording){
        handleStillRecording();
        return;
    }
    router.showSelection();
}

void SignalsVisualizationGui::handlePauseButtonClick()
{
    if (settingsPanel.isVisible() || deviceManager.hasLostConnection()) return;
    paused = !paused;
    deviceManager.setRunning(!paused);
    toolbarPanel.setPaused(paused);
}

void SignalsVisualizationGui::handleSyncButtonClick()
{
    if (!currentDevice || !paused)
        return;

    syncCurrentDevice();
}

void SignalsVisualizationGui::handleRecordButtonClick(const char *message)
{
    if (!currentDevice)
        return;

    if (!currentDeviceSupportsRecording())
    {
        showAlert("Recording is available only for devices with values");
        return;
    }

    //logMessage("Record button clicked. Current recording state: %s\n", recording ? "ON" : "OFF");

    if (recording)
    {
        debugLogMessage("SignalsVisualizationGui::handleRecordButtonClick", "recording stop", "device=%s", currentDevice->UID.c_str());
        if (!dataBundleManager.hasRecordingData())
        {
            recordCurrentSamples(getRecordableValueKeys());
        }
        const bool saved = dataBundleManager.saveRecording();
        recording = false;
        showAlert(saved ? (message ? message : "Record was saved") : "Recording has no data to save");
    }
    else
    {
        const auto recordableKeys = getRecordableValueKeys();
        debugLogMessage(
            DEBUG_VERBOSE_IMPORTANT,
            "SignalsVisualizationGui::handleRecordButtonClick",
            "recording start",
            "device=%s type=%s recordableKeys=%u",
            currentDevice->UID.c_str(),
            currentDevice->Type.c_str(),
            static_cast<unsigned int>(recordableKeys.size()));
        if (!dataBundleManager.startRecording(currentDevice->Type, currentDevice->UID))
        {
            showAlert("Failed to start recording");
            return;
        }
        recording = true;
        recordCurrentSamples(recordableKeys);
    }

    updateActionButtonsState();
}

void SignalsVisualizationGui::handleClearButtonClick()
{
    if (!recording && !currentDeviceSupportsRecording())
    {
        showAlert("Nothing to clear on control-only devices");
        return;
    }

    static const char *btns[] = {"Yes", ""};
    const char *message = recording
                              ? "Are you sure you want to scrape this recording?"
        : "Are you sure you want to clear the device history?";

    feedbackPanel.showConfirmationDialog(
        "Confirm Clear",
        message,
        btns,
        this,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const lv_event_code_t code = lv_event_get_code(e);

            if (self->feedbackPanel.isConfirmationAccepted(e, "Yes")) {
                if (self->recording) {
                    self->dataBundleManager.discardRecording();
                    self->recording = false;
                    self->showAlert("Recording discarded as requested");
                    self->updateActionButtonsState();
                } else {
                    self->handleClearConfirmButtonClick();
                }
            } else if (code != LV_EVENT_DELETE) {
                return;
            }

            self->feedbackPanel.closeConfirmationDialog(e);
        });
}

void SignalsVisualizationGui::handleClearConfirmButtonClick()
{
    if (currentDevice)
    {
        debugLogMessage("SignalsVisualizationGui::handleClearConfirmButtonClick", "runtime memory write", "device=%s", currentDevice->UID.c_str());
        chartHistoryOffset = 0;
        chartDragAccumulatorPx = 0;

        // Clear device internal history
        currentDevice->clearHistory();

        // Clear per-key buffers and set them to zero
        for (auto &v : currentDevice->getValuesKeys())
        {
            clearDeviceHistoryBuffer(v);
        }

        chartPanel.resetToZero();
        signalListPanel.setAllSignalCardValues("0");
    }
}

void SignalsVisualizationGui::handleSettingsButtonClick(lv_obj_t *recordGroup, lv_obj_t *btnSettings,lv_obj_t *parentWidget)
{
    if (settingsPanel.isVisible()) {
        hideSettingsPanel();
        return;
    }
    if (!parentWidget || !recordGroup || !btnSettings) return;
    paused = true;
    deviceManager.setRunning(false);
    toolbarPanel.setPaused(true);
    const auto availableChartValueKeys = getAvailableChartValueKeys();
    const auto activeChartValueKeys = getActiveChartValueKeys();
    settingsPanel.show(
        parentWidget,
        recordGroup,
        btnSettings,
        this,
        dataBundleManager.getDataBundleAmount(),
        dataBundleManager.isBundleStorageFull(),
        router.getVisualizationUpdatePeriodMs(),
        availableChartValueKeys,
        activeChartValueKeys,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->hideSettingsPanel();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->handleDataBundleShowButtonClick();
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
            self->handleChartValueSelectionClick(static_cast<size_t>(index));
        },
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->router.setVisualizationUpdatePeriodMs(lv_slider_get_value(lv_event_get_target(e)));
        });
    refreshScaleModeControls();
    refreshSeriesColors();
}

void SignalsVisualizationGui::handleChartValueSelectionClick(size_t valueIndex)
{
    toggleChartValueSelection(valueIndex);
}

void SignalsVisualizationGui::handleDataBundleShowButtonClick(){
    if(recording){
        handleStillRecording();
        return;
    }
    hideSettingsPanel();
    
    router.openDatabankFromVisualization();
}

void SignalsVisualizationGui::handleStillRecording(){
    if(!recording) return;

    static const char *btns[] = {"Save", "Discard", ""};
    feedbackPanel.showConfirmationDialog(
        "Confirm Clear",
        "You are currently recording. Do you want to stop recording?",
        btns,
        this,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            const lv_event_code_t code = lv_event_get_code(e);

            if (self->feedbackPanel.isConfirmationAccepted(e, "Save")) {
                self->handleRecordButtonClick(nullptr);
            } else if (self->feedbackPanel.isConfirmationAccepted(e, "Discard")) {
                self->dataBundleManager.discardRecording();
                self->recording = false;
                self->showAlert("Recording discarded as requested");
                self->updateActionButtonsState();
            } else if (code != LV_EVENT_DELETE) {
                return;
            }

            self->feedbackPanel.closeConfirmationDialog(e);
        });
}

void SignalsVisualizationGui::hideSettingsPanel(bool resume)
{
    const bool wasVisible = settingsPanel.isVisible();
    manualScalePanel.hide();
    settingsPanel.hide();
    if (wasVisible) {
        paused = deviceManager.hasLostConnection();
        deviceManager.setRunning(resume && !paused);
        toolbarPanel.setPaused(paused);
    }
}

void SignalsVisualizationGui::goToPreviousDevice()
{
    bool wasRunning = false;
    if (!beginDeviceNavigation(true, wasRunning))
        return;

    finishDeviceNavigation(wasRunning, visualizationSession.previousDevice());
}

void SignalsVisualizationGui::goToNextDevice()
{
    bool wasRunning = false;
    if (!beginDeviceNavigation(true, wasRunning))
        return;

    finishDeviceNavigation(wasRunning, visualizationSession.nextDevice());
}

void SignalsVisualizationGui::goToFirstDevice()
{
    bool wasRunning = false;
    if (!beginDeviceNavigation(false, wasRunning))
        return;

    visualizationSession.resetCurrentIndex();
    finishDeviceNavigation(wasRunning, visualizationSession.getCurrentDevice());
}

bool SignalsVisualizationGui::syncCurrentDevice()
{
    if (!currentDevice)
    {
        return false;
    }

    if (!deviceManager.ensureProtocolInitialized()) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "SignalsVisualizationGui::syncCurrentDevice", "protocol init failed", "device=%s", currentDevice->UID.c_str());
        showAlert("Protocol init failed");
        return false;
    }

    currentDevice->requestRuntimeUpdate();
    const bool success = deviceManager.sync(currentDevice);
    if (!success) {
        debugLogMessage(DEBUG_VERBOSE_ERRORS, "SignalsVisualizationGui::syncCurrentDevice", "runtime sync failed", "device=%s error=%s", currentDevice->UID.c_str(), currentDevice->getError().c_str());
        showAlert(currentDevice->getError().empty() ? "Sync failed" : currentDevice->getError().c_str());
        return false;
    }

    chartHistoryOffset = 0;
    chartDragAccumulatorPx = 0;
    currentDevice->setRedrawPending(true);
    updateDeviceDataDisplay();
    updateChart();
    currentDevice->setRedrawPending(false);
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "SignalsVisualizationGui::syncCurrentDevice", "runtime sync", "device=%s manual update completed", currentDevice->UID.c_str());
    return true;
}

void SignalsVisualizationGui::showVisualization()
{
    if (!initialized || !ui_DeviceWidget)
        return;

    lv_obj_clear_flag(ui_DeviceWidget, LV_OBJ_FLAG_HIDDEN);
    toolbarPanel.setPaused(paused);

    // Refresh the display with current device data
    goToFirstDevice();
    deviceManager.setRunning(!paused);
    debugLogMessage("SignalsVisualizationGui::showVisualization", "runtime pause state", "paused=%d running=%d", paused, deviceManager.isRunning());
    debugLogMessage("SignalsVisualizationGui::showVisualization", "gui operation", "shown");
}

bool SignalsVisualizationGui::reconnectAfterDisconnect()
{
    if (!deviceManager.reconnectDevice(currentDevice)) return false;
    // Reconnect restores pins, never automatically replay cached actuator control.
    paused = true;
    toolbarPanel.setPaused(paused);
    deviceManager.setRunning(!paused);
    updateChart(true);
    return true;
}

void SignalsVisualizationGui::hideVisualization()
{
    if (!initialized || !ui_DeviceWidget)
        return;

    hideSettingsPanel(false);
    lv_obj_add_flag(ui_DeviceWidget, LV_OBJ_FLAG_HIDDEN);
    debugLogMessage("SignalsVisualizationGui::hideVisualization", "gui operation", "hidden");
}

void SignalsVisualizationGui::showAlert(const char *message){
    if(message == nullptr || !initialized || !ui_DeviceWidget)
        return;

    feedbackPanel.showAlert(
        ui_DeviceWidget,
        this,
        message,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsVisualizationGui *>(lv_event_get_user_data(e));
            self->hideAlert();
        });
}

void SignalsVisualizationGui::hideAlert(){
    feedbackPanel.hideAlert();
}

void SignalsVisualizationGui::handleScaleModeEvent(lv_event_t *event)
{
    auto *self = static_cast<SignalsVisualizationGui*>(lv_event_get_user_data(event));
    self->setManualScaleEnabled(lv_dropdown_get_selected(lv_event_get_target(event)) == 1);
}

void SignalsVisualizationGui::handleManualScaleEvent(lv_event_t *event)
{
    static_cast<SignalsVisualizationGui*>(lv_event_get_user_data(event))->showManualScalePanel();
}

void SignalsVisualizationGui::refreshScaleModeControls()
{
    settingsPanel.configureScaleMode(manualScaleEnabled, this, handleScaleModeEvent, handleManualScaleEvent);
}

void SignalsVisualizationGui::setManualScaleEnabled(bool enabled)
{
    if (enabled == manualScaleEnabled) return;
    if (enabled) {
        if (!manualScaleConfigured) {
            manualScale.yMin = autoYMin;
            manualScale.yMax = autoYMax;
        }
        if (!manualScale.valid()) return;
        manualScaleConfigured = true;
    } else {
        manualScalePanel.hide();
    }
    manualScaleEnabled = enabled;
    refreshScaleModeControls();
    updateChart(true);
}

void SignalsVisualizationGui::showManualScalePanel()
{
    if (!manualScaleEnabled) return;
    manualScalePanel.show(manualScale, [this](const ChartManualScale& scale) {
        manualScale = scale;
        updateChart(true);
    });
}

void SignalsVisualizationGui::handleSeriesColorEvent(lv_event_t *event)
{
    auto *self = static_cast<SignalsVisualizationGui*>(lv_event_get_user_data(event));
    const bool primary = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(event))) == 0;
    auto& index = primary ? self->primaryColorIndex : self->secondaryColorIndex;
    index = static_cast<uint8_t>((index + 1) % (sizeof(SIGNAL_CARD_COLORS) / sizeof(SIGNAL_CARD_COLORS[0])));
    self->refreshSeriesColors();
}

void SignalsVisualizationGui::refreshSeriesColors()
{
    const auto primary = SIGNAL_CARD_COLORS[primaryColorIndex];
    const auto secondary = SIGNAL_CARD_COLORS[secondaryColorIndex];
    chartPanel.setSeriesColors(primary, secondary);
    settingsPanel.configureSeriesColors(primary, secondary, this, handleSeriesColorEvent);
    settingsPanel.updateChartValueBlocks(getAvailableChartValueKeys(), getActiveChartValueKeys());
}

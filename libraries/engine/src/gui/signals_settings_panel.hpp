#ifndef SIGNALS_SETTINGS_PANEL_HPP
#define SIGNALS_SETTINGS_PANEL_HPP

#include "lvgl.h"
#include <string>
#include <vector>

class SignalsSettingsPanel
{
private:
    uint32_t primaryColor = 0x009BFF;
    uint32_t secondaryColor = 0xFF6B35;
    lv_obj_t *ui_SettingsOverlay = nullptr;
    lv_obj_t *ui_SettingsBridgeGroup = nullptr;
    lv_obj_t *ui_SettingsBridge = nullptr;
    lv_obj_t *ui_SettingsBridgeFill = nullptr;
    lv_obj_t *ui_SettingsGroup = nullptr;
    lv_obj_t *ui_SettingsOutlay = nullptr;
    lv_obj_t *ui_SettingsHeaderLine = nullptr;
    lv_obj_t *ui_SettingsHeaderLabel = nullptr;
    lv_obj_t *ui_SettingsScaleModeLabel = nullptr;
    lv_obj_t *ui_SettingsScaleModeValueLabel = nullptr;
    lv_obj_t *ui_SettingsSeriesColorLabel = nullptr;
    lv_obj_t *ui_SettingsPrimarySwatch = nullptr;
    lv_obj_t *ui_SettingsSecondarySwatch = nullptr;
    lv_obj_t *ui_SettingsGraphValuesLabel = nullptr;
    lv_obj_t *ui_SettingsManualScaleButton = nullptr;
    lv_obj_t *ui_SettingsManualScaleButtonLabel = nullptr;
    lv_obj_t *ui_SettingsUpdatePeriodLabel = nullptr;
    lv_obj_t *ui_SettingsUpdatePeriodSlider = nullptr;
    lv_obj_t *ui_SettingsDataBundleLabel = nullptr;
    lv_obj_t *ui_SettingsDataBundleCountLabel = nullptr;
    lv_obj_t *ui_SettingsDataBundleShowButton = nullptr;
    lv_obj_t *ui_SettingsDataBundleShowButtonLabel = nullptr;
    std::vector<lv_obj_t *> ui_SettingsValueBlocks;
    std::vector<lv_obj_t *> ui_SettingsValueBlockLabels;

public:
    bool isVisible() const { return ui_SettingsOverlay != nullptr; }

    void show(lv_obj_t *parentWidget,
              lv_obj_t *recordGroup,
              lv_obj_t *btnSettings,
              void *userData,
              uint32_t bundleAmount,
              bool isBundleFull,
              uint32_t updatePeriodMs,
              const std::vector<std::string> &chartValueKeys,
              const std::vector<std::string> &selectedChartValueKeys,
              lv_event_cb_t closeCallback,
              lv_event_cb_t showBundlesCallback,
              lv_event_cb_t chartValueCallback,
              lv_event_cb_t updatePeriodCallback);

    void updateChartValueBlocks(const std::vector<std::string> &chartValueKeys,
                                const std::vector<std::string> &selectedChartValueKeys);

    void configureSeriesColors(uint32_t primary, uint32_t secondary, void *userData, lv_event_cb_t callback);
    void hide();
    void configureScaleMode(bool manual, void *userData,
                            lv_event_cb_t modeCallback, lv_event_cb_t manualCallback);
};

#endif

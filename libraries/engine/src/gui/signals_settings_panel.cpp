#include "signals_settings_panel.hpp"
#include "visualization_poll_schedule.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace
{
int selectedSlotForKey(const std::vector<std::string> &selectedKeys, const std::string &key)
{
    auto it = std::find(selectedKeys.begin(), selectedKeys.end(), key);
    return it == selectedKeys.end() ? -1 : static_cast<int>(it - selectedKeys.begin());
}
}

void SignalsSettingsPanel::show(lv_obj_t *parentWidget,
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
                                lv_event_cb_t updatePeriodCallback)
{
    if (!parentWidget || isVisible()) {
        return;
    }

    ui_SettingsOverlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(ui_SettingsOverlay, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(ui_SettingsOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui_SettingsOverlay, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(ui_SettingsOverlay, 0, 0);
    lv_obj_add_flag(ui_SettingsOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_SettingsOverlay, closeCallback, LV_EVENT_CLICKED, userData);

    ui_SettingsBridgeGroup = lv_obj_create(parentWidget);
    lv_obj_remove_style_all(ui_SettingsBridgeGroup);
    lv_obj_set_width(ui_SettingsBridgeGroup, 250);
    lv_obj_set_height(ui_SettingsBridgeGroup, 379);
    lv_obj_set_x(ui_SettingsBridgeGroup, -7);
    lv_obj_set_y(ui_SettingsBridgeGroup, 25);
    lv_obj_set_align(ui_SettingsBridgeGroup, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_SettingsBridgeGroup, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ui_SettingsBridge = lv_obj_create(ui_SettingsBridgeGroup);
    lv_obj_remove_style_all(ui_SettingsBridge);
    lv_obj_set_width(ui_SettingsBridge, 50);
    lv_obj_set_height(ui_SettingsBridge, 50);
    lv_obj_set_align(ui_SettingsBridge, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_SettingsBridge, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_SettingsBridge, lv_color_hex(0x009BFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsBridge, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsBridgeFill = lv_obj_create(ui_SettingsBridgeGroup);
    lv_obj_remove_style_all(ui_SettingsBridgeFill);
    lv_obj_set_width(ui_SettingsBridgeFill, 50);
    lv_obj_set_height(ui_SettingsBridgeFill, 50);
    lv_obj_set_x(ui_SettingsBridgeFill, -37);
    lv_obj_set_y(ui_SettingsBridgeFill, -25);
    lv_obj_set_align(ui_SettingsBridgeFill, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_SettingsBridgeFill, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_SettingsBridgeFill, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SettingsBridgeFill, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsBridgeFill, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_move_foreground(recordGroup);
    lv_obj_move_foreground(btnSettings);

    ui_SettingsGroup = lv_obj_create(ui_SettingsOverlay);
    lv_obj_remove_style_all(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsGroup, 250);
    lv_obj_set_height(ui_SettingsGroup, 379);
    lv_obj_set_x(ui_SettingsGroup, -7);
    lv_obj_set_y(ui_SettingsGroup, 25);
    lv_obj_set_align(ui_SettingsGroup, LV_ALIGN_TOP_RIGHT);
    lv_obj_add_flag(ui_SettingsGroup, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_SettingsGroup, LV_OBJ_FLAG_SCROLLABLE);

    ui_SettingsOutlay = lv_obj_create(ui_SettingsGroup);
    lv_obj_remove_style_all(ui_SettingsOutlay);
    lv_obj_set_width(ui_SettingsOutlay, 250);
    lv_obj_set_height(ui_SettingsOutlay, 354);
    lv_obj_set_align(ui_SettingsOutlay, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ui_SettingsOutlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_SettingsOutlay, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SettingsOutlay, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsOutlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_SettingsOutlay, lv_color_hex(0x009BFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_SettingsOutlay, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_SettingsOutlay, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsHeaderLine = lv_obj_create(ui_SettingsGroup);
    lv_obj_remove_style_all(ui_SettingsHeaderLine);
    lv_obj_set_width(ui_SettingsHeaderLine, 230);
    lv_obj_set_height(ui_SettingsHeaderLine, 1);
    lv_obj_set_x(ui_SettingsHeaderLine, 0);
    lv_obj_set_y(ui_SettingsHeaderLine, 58);
    lv_obj_set_align(ui_SettingsHeaderLine, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_SettingsHeaderLine, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_SettingsHeaderLine, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsHeaderLine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsHeaderLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsHeaderLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsHeaderLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsHeaderLabel, 10);
    lv_obj_set_y(ui_SettingsHeaderLabel, 35);
    lv_label_set_text(ui_SettingsHeaderLabel, "Settings");
    lv_obj_set_style_text_color(ui_SettingsHeaderLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SettingsHeaderLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsScaleModeLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsScaleModeLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsScaleModeLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsScaleModeLabel, 10);
    lv_obj_set_y(ui_SettingsScaleModeLabel, 72);
    lv_label_set_text(ui_SettingsScaleModeLabel, "Scale mode:");
    lv_obj_set_style_text_color(ui_SettingsScaleModeLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsScaleModeValueLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsScaleModeValueLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsScaleModeValueLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsScaleModeValueLabel, -20);
    lv_obj_set_y(ui_SettingsScaleModeValueLabel, 72);
    lv_obj_set_align(ui_SettingsScaleModeValueLabel, LV_ALIGN_TOP_RIGHT);
    lv_label_set_text(ui_SettingsScaleModeValueLabel, "Auto");
    lv_obj_set_style_text_color(ui_SettingsScaleModeValueLabel, lv_color_hex(0x009BFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsSeriesColorLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsSeriesColorLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsSeriesColorLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsSeriesColorLabel, 10);
    lv_obj_set_y(ui_SettingsSeriesColorLabel, 96);
    lv_label_set_text(ui_SettingsSeriesColorLabel, "Line colors:");
    lv_obj_set_style_text_color(ui_SettingsSeriesColorLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsPrimarySwatch = lv_obj_create(ui_SettingsGroup);
    lv_obj_remove_style_all(ui_SettingsPrimarySwatch);
    lv_obj_set_size(ui_SettingsPrimarySwatch, 28, 16);
    lv_obj_set_x(ui_SettingsPrimarySwatch, -58);
    lv_obj_set_y(ui_SettingsPrimarySwatch, 96);
    lv_obj_set_align(ui_SettingsPrimarySwatch, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_SettingsPrimarySwatch, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_SettingsPrimarySwatch, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SettingsPrimarySwatch, lv_color_hex(0x009BFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsPrimarySwatch, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsSecondarySwatch = lv_obj_create(ui_SettingsGroup);
    lv_obj_remove_style_all(ui_SettingsSecondarySwatch);
    lv_obj_set_size(ui_SettingsSecondarySwatch, 28, 16);
    lv_obj_set_x(ui_SettingsSecondarySwatch, -20);
    lv_obj_set_y(ui_SettingsSecondarySwatch, 96);
    lv_obj_set_align(ui_SettingsSecondarySwatch, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(ui_SettingsSecondarySwatch, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_SettingsSecondarySwatch, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SettingsSecondarySwatch, lv_color_hex(0xFF6B35), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsSecondarySwatch, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsGraphValuesLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsGraphValuesLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsGraphValuesLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsGraphValuesLabel, 10);
    lv_obj_set_y(ui_SettingsGraphValuesLabel, 124);
    lv_label_set_text(ui_SettingsGraphValuesLabel, "Graph values:");
    lv_obj_set_style_text_color(ui_SettingsGraphValuesLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsValueBlocks.clear();
    ui_SettingsValueBlockLabels.clear();
    const size_t blockCount = chartValueKeys.size() > 4 ? 4 : chartValueKeys.size();
    for (size_t i = 0; i < blockCount; ++i) {
        lv_obj_t *block = lv_btn_create(ui_SettingsGroup);
        lv_obj_set_size(block, 96, 24);
        lv_obj_set_x(block, 17 + static_cast<int>(i % 2) * 106);
        lv_obj_set_y(block, 148 + static_cast<int>(i / 2) * 30);
        lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(block, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_user_data(block, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(block, chartValueCallback, LV_EVENT_CLICKED, userData);

        lv_obj_t *label = lv_label_create(block);
        lv_obj_set_width(label, 86);
        lv_obj_set_height(label, LV_SIZE_CONTENT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_label_set_text(label, chartValueKeys[i].c_str());
        lv_obj_set_align(label, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_SettingsValueBlocks.push_back(block);
        ui_SettingsValueBlockLabels.push_back(label);
    }

    updateChartValueBlocks(chartValueKeys, selectedChartValueKeys);

    ui_SettingsManualScaleButton = lv_btn_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsManualScaleButton, 200);
    lv_obj_set_height(ui_SettingsManualScaleButton, 20);
    lv_obj_set_x(ui_SettingsManualScaleButton, 17);
    lv_obj_set_y(ui_SettingsManualScaleButton, 214);
    lv_obj_add_flag(ui_SettingsManualScaleButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsManualScaleButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_state(ui_SettingsManualScaleButton, LV_STATE_DISABLED);
    lv_obj_set_style_radius(ui_SettingsManualScaleButton, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_SettingsManualScaleButton, lv_color_hex(0xA8B3BF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SettingsManualScaleButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsManualScaleButtonLabel = lv_label_create(ui_SettingsManualScaleButton);
    lv_obj_set_width(ui_SettingsManualScaleButtonLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsManualScaleButtonLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_SettingsManualScaleButtonLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SettingsManualScaleButtonLabel, "Manual Scale");

    ui_SettingsUpdatePeriodLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_pos(ui_SettingsUpdatePeriodLabel, 10, 242);
    lv_obj_set_style_text_color(ui_SettingsUpdatePeriodLabel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(ui_SettingsUpdatePeriodLabel, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(ui_SettingsUpdatePeriodLabel, "Update period: %u ms", static_cast<unsigned>(updatePeriodMs));

    ui_SettingsUpdatePeriodSlider = lv_slider_create(ui_SettingsGroup);
    lv_obj_set_size(ui_SettingsUpdatePeriodSlider, 200, 12);
    lv_obj_set_pos(ui_SettingsUpdatePeriodSlider, 25, 272);
    lv_slider_set_range(ui_SettingsUpdatePeriodSlider,
                        VisualizationPollSchedule::MIN_PERIOD_MS,
                        VisualizationPollSchedule::MAX_PERIOD_MS);
    lv_slider_set_value(ui_SettingsUpdatePeriodSlider, updatePeriodMs, LV_ANIM_OFF);
    lv_obj_add_event_cb(ui_SettingsUpdatePeriodSlider, [](lv_event_t *e) {
        auto *self = static_cast<SignalsSettingsPanel *>(lv_event_get_user_data(e));
        const int value = lv_slider_get_value(lv_event_get_target(e));
        lv_label_set_text_fmt(self->ui_SettingsUpdatePeriodLabel, "Update period: %d ms", value);
    }, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(ui_SettingsUpdatePeriodSlider, updatePeriodCallback, LV_EVENT_VALUE_CHANGED, userData);

    lv_obj_t *minimum = lv_label_create(ui_SettingsGroup);
    lv_obj_set_pos(minimum, 17, 288);
    lv_obj_set_style_text_font(minimum, &lv_font_montserrat_10, 0);
    lv_label_set_text(minimum, "10 ms");
    lv_obj_t *maximum = lv_label_create(ui_SettingsGroup);
    lv_obj_set_pos(maximum, 190, 288);
    lv_obj_set_style_text_font(maximum, &lv_font_montserrat_10, 0);
    lv_label_set_text(maximum, "1000 ms");

    ui_SettingsDataBundleLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsDataBundleLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsDataBundleLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsDataBundleLabel, 10);
    lv_obj_set_y(ui_SettingsDataBundleLabel, 306);
    lv_label_set_text(ui_SettingsDataBundleLabel, "Data Bundles:");
    lv_obj_set_style_text_color(ui_SettingsDataBundleLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsDataBundleCountLabel = lv_label_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsDataBundleCountLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsDataBundleCountLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SettingsDataBundleCountLabel, -20);
    lv_obj_set_y(ui_SettingsDataBundleCountLabel, 306);
    lv_obj_set_align(ui_SettingsDataBundleCountLabel, LV_ALIGN_TOP_RIGHT);
    const std::string bundleCountText = "[" + std::to_string(bundleAmount) + "/30]";
    lv_label_set_text(ui_SettingsDataBundleCountLabel, bundleCountText.c_str());
    lv_obj_set_style_text_color(
        ui_SettingsDataBundleCountLabel,
        lv_color_hex(isBundleFull ? 0xE55858 : 0x000000),
        LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SettingsDataBundleShowButton = lv_btn_create(ui_SettingsGroup);
    lv_obj_set_width(ui_SettingsDataBundleShowButton, 200);
    lv_obj_set_height(ui_SettingsDataBundleShowButton, 20);
    lv_obj_set_x(ui_SettingsDataBundleShowButton, 17);
    lv_obj_set_y(ui_SettingsDataBundleShowButton, 330);
    lv_obj_add_flag(ui_SettingsDataBundleShowButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_SettingsDataBundleShowButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_SettingsDataBundleShowButton, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_SettingsDataBundleShowButton, showBundlesCallback, LV_EVENT_CLICKED, userData);

    ui_SettingsDataBundleShowButtonLabel = lv_label_create(ui_SettingsDataBundleShowButton);
    lv_obj_set_width(ui_SettingsDataBundleShowButtonLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_SettingsDataBundleShowButtonLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_SettingsDataBundleShowButtonLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SettingsDataBundleShowButtonLabel, "Open Databank");
}

void SignalsSettingsPanel::updateChartValueBlocks(const std::vector<std::string> &chartValueKeys,
                                                  const std::vector<std::string> &selectedChartValueKeys)
{
    const size_t count = ui_SettingsValueBlocks.size();
    for (size_t i = 0; i < count; ++i) {
        lv_obj_t *block = ui_SettingsValueBlocks[i];
        if (!block || i >= chartValueKeys.size()) {
            continue;
        }

        const int selectedSlot = selectedSlotForKey(selectedChartValueKeys, chartValueKeys[i]);
        uint32_t color = 0xA8B3BF;
        if (selectedSlot == 0) {
            color = 0x009BFF;
        } else if (selectedSlot == 1) {
            color = 0xFF6B35;
        }

        lv_obj_set_style_bg_color(block, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(block, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void SignalsSettingsPanel::hide()
{
    if (ui_SettingsOverlay != nullptr) {
        lv_obj_del(ui_SettingsOverlay);
        ui_SettingsOverlay = nullptr;
    }

    if (ui_SettingsBridgeGroup != nullptr) {
        lv_obj_del(ui_SettingsBridgeGroup);
        ui_SettingsBridgeGroup = nullptr;
    }

    ui_SettingsBridge = nullptr;
    ui_SettingsBridgeFill = nullptr;
    ui_SettingsGroup = nullptr;
    ui_SettingsOutlay = nullptr;
    ui_SettingsHeaderLine = nullptr;
    ui_SettingsHeaderLabel = nullptr;
    ui_SettingsScaleModeLabel = nullptr;
    ui_SettingsScaleModeValueLabel = nullptr;
    ui_SettingsSeriesColorLabel = nullptr;
    ui_SettingsPrimarySwatch = nullptr;
    ui_SettingsSecondarySwatch = nullptr;
    ui_SettingsGraphValuesLabel = nullptr;
    ui_SettingsManualScaleButton = nullptr;
    ui_SettingsManualScaleButtonLabel = nullptr;
    ui_SettingsUpdatePeriodLabel = nullptr;
    ui_SettingsUpdatePeriodSlider = nullptr;
    ui_SettingsDataBundleLabel = nullptr;
    ui_SettingsDataBundleCountLabel = nullptr;
    ui_SettingsDataBundleShowButton = nullptr;
    ui_SettingsDataBundleShowButtonLabel = nullptr;
    ui_SettingsValueBlocks.clear();
    ui_SettingsValueBlockLabels.clear();
}

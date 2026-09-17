#include "signals_manual_scale_panel.hpp"
#include <algorithm>
#include <cstdio>
#include <utility>

namespace {
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y) {
    auto *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    return obj;
}
lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y,
                 lv_event_cb_t callback, void *data) {
    auto *obj = lv_btn_create(parent);
    lv_obj_set_size(obj, 140, 38);
    lv_obj_set_pos(obj, x, y);
    auto *caption = lv_label_create(obj);
    lv_label_set_text(caption, text);
    lv_obj_center(caption);
    lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
    return obj;
}
}

void SignalsManualScalePanel::show(const ChartManualScale& scale,
                                 std::function<void(const ChartManualScale&)> apply) {
    hide();
    if (!scale.valid()) return;
    draft = scale;
    onApply = std::move(apply);
    const double span = draft.yMax - draft.yMin;
    domainMin = draft.yMin - span;
    domainMax = draft.yMax + span;
    if (!std::isfinite(domainMin) || !std::isfinite(domainMax) ||
        !std::isfinite(domainMax - domainMin)) {
        domainMin = draft.yMin;
        domainMax = draft.yMax;
    }

    overlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    auto *window = lv_obj_create(overlay);
    lv_obj_set_size(window, 600, 290);
    lv_obj_center(window);
    lv_obj_clear_flag(window, LV_OBJ_FLAG_SCROLLABLE);
    label(window, "Manual Y Scale", 8, 0);
    label(window, "Y: signal value (both series)", 8, 35);
    yLabel = label(window, "", 8, 62);
    ySlider = lv_slider_create(window);
    lv_obj_set_size(ySlider, 520, 18);
    lv_obj_set_pos(ySlider, 18, 98);
    lv_slider_set_mode(ySlider, LV_SLIDER_MODE_RANGE);
    lv_slider_set_range(ySlider, 0, 1000);
    syncYSlider();
    lv_obj_add_event_cb(ySlider, handleSlider, LV_EVENT_VALUE_CHANGED, this);
    button(window, "Y finer", 8, 138, [](lv_event_t *e) {
        static_cast<SignalsManualScalePanel*>(lv_event_get_user_data(e))->resizeYDomain(false);
    }, this);
    button(window, "Y wider", 160, 138, [](lv_event_t *e) {
        static_cast<SignalsManualScalePanel*>(lv_event_get_user_data(e))->resizeYDomain(true);
    }, this);
    errorLabel = label(window, "", 8, 183);
    lv_obj_set_style_text_color(errorLabel, lv_color_hex(0xDD3333), 0);
    button(window, "Cancel", 250, 210, [](lv_event_t *e) {
        static_cast<SignalsManualScalePanel*>(lv_event_get_user_data(e))->hide();
    }, this);
    button(window, "Apply", 402, 210, [](lv_event_t *e) {
        auto *self = static_cast<SignalsManualScalePanel*>(lv_event_get_user_data(e));
        if (!self->draft.valid()) {
            lv_label_set_text(self->errorLabel, "Minimum must be less than maximum.");
            return;
        }
        const auto settings = self->draft;
        auto callback = self->onApply;
        self->hide();
        if (callback) callback(settings);
    }, this);
    refreshLabels();
}

void SignalsManualScalePanel::handleSlider(lv_event_t *event) {
    auto *self = static_cast<SignalsManualScalePanel*>(lv_event_get_user_data(event));
    const double span = self->domainMax - self->domainMin;
    self->draft.yMin = self->domainMin + span * lv_slider_get_left_value(self->ySlider) / 1000.0;
    self->draft.yMax = self->domainMin + span * lv_slider_get_value(self->ySlider) / 1000.0;
    self->refreshLabels();
}

void SignalsManualScalePanel::refreshLabels() {
    // Use snprintf: LVGL's formatter may have float formatting disabled.
    char text[100];
    std::snprintf(text, sizeof(text), "Min: %.9g     Max: %.9g", draft.yMin, draft.yMax);
    lv_label_set_text(yLabel, text);
    lv_label_set_text(errorLabel, "");
}

void SignalsManualScalePanel::syncYSlider() {
    const double span = domainMax - domainMin;
    const int left = static_cast<int>(std::lround((draft.yMin - domainMin) / span * 1000));
    const int right = static_cast<int>(std::lround((draft.yMax - domainMin) / span * 1000));
    lv_slider_set_left_value(ySlider, 0, LV_ANIM_OFF);
    lv_slider_set_value(ySlider, right, LV_ANIM_OFF);
    lv_slider_set_left_value(ySlider, left, LV_ANIM_OFF);
}

void SignalsManualScalePanel::resizeYDomain(bool wider) {
    if (!draft.valid()) return;
    const double middle = draft.yMin + (draft.yMax - draft.yMin) / 2;
    const double half = wider ? (domainMax - domainMin) : (draft.yMax - draft.yMin) * 0.6;
    if (!std::isfinite(middle - half) || !std::isfinite(middle + half) ||
        !std::isfinite(half * 2) || half <= 0 || middle - half >= middle + half) return;
    domainMin = middle - half;
    domainMax = middle + half;
    syncYSlider();
}

void SignalsManualScalePanel::hide() {
    if (overlay) lv_obj_del(overlay);
    overlay = ySlider = yLabel = errorLabel = nullptr;
    onApply = nullptr;
}

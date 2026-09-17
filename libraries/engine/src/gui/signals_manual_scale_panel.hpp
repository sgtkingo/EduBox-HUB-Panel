#pragma once

#include "lvgl.h"
#include "chart_manual_scale.hpp"
#include <functional>

class SignalsManualScalePanel {
    lv_obj_t *overlay = nullptr;
    lv_obj_t *ySlider = nullptr;
    lv_obj_t *yLabel = nullptr;
    lv_obj_t *errorLabel = nullptr;
    ChartManualScale draft;
    double domainMin = -10;
    double domainMax = 10;
    std::function<void(const ChartManualScale&)> onApply;

    void refreshLabels();
    void syncYSlider();
    void resizeYDomain(bool wider);
    static void handleSlider(lv_event_t *event);
public:
    void show(const ChartManualScale& scale,
              std::function<void(const ChartManualScale&)> apply);
    void hide();
};

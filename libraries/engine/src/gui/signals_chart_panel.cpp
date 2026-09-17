/**
 * @file signals_chart_panel.cpp
 * @brief Chart panel helper for SignalsVisualizationGui.
 */

#include "signals_chart_panel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

void SignalsChartPanel::create(lv_obj_t *parent)
{
    chart = lv_chart_create(parent);
    lv_obj_set_width(chart, 430);
    lv_obj_set_height(chart, 280);
    lv_obj_set_x(chart, 115);
    lv_obj_set_y(chart, 20);
    lv_obj_set_align(chart, LV_ALIGN_CENTER);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, HISTORY_CAP);
    lv_chart_set_div_line_count(chart, 5, HISTORY_CAP);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 5, 0, HISTORY_CAP, 1, true, 50);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 5, 2, true, 42);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, 8, 4, 0, 2, false, 42);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, PLOT_MIN, PLOT_MAX);
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, PLOT_MIN, PLOT_MAX);
    lv_obj_add_event_cb(chart, handleDrawPartEvent, LV_EVENT_DRAW_PART_BEGIN, this);

    primarySeries = lv_chart_add_series(chart, lv_color_hex(0x009BFF), LV_CHART_AXIS_PRIMARY_Y);
    secondarySeries = lv_chart_add_series(chart, lv_color_hex(0xFF6B35), LV_CHART_AXIS_SECONDARY_Y);

    lv_obj_set_style_bg_color(chart, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(chart, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chart, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x000000), LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(chart, 255, LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(chart, lv_color_hex(0x000000), LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(chart, 255, LV_PART_TICKS | LV_STATE_DEFAULT);

    emptyLabel = lv_label_create(chart);
    lv_label_set_text(emptyLabel, "No numeric signal available");
    lv_obj_center(emptyLabel);
    lv_obj_set_style_text_color(emptyLabel, lv_color_hex(0x5F6B7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(emptyLabel, LV_OBJ_FLAG_HIDDEN);

    scalingLabel = lv_label_create(parent);
    lv_label_set_text(scalingLabel, "Scaling 1x");
    lv_obj_set_width(scalingLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(scalingLabel, LV_SIZE_CONTENT);
    lv_obj_align_to(scalingLabel, chart, LV_ALIGN_OUT_TOP_LEFT, 0, -5);
    lv_obj_set_style_text_color(scalingLabel, lv_color_hex(0x009BFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(scalingLabel, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

    secondaryScalingLabel = lv_label_create(parent);
    lv_label_set_text(secondaryScalingLabel, "");
    lv_obj_set_width(secondaryScalingLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(secondaryScalingLabel, LV_SIZE_CONTENT);
    lv_obj_align_to(secondaryScalingLabel, chart, LV_ALIGN_OUT_TOP_RIGHT, -42, -5);
    lv_obj_set_style_text_color(secondaryScalingLabel, lv_color_hex(0xFF6B35), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(secondaryScalingLabel, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(secondaryScalingLabel, LV_OBJ_FLAG_HIDDEN);

    samplesLabel = lv_label_create(parent);
    lv_label_set_text(samplesLabel, "Samples 10");
    lv_obj_set_width(samplesLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(samplesLabel, LV_SIZE_CONTENT);
    lv_obj_align_to(samplesLabel, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
    lv_obj_set_style_text_color(samplesLabel, lv_color_hex(0x5F6B7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(samplesLabel, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void SignalsChartPanel::showEmptyState(const char *message)
{
    if (!isReady()) {
        return;
    }

    clearSeries();
    if (emptyLabel) {
        lv_label_set_text(emptyLabel, message ? message : "");
        lv_obj_clear_flag(emptyLabel, LV_OBJ_FLAG_HIDDEN);
    }
    refresh();
}

void SignalsChartPanel::hideEmptyState()
{
    if (emptyLabel) {
        lv_obj_add_flag(emptyLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void SignalsChartPanel::setScalingText(const char *primaryText, const char *secondaryText)
{
    primaryScalingBase = primaryText ? primaryText : "";
    secondaryScalingBase = secondaryText ? secondaryText : "";
    refreshScalingLabels();
}

void SignalsChartPanel::refreshScalingLabels()
{
    if (!secondaryScalingLabel) {
        if (scalingLabel) {
            lv_label_set_text(scalingLabel, primaryScalingBase.c_str());
        }
        return;
    }

    auto buildLabel = [](const std::string &base, double minValue, double maxValue) {
        const int exponent = SignalsChartPanel::axisScaleExponent(minValue, maxValue);
        if (exponent == 0) {
            return base;
        }

        char suffix[16];
        std::snprintf(suffix, sizeof(suffix), " x1e%d", exponent);
        return base.empty() ? std::string(suffix + 1) : base + suffix;
    };

    if (scalingLabel) {
        const std::string text = buildLabel(primaryScalingBase, primaryRawMin, primaryRawMax);
        lv_label_set_text(scalingLabel, text.c_str());
    }

    if (!secondaryScalingBase.empty()) {
        const std::string text = buildLabel(secondaryScalingBase, secondaryRawMin, secondaryRawMax);
        lv_label_set_text(secondaryScalingLabel, text.c_str());
        lv_obj_clear_flag(secondaryScalingLabel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(secondaryScalingLabel, "");
        lv_obj_add_flag(secondaryScalingLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void SignalsChartPanel::setSamplesText(const char *text)
{
    if (samplesLabel) {
        lv_label_set_text(samplesLabel, text ? text : "");
    }
}

void SignalsChartPanel::setVisibleSampleCount(int sampleCount)
{
    if (!chart || sampleCount <= 0) {
        return;
    }

    lv_chart_set_point_count(chart, static_cast<uint16_t>(sampleCount));
    visibleSampleCount = sampleCount;
    xTickMax = sampleCount <= 20 ? sampleCount - 1 : 10;
    if (xTickMax < 1) {
        xTickMax = 1;
    }
    lv_chart_set_div_line_count(chart, 5, xTickMax + 1);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 5, 0, xTickMax + 1, 1, true, 50);
}

void SignalsChartPanel::setRange(double primaryMinValue,
                                 double primaryMaxValue,
                                 double secondaryMinValue,
                                 double secondaryMaxValue,
                                 bool hasSecondarySeries)
{
    if (!chart) {
        return;
    }

    primaryRawMin = primaryMinValue;
    primaryRawMax = primaryMaxValue;
    secondaryRawMin = hasSecondarySeries ? secondaryMinValue : primaryMinValue;
    secondaryRawMax = hasSecondarySeries ? secondaryMaxValue : primaryMaxValue;
    hasSecondaryRawRange = hasSecondarySeries;

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, PLOT_MIN, PLOT_MAX);
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, PLOT_MIN, PLOT_MAX);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 8, 4, 5, 2, true, 42);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, 8, 4, hasSecondarySeries ? 5 : 0, 2, hasSecondarySeries, 42);
    refreshScalingLabels();
}

int SignalsChartPanel::axisScaleExponent(double minValue, double maxValue)
{
    const double maxAbs = std::max(std::fabs(minValue), std::fabs(maxValue));
    if (maxAbs == 0.0 || (maxAbs >= 0.001 && maxAbs <= 9999.0)) {
        return 0;
    }

    int exponent = static_cast<int>(std::floor(std::log10(maxAbs) / 3.0)) * 3;
    if (exponent > 9) exponent = 9;
    if (exponent < -9) exponent = -9;
    return exponent;
}

double SignalsChartPanel::axisScaleDivisor(int exponent)
{
    return exponent == 0 ? 1.0 : std::pow(10.0, static_cast<double>(exponent));
}

void SignalsChartPanel::formatAxisLabel(char *buffer, size_t bufferSize, double value, double span, int exponent)
{
    if (!buffer || bufferSize == 0) {
        return;
    }

    const double displayValue = value / axisScaleDivisor(exponent);
    const double absSpan = std::fabs(span / axisScaleDivisor(exponent));
    if (absSpan < 1.0) {
        std::snprintf(buffer, bufferSize, "%.3f", displayValue);
    } else if (absSpan < 20.0) {
        std::snprintf(buffer, bufferSize, "%.2f", displayValue);
    } else if (absSpan < 200.0) {
        std::snprintf(buffer, bufferSize, "%.1f", displayValue);
    } else {
        std::snprintf(buffer, bufferSize, "%.0f", displayValue);
    }

    char *dot = std::strchr(buffer, '.');
    if (dot) {
        char *end = buffer + std::strlen(buffer) - 1;
        while (end > dot && *end == '0') {
            *end-- = '\0';
        }
        if (end == dot) {
            *end = '\0';
        }
    }
}

void SignalsChartPanel::applyTickLabelFont(lv_obj_draw_part_dsc_t *dsc)
{
    if (!dsc || !dsc->label_dsc || !dsc->text) {
        return;
    }

    const size_t length = std::strlen(dsc->text);
    dsc->label_dsc->font = length > 5 ? &lv_font_montserrat_10 : &lv_font_montserrat_12;
}

void SignalsChartPanel::handleDrawPartEvent(lv_event_t *e)
{
    auto *panel = static_cast<SignalsChartPanel *>(lv_event_get_user_data(e));
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (!panel || !dsc || dsc->part != LV_PART_TICKS || dsc->text == nullptr) {
        return;
    }

    if (dsc->id == LV_CHART_AXIS_PRIMARY_X) {
        const int sampleIndex = panel->xTickMax <= 0
                                    ? 0
                                    : static_cast<int>(std::lround((static_cast<double>(dsc->value) / panel->xTickMax) * (panel->visibleSampleCount - 1)));
        std::snprintf(dsc->text, dsc->text_length, "%d", sampleIndex);
        applyTickLabelFont(dsc);
        return;
    }

    if (dsc->id != LV_CHART_AXIS_PRIMARY_Y && dsc->id != LV_CHART_AXIS_SECONDARY_Y) {
        return;
    }

    const bool secondary = dsc->id == LV_CHART_AXIS_SECONDARY_Y;
    if (secondary && !panel->hasSecondaryRawRange) {
        return;
    }

    const double minValue = secondary ? panel->secondaryRawMin : panel->primaryRawMin;
    const double maxValue = secondary ? panel->secondaryRawMax : panel->primaryRawMax;
    const double span = maxValue - minValue;
    const double normalized = static_cast<double>(dsc->value - PLOT_MIN) / static_cast<double>(PLOT_MAX - PLOT_MIN);
    const double rawValue = minValue + normalized * span;
    const int exponent = axisScaleExponent(minValue, maxValue);
    formatAxisLabel(dsc->text, dsc->text_length, rawValue, span, exponent);
    applyTickLabelFont(dsc);
}

void SignalsChartPanel::clearSeries()
{
    if (!isReady()) {
        return;
    }

    lv_chart_set_all_value(chart, primarySeries, LV_CHART_POINT_NONE);
    lv_chart_set_all_value(chart, secondarySeries, LV_CHART_POINT_NONE);
}

void SignalsChartPanel::populatePrimarySeries(const lv_coord_t *history, int sampleCount)
{
    if (!isReady() || !history || sampleCount <= 0) {
        return;
    }

    for (int i = 0; i < sampleCount; ++i) {
        lv_chart_set_value_by_id(chart, primarySeries, i, history[i]);
    }
}

void SignalsChartPanel::populateSecondarySeries(const lv_coord_t *history, int sampleCount)
{
    if (!isReady() || !history || sampleCount <= 0) {
        return;
    }

    for (int i = 0; i < sampleCount; ++i) {
        lv_chart_set_value_by_id(chart, secondarySeries, i, history[i]);
    }
}

void SignalsChartPanel::hideSecondarySeries()
{
    if (!isReady()) {
        return;
    }

    lv_chart_set_all_value(chart, secondarySeries, LV_CHART_POINT_NONE);
}

void SignalsChartPanel::refresh()
{
    if (chart) {
        lv_chart_refresh(chart);
        lv_obj_invalidate(chart);
    }
}

void SignalsChartPanel::resetToZero()
{
    if (!isReady()) {
        return;
    }

    lv_chart_set_all_value(chart, primarySeries, 0);
    lv_chart_set_all_value(chart, secondarySeries, 0);
    refresh();
}

void SignalsChartPanel::setSeriesColors(uint32_t primary, uint32_t secondary)
{
    if (!isReady()) return;
    lv_chart_set_series_color(chart, primarySeries, lv_color_hex(primary));
    lv_chart_set_series_color(chart, secondarySeries, lv_color_hex(secondary));
    if (scalingLabel) lv_obj_set_style_text_color(scalingLabel, lv_color_hex(primary), LV_PART_MAIN);
    if (secondaryScalingLabel) lv_obj_set_style_text_color(secondaryScalingLabel, lv_color_hex(secondary), LV_PART_MAIN);
    refresh();
}

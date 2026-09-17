/**
 * @file signals_chart_panel.hpp
 * @brief Chart panel helper for SignalsVisualizationGui.
 */

#ifndef SIGNALS_CHART_PANEL_HPP
#define SIGNALS_CHART_PANEL_HPP

#include <cstddef>
#include <string>
#include "lvgl.h"

#include "../devices/base_device.hpp"

class SignalsChartPanel
{
private:
    static const lv_coord_t PLOT_MIN = 0;
    static const lv_coord_t PLOT_MAX = 1000;

    lv_obj_t *chart = nullptr;
    lv_obj_t *emptyLabel = nullptr;
    lv_obj_t *scalingLabel = nullptr;
    lv_obj_t *secondaryScalingLabel = nullptr;
    lv_obj_t *samplesLabel = nullptr;
    std::string primaryScalingBase;
    std::string secondaryScalingBase;
    lv_chart_series_t *primarySeries = nullptr;
    lv_chart_series_t *secondarySeries = nullptr;
    double primaryRawMin = 0.0;
    double primaryRawMax = 1.0;
    double secondaryRawMin = 0.0;
    double secondaryRawMax = 1.0;
    bool hasSecondaryRawRange = false;
    int visibleSampleCount = HISTORY_CAP;
    int xTickMax = HISTORY_CAP - 1;

    static void handleDrawPartEvent(lv_event_t *e);
    static int axisScaleExponent(double minValue, double maxValue);
    static double axisScaleDivisor(int exponent);
    static void formatAxisLabel(char *buffer, size_t bufferSize, double value, double span, int exponent);
    static void applyTickLabelFont(lv_obj_draw_part_dsc_t *dsc);
    void refreshScalingLabels();

public:
    void create(lv_obj_t *parent);

    bool isReady() const { return chart && primarySeries && secondarySeries; }
    lv_obj_t *getChart() const { return chart; }

    void showEmptyState(const char *message);
    void hideEmptyState();
    void setScalingText(const char *primaryText, const char *secondaryText = nullptr);
    void setSamplesText(const char *text);
    void setVisibleSampleCount(int sampleCount);
    void setRange(double primaryMinValue,
                  double primaryMaxValue,
                  double secondaryMinValue,
                  double secondaryMaxValue,
                  bool hasSecondarySeries);
    void setSeriesColors(uint32_t primary, uint32_t secondary);
    void clearSeries();
    void populatePrimarySeries(const lv_coord_t *history, int sampleCount);
    void populateSecondarySeries(const lv_coord_t *history, int sampleCount);
    void hideSecondarySeries();
    void refresh();
    void resetToZero();
};

#endif // SIGNALS_CHART_PANEL_HPP

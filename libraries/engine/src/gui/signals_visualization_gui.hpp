/**
 * @file signals_visualization_gui.hpp
 * @brief Declaration of the SignalsVisualizationGui widget
 *
 * This header defines the SignalsVisualizationGui class which handles
 * active device visualization, data display, synchronization, and navigation.
 *
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny, Ondřej Wrubel
 */

#ifndef SIGNALS_VISUALIZATION_GUI_HPP
#define SIGNALS_VISUALIZATION_GUI_HPP

#include "lvgl.h"
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "gui_router.hpp"
#include "signals_chart_panel.hpp"
#include "signals_manual_scale_panel.hpp"
#include "signals_feedback_panel.hpp"
#include "signals_list_panel.hpp"
#include "signals_settings_panel.hpp"
#include "signals_toolbar_panel.hpp"
#include "../managers/device_manager.hpp"
#include "../managers/device_visualization_session.hpp"
#include "../managers/data_bundle_manager.hpp"
#include "../exceptions/data_exceptions.hpp"

/**
 * @class SignalsVisualizationGui
 * @brief Handles active device visualization, data display, and navigation.
 *
 * This class is responsible for:
 * - Displaying current device data and values
 * - Handling device navigation (prev/next)
 * - Managing device synchronization operations
 * - Handling device-specific events and interactions
 */
class SignalsVisualizationGui
{
private:
    GuiRouter &router;
    DeviceManager &deviceManager;        ///< Reference to the device manager instance
    DeviceVisualizationSession &visualizationSession; ///< Active visualization session of selected devices
    DataBundleManager &dataBundleManager;///< Reference to the databundle manager instance
    BaseDevice *currentDevice = nullptr; ///< Currently visualized device

    static const int CHART_HISTORY_CAP = HISTORY_CAP * 12;
    static const int CHART_MIN_VISIBLE_SAMPLES = HISTORY_CAP;
    static const int CHART_MAX_VISIBLE_SAMPLES = 100;
    static const int CHART_SAMPLE_STEP = 5;
    static const lv_coord_t CHART_PLOT_MIN = 0;
    static const lv_coord_t CHART_PLOT_MAX = 1000;

    /// Static buffers for chart data
    std::map<std::string, std::array<double, CHART_HISTORY_CAP>> bufMap;
    std::map<std::string, bool> initedMap;
    std::map<std::string, int> historyCountMap;

    bool initialized = false; ///< Initialization state flag
    bool paused = false;      ///< Pause state flag
    bool recording = false;   ///< Recording state flag
    bool showingConfigPanel = false;
    uint8_t primaryColorIndex = 0;
    uint8_t secondaryColorIndex = 1;
    void refreshSeriesColors();
    static void handleSeriesColorEvent(lv_event_t *event);
    bool manualScaleEnabled = false;
    bool manualScaleConfigured = false;
    ChartManualScale manualScale;
    SignalsManualScalePanel manualScalePanel;
    double autoYMin = -1.0;
    double autoYMax = 1.0;
    void setManualScaleEnabled(bool enabled);
    void showManualScalePanel();
    void refreshScaleModeControls();
    static void handleScaleModeEvent(lv_event_t *event);
    static void handleManualScaleEvent(lv_event_t *event);
    int chartHistoryOffset = 0;
    int chartDragAccumulatorPx = 0;
    int chartVisibleSampleCount = HISTORY_CAP;
    int chartPinchLastDistancePx = 0;
    int chartPinchAccumulatorPx = 0;
    bool chartPinchActive = false;
    std::string chartSelectedDeviceId;
    std::vector<std::string> chartSelectedValueKeys;

    void createMainWidget();
    void createTitleLabel();
    void createSignalScrollPanel();
    void createListModeToggle();
    void createChartPanel();

    // --- DEVICE VISUALIZATION MEMBERS ---
    lv_obj_t *ui_DeviceWidget; ///< Widget for device visualisation
    lv_obj_t *ui_DeviceLabel;  ///< Label for device name
    lv_obj_t *ui_ListModeButton = nullptr;
    lv_obj_t *ui_ListModeButtonLabel = nullptr;
    lv_obj_t *ui_ListModeTitle = nullptr;

    SignalsListPanel signalListPanel; ///< Scrollable panel for live values and editable controls
    SignalsChartPanel chartPanel;     ///< Chart panel for numeric signal history
    SignalsFeedbackPanel feedbackPanel; ///< Transient alerts and modal shadow overlay
    SignalsSettingsPanel settingsPanel; ///< Settings overlay panel for runtime visualization actions
    SignalsToolbarPanel toolbarPanel; ///< Bottom navigation and recording toolbar

    // --- NAVIGATION AND CONTROL MEMBERS ---
    lv_obj_t *ui_LogoGroup;                              ///< Logo panel widget
    lv_obj_t *ui_LogoCornerBottomLeft;                   ///< Decorative corner for logo panel
    lv_obj_t *ui_LogoCornerFillBottomLeft;               ///< Decorative fill for logo
    lv_obj_t *ui_LogoCornerTopRight;                     ///< Decorative corner for logo panel
    lv_obj_t *ui_LogoCornerFillBottomRight;              ///< Decorative fill for logo
    lv_obj_t *ui_LogoOutlay;                             ///< Decorative outlay for logo panel
    lv_obj_t *ui_LogoImage;                              ///< Logo image widget

    void createToolbarPanel();

    /**
     * @brief Add logo panel to a widget
     * @param parentWidget The parent widget to add the logo panel to
     */
    void addLogoPanelToWidget(lv_obj_t *parentWidget);

    /**
     * @brief Build device history data for chart display
     * @param device Pointer to the device
     * @param key The key of the device parameter
     * @param history The history array to store the history
     */
    template <typename T>
    void buildDeviceHistory(BaseDevice *device, const std::string &key, double *history, bool appendSample)
    {
        if (!history || !device)
            return;

        const auto values = device->getValues();
        auto it = values.find(key);
        if (it == values.end())
            return;

        const std::string historyKey = makeHistoryBufferKey(device, key);
        auto &buf = bufMap[historyKey];
        bool &inited = initedMap[historyKey];
        int &count = historyCountMap[historyKey];

        // Get current value as string and convert
        double curr = 0.0;
        try
        {
            std::string s = device->getValue<std::string>(key);
            const T value = convertStringToType<T>(s);
            curr = static_cast<double>(value);

            debugLogMessage("SignalsVisualizationGui::buildDeviceHistory", "math conversion", "device=%s key=%s raw=%s rawValue=%.4f append=%d", device->UID.c_str(), key.c_str(), s.c_str(), curr, appendSample);
        }
        catch (const std::exception &e)
        {
            throw InvalidDataTypeException("SignalsVisualizationGui::buildDeviceHistory", e.what());
        }

        if (!inited)
        {
            // First call: seed the chart buffer from the device-owned history.
            for (int i = 0; i < CHART_HISTORY_CAP; ++i)
            {
                buf[i] = curr;
            }

            std::string *deviceHistory = device->getHistory(key);
            const int nextHistoryIndex = it->second.lastHistoryIndex;
            for (int i = 0; i < HISTORY_CAP; ++i)
            {
                const int sourceIndex = (nextHistoryIndex + i) % HISTORY_CAP;
                const std::string &rawHistoryValue = deviceHistory[sourceIndex];
                if (rawHistoryValue.empty())
                {
                    continue;
                }

                const T historyValue = convertStringToType<T>(rawHistoryValue);
                buf[i] = static_cast<double>(historyValue);
            }

            count = HISTORY_CAP;
            inited = true;
        }
        else if (appendSample)
        {
            // Shift left by one position
            if (count < CHART_HISTORY_CAP)
            {
                buf[count++] = curr;
            }
            else
            {
                for (int i = 0; i < CHART_HISTORY_CAP - 1; ++i)
                {
                    buf[i] = buf[i + 1];
                }
                buf[CHART_HISTORY_CAP - 1] = curr;
            }
        }

        const int visibleSamples = chartVisibleSampleCount;
        const int maxOffset = count > visibleSamples ? count - visibleSamples : 0;
        if (chartHistoryOffset > maxOffset)
        {
            chartHistoryOffset = maxOffset;
        }

        int start = count - visibleSamples - chartHistoryOffset;
        if (start < 0)
        {
            start = 0;
        }

        // Copy selected window to output array
        for (int i = 0; i < visibleSamples; ++i)
        {
            try
            {
                const int sourceIndex = start + i;
                history[i] = sourceIndex < count ? buf[sourceIndex] : buf[count - 1];
            }
            catch (const std::exception &e)
            {
                throw InvalidDataTypeException("SignalsVisualizationGui::buildDeviceHistory", e.what());
            }
        }
    }

    std::string makeHistoryBufferKey(const BaseDevice *device, const std::string &key) const
    {
        return device ? device->getId() + ":" + key : key;
    }

    void clearDeviceHistoryBuffer(const std::string &key)
    {
        const std::string historyKey = makeHistoryBufferKey(currentDevice, key);
        std::array<double, CHART_HISTORY_CAP> zeroBuf;
        zeroBuf.fill(0);

        bufMap[historyKey] = zeroBuf;
        initedMap[historyKey] = true;
        historyCountMap[historyKey] = HISTORY_CAP;
    }

    /**
     * @brief Update device data display
     */
    void updateDeviceDataDisplay();

    /**
     * @brief Update chart with current device data
     */
    void updateChart(bool force = false);

    void updateDeviceTitle();
    void ensureSignalCards(size_t count);
    void clearUnusedSignalCards(size_t usedCount);
    void updateSignalCards(const std::unordered_map<std::string, DeviceParam> &values,
                           const std::vector<std::string> &valueKeys);
    void ensureConfigControls(size_t count);
    void clearUnusedConfigControls(size_t usedCount);
    void updateEditableControls(const std::unordered_map<std::string, DeviceParam> &params,
                                const std::vector<std::string> &keys,
                                bool isValueControl);
    void updateListModeToggleState();
    void toggleListMode();
    void ensureControlEditor(size_t controlIndex, const DeviceParam &param);
    void syncControlEditorValue(size_t controlIndex, const DeviceParam &param);
    bool buildNumericHistoryForKey(const std::string &key, double *history, bool appendSample);
    void recordCurrentSamples(const std::vector<std::string> &valueKeys);
    std::vector<std::string> getRecordableValueKeys() const;
    std::vector<std::string> getAvailableChartValueKeys() const;
    std::vector<std::string> getActiveChartValueKeys();
    void ensureActiveChartValueKeys();
    void toggleChartValueSelection(size_t valueIndex);
    std::string buildChartScalingText(const std::string &chartKey,
                                             const std::unordered_map<std::string, DeviceParam> &values);
    void showEmptyChartState(const char *message);
    void hideEmptyChartState();
    static std::pair<double, double> computeChartRange(const double *history);
    static std::pair<double, double> computeChartRange(const double *history, int sampleCount);
    void mapHistoryToPlot(const double *rawHistory, lv_coord_t *plotHistory, int sampleCount, double minValue, double maxValue);
    int getMaxChartHistoryOffset(const std::vector<std::string> &chartKeys) const;
    void panChartHistory(int steps);
    void adjustChartVisibleSamples(int deltaSamples);
    void updateChartSampleLabel();
    bool handleChartPinchGesture();
    void handleChartDrag(lv_event_t *e);
    bool beginDeviceNavigation(bool requireIdleRecording, bool &wasRunning);
    void finishDeviceNavigation(bool wasRunning, BaseDevice *nextDevice);
    bool currentDeviceSupportsRecording() const;
    void updateActionButtonsState();
    bool applyEditableValue(bool isValueControl, const std::string &key, const std::string &value);
    void handleDropdownConfigChanged(size_t controlIndex);
    void handleSliderConfigChanged(size_t controlIndex);
    void handleTextConfigSubmitted(size_t controlIndex);
    static uint32_t getSignalAccentColor(size_t index);
    static bool isNumericType(DeviceDataType dtype);
    static bool hasSelectableOptions(const DeviceParam &param);
    static bool supportsSliderInput(const DeviceParam &param);
    static std::string buildUnitText(const std::string &unit, const char *fallbackText);

public:
    /**
     * @brief Constructor
     * @param deviceManager Reference to the device manager instance
     */
    SignalsVisualizationGui(GuiRouter &router, DeviceManager &deviceManager, DeviceVisualizationSession &visualizationSession, DataBundleManager &dataBundleManager);

    /**
     * @brief Destructor
     */
    ~SignalsVisualizationGui() = default;

    /**
     * @brief Initialize the device visualization GUI
     */
    void init();

    /**
     * @brief Check if the visualization GUI has been initialized
     * @return True if initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Construct the device visualization widget
     */
    void constructVisualization();

    /**
     * @brief Get parent widget
     */
    lv_obj_t *getParentWidget() const { return ui_DeviceWidget; }

    /**
     * @brief Draw/update the currently selected device visualization
     */
    void drawCurrentDevice(bool force = false);

    /**
     * @brief Go to the previous device in the list
     */
    void goToPreviousDevice();

    /**
     * @brief Go to the next device in the list
     */
    void goToNextDevice();

    /**
     * @brief Go to the first device in the list
     */
    void goToFirstDevice();

    /**
     * @brief Handle back button click event
     */
    void handleBackButtonClick();

    /**
     * @brief Handle pause button click event
     */
    void handlePauseButtonClick();

    /**
     * @brief Handle manual runtime update button click event
     */
    void handleSyncButtonClick();

    /**
     * @brief Handle record button click event
     * @param message Message to display on alert (this handle is called in different ways). Can be empty (default is "Record was saved")
     */
    void handleRecordButtonClick(const char *message);

    /**
     * @brief opens a confirmation dialog to clear the current device's history data
     */
    void handleClearButtonClick();

    /**
     * @brief clears the current device's history data upon confirmation
     */
    void handleClearConfirmButtonClick();

    /**
     * @brief Show settings panel
     * @param recordGroup The record panel group to keep above the overlay
     * @param btnSettings The settings button to keep above the overlay
     */
    void handleSettingsButtonClick(lv_obj_t *recordGroup, lv_obj_t *btnSettings, lv_obj_t *parentWidget);
    void handleChartValueSelectionClick(size_t valueIndex);

    /**
     * @brief Handle data bundle show button click event
     */
    void handleDataBundleShowButtonClick();

    /**
     * @brief User should be stopped when recording is still ongoing and they want to click on any bundle related buttons
     */
    void handleStillRecording();

    /**
     * @brief Synchronize the current device data
     * @return True if synchronization was successful, false otherwise
     */
    bool syncCurrentDevice();

    /**
     * @brief Show shadow overlay
     * @param popup The popup dialog which is meant to be highlighted
     */
    void showShadowOverlay();

    /**
     * @brief Hide shadow overlay
     */
    void hideShadowOverlay();

    /**
     * @brief Hide settings panel
     */
    void hideSettingsPanel(bool resume = true);

    /**
     * @brief Show the device visualization screen
     */
    void showVisualization();

    /**
     * @brief Hide the device visualization screen
     */
    void hideVisualization();

    /**
     * @brief Show alert
     * @param message The alert message to display
     */
    void showAlert(const char *message);

    /**
     * @brief Hide alert
     */
    void hideAlert();
};

#endif // SIGNALS_VISUALIZATION_GUI_HPP

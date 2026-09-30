#ifndef COMMUNICATION_SELECTION_GUI_HPP
#define COMMUNICATION_SELECTION_GUI_HPP

#include "lvgl.h"

#include "gui_router.hpp"
#include <string>
#include "../managers/device_manager.hpp"

class CommunicationSelectionGui
{
private:
    GuiRouter &router;
    enum class WirelessFlow : uint8_t { Manual, CablePairing, AlreadyPaired, LocalForgetting, TargetScanning, Connecting, Success, Forgotten, Error };
    WirelessFlow wirelessFlow = WirelessFlow::Manual;
    DefaultCommunicationMode wirelessCompletionMode = DefaultCommunicationMode::WIRELESS_MANUAL;
    std::string targetBoardId;
    uint32_t targetPin = 0, wirelessFlowStarted = 0, lastPairAttempt = 0;
    DeviceManager &deviceManager;
    bool initialized = false;
    lv_obj_t *wirelessInstruction = nullptr, *wirelessSpinner = nullptr;
    lv_obj_t *wirelessConnect = nullptr, *wirelessScan = nullptr, *wirelessForget = nullptr;
    lv_obj_t *wirelessAction = nullptr, *wirelessContinue = nullptr, *wirelessBack = nullptr;
    bool connectionBusy = false;
    bool cableForgetOnly = false, localForgetForPairing = false;

    lv_obj_t *ui_Widget = nullptr;
    void setWirelessFlow(WirelessFlow flow, const char *message = nullptr);
    void beginCablePairing(bool forgetOnly = false);
    void attemptCablePairing(bool resetExisting);
    void beginLocalForget(const CablePairingInfo &pairing, bool continuePairing);
    void beginTargetScan(const CablePairingInfo &pairing);
    lv_obj_t *ui_LoadingOverlay = nullptr;
    lv_obj_t *ui_LoadingLabel = nullptr;
    lv_obj_t *wirelessPanel = nullptr, *wirelessStatus = nullptr;
    lv_obj_t *wirelessPeers = nullptr, *wirelessPin = nullptr;
    lv_obj_t *wirelessKeyboardOverlay = nullptr, *wirelessKeyboard = nullptr;
    lv_timer_t *wirelessTimer = nullptr;
    bool wirelessPending = false;
    size_t displayedPeerCount = static_cast<size_t>(-1);
    void showWireless(bool remembered);
    void refreshWireless();
    void startWirelessConnection(bool remembered);
    void closeWireless();
    void showWirelessKeyboard();
    void hideWirelessKeyboard();
    void destroyWirelessKeyboard();

    void createOptionButton(const char *text, lv_coord_t x, lv_coord_t y, DefaultCommunicationMode mode, bool supported = true);
    void createWirelessManualButton(lv_coord_t x, lv_coord_t y);
    void createModeIcon(const void *imageSource, lv_coord_t centerX, lv_coord_t y, uint16_t zoom);
    void handleModeSelection(DefaultCommunicationMode mode);
    uint32_t showLoading(const char *message);
    void finishLoading(uint32_t startTick, bool keepVisible);
    void hideLoading();

public:
    explicit CommunicationSelectionGui(GuiRouter &router, DeviceManager &deviceManager);
    ~CommunicationSelectionGui() = default;

    void init(void);
    void applyDefaultCommunicationMode();
    void constructCommunicationSelection(void);
    void hideCommunicationSelection(void);
};

#endif // COMMUNICATION_SELECTION_GUI_HPP

#include "communication_selection_gui.hpp"

#include "../helpers.hpp"
#include "./images/ui_images.h"
#include "expt.hpp"
#include <cstring>
#include <string>

#ifndef LV_SYMBOL_SETTINGS
#define LV_SYMBOL_SETTINGS "⚙"
#endif

namespace
{
constexpr uint32_t MIN_CONNECT_LOADING_MS = 500;
}

CommunicationSelectionGui::CommunicationSelectionGui(GuiRouter &router, DeviceManager &deviceManager)
    : router(router), deviceManager(deviceManager)
{
}

void CommunicationSelectionGui::createOptionButton(const char *text, lv_coord_t x, lv_coord_t y, DefaultCommunicationMode mode, bool supported)
{
    lv_obj_t *button = lv_btn_create(ui_Widget);
    lv_obj_set_size(button, 220, 80);
    lv_obj_set_pos(button, x, y);
    if (!supported) {
        lv_obj_set_style_bg_color(button, lv_color_hex(0x8A8F98), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x8A8F98), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(button, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_obj_add_event_cb(button, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
        DefaultCommunicationMode mode = static_cast<DefaultCommunicationMode>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))));
        self->handleModeSelection(mode);
    }, LV_EVENT_ALL, this);
    lv_obj_set_user_data(button, reinterpret_cast<void *>(static_cast<intptr_t>(mode)));

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void CommunicationSelectionGui::createWirelessManualButton(lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *button = lv_btn_create(ui_Widget);
    lv_obj_set_size(button, 58, 80);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x033E70), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x044C86), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_user_data(button, reinterpret_cast<void *>(static_cast<intptr_t>(DefaultCommunicationMode::WIRELESS_MANUAL)));
    lv_obj_add_event_cb(button, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
        self->handleModeSelection(DefaultCommunicationMode::WIRELESS_MANUAL);
    }, LV_EVENT_ALL, this);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void CommunicationSelectionGui::createModeIcon(const void *imageSource, lv_coord_t centerX, lv_coord_t y, uint16_t zoom)
{
    lv_obj_t *holder = lv_obj_create(ui_Widget);
    lv_obj_remove_style_all(holder);
    lv_obj_set_size(holder, 92, 92);
    lv_obj_set_pos(holder, centerX - 46, y);
    lv_obj_clear_flag(holder, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *image = lv_img_create(holder);
    lv_img_set_src(image, imageSource);
    lv_img_set_zoom(image, zoom);
    lv_obj_center(image);
    lv_obj_clear_flag(image, LV_OBJ_FLAG_CLICKABLE);
}

uint32_t CommunicationSelectionGui::showLoading(const char *message)
{
    if (!ui_LoadingOverlay) {
        ui_LoadingOverlay = lv_obj_create(ui_Widget);
        lv_obj_remove_style_all(ui_LoadingOverlay);
        lv_obj_set_size(ui_LoadingOverlay, lv_pct(100), lv_pct(100));
        lv_obj_set_align(ui_LoadingOverlay, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_color(ui_LoadingOverlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_LoadingOverlay, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(ui_LoadingOverlay, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *panel = lv_obj_create(ui_LoadingOverlay);
        lv_obj_set_size(panel, 240, 120);
        lv_obj_center(panel);
        lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *spinner = lv_spinner_create(panel, 900, 60);
        lv_obj_set_size(spinner, 38, 38);
        lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 16);

        ui_LoadingLabel = lv_label_create(panel);
        lv_label_set_text(ui_LoadingLabel, message ? message : "Connecting...");
        lv_obj_align(ui_LoadingLabel, LV_ALIGN_BOTTOM_MID, 0, -18);
        lv_obj_set_style_text_align(ui_LoadingLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_label_set_text(ui_LoadingLabel, message ? message : "Connecting...");
    }

    connectionBusy = true;
    lv_obj_clear_flag(ui_LoadingOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_LoadingOverlay);
    lv_obj_invalidate(ui_LoadingOverlay);
    const uint32_t startTick = lv_tick_get();
    lv_timer_handler();
    lv_refr_now(nullptr);
    delay_ms(20);
    lv_timer_handler();
    lv_refr_now(nullptr);
    return startTick;
}

void CommunicationSelectionGui::finishLoading(uint32_t startTick, bool keepVisible)
{
    while (lv_tick_elaps(startTick) < MIN_CONNECT_LOADING_MS) {
        lv_timer_handler();
        delay_ms(16);
    }

    if (!keepVisible) {
        hideLoading();
    }
}

void CommunicationSelectionGui::hideLoading()
{
    connectionBusy = false;
    if (ui_LoadingOverlay) {
        lv_obj_add_flag(ui_LoadingOverlay, LV_OBJ_FLAG_HIDDEN);
    }
}

void CommunicationSelectionGui::handleModeSelection(DefaultCommunicationMode mode)
{
    if (connectionBusy) {
        return;
    }

    if (mode != DefaultCommunicationMode::CABLE) {
        showWireless(mode == DefaultCommunicationMode::WIRELESS_AUTO);
        return;
    }

    deviceManager.endProtocolSession();
    if (auto* link = deviceManager.getProtocolLinkControl()) link->selectCable();
    const uint32_t loadingStart = showLoading("Connecting...");
    if (!deviceManager.initializeProtocolConnection()) {
        finishLoading(loadingStart, false);
        splashMessage("Unable to establish connection. Check cable connection and emulator.");
        return;
    }

    finishLoading(loadingStart, true);
    router.completeCommunicationSelection(mode);
}

void CommunicationSelectionGui::applyDefaultCommunicationMode()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    const DefaultCommunicationMode mode = router.consumeCommunicationAutoStartMode();
    if (mode == DefaultCommunicationMode::ASK) {
        return;
    }

    lv_timer_handler();
    lv_refr_now(nullptr);
    handleModeSelection(mode);
}

void CommunicationSelectionGui::init(void)
{
    if (initialized) {
        return;
    }

    constructCommunicationSelection();
    initialized = true;
}

void CommunicationSelectionGui::constructCommunicationSelection(void)
{
    ui_Widget = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_Widget);
    lv_obj_set_width(ui_Widget, 760);
    lv_obj_set_height(ui_Widget, 440);
    lv_obj_set_align(ui_Widget, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Widget, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0x055DA9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Widget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Widget, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(ui_Widget);
    lv_label_set_text(title, "Communication");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *subtitle = lv_label_create(ui_Widget);
    lv_label_set_text(subtitle, "Choose how the target platform should be reached");
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 75);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    createOptionButton("Cable (UART)", 70, 130, DefaultCommunicationMode::CABLE);
    createOptionButton("Wireless (BLE)", 395, 130, DefaultCommunicationMode::WIRELESS_AUTO);
    createWirelessManualButton(625, 130);
    createModeIcon(&ui_img_cable_png, 180, 226, 150);
    createModeIcon(&ui_img_bluetooth_png, 539, 232, 150);

    lv_obj_t *back = lv_btn_create(ui_Widget);
    lv_obj_set_size(back, 90, 36);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 16, -14);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            self->router.showMainMenu();
        }
    }, LV_EVENT_ALL, this);
    lv_obj_t *backLabel = lv_label_create(back);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);

    lv_obj_t *hint = lv_label_create(ui_Widget);
    lv_label_set_text(hint, "For cable connection (UART), please check if cable is in place.");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void CommunicationSelectionGui::hideCommunicationSelection(void)
{
    if (!initialized) {
        return;
    }

    if (wirelessTimer) { lv_timer_del(wirelessTimer); wirelessTimer = nullptr; }
    destroyWirelessKeyboard();
    wirelessPanel = wirelessStatus = wirelessPeers = wirelessPin = nullptr;
    wirelessInstruction = wirelessSpinner = nullptr;
    wirelessConnect = wirelessScan = wirelessForget = nullptr;
    wirelessAction = wirelessContinue = wirelessBack = nullptr;
    wirelessPending = false;
    if (ui_Widget) {
        lv_obj_del(ui_Widget);
    }

    ui_Widget = nullptr;
    ui_LoadingOverlay = nullptr;
    ui_LoadingLabel = nullptr;
    connectionBusy = false;
    initialized = false;
}

void CommunicationSelectionGui::setWirelessFlow(WirelessFlow flow, const char *message)
{
    wirelessFlow = flow;
    const bool manual = flow == WirelessFlow::Manual;
    const bool working = flow == WirelessFlow::CablePairing ||
        flow == WirelessFlow::LocalForgetting || flow == WirelessFlow::TargetScanning ||
        flow == WirelessFlow::Connecting;
    const bool action = flow == WirelessFlow::AlreadyPaired ||
        flow == WirelessFlow::Forgotten || flow == WirelessFlow::Error;
    auto show = [](lv_obj_t *object, bool visible) {
        if (!object) return;
        if (visible) lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    };
    show(wirelessStatus, manual); show(wirelessPeers, manual); show(wirelessPin, manual);
    show(wirelessConnect, manual); show(wirelessScan, manual); show(wirelessForget, manual);
    show(wirelessInstruction, !manual); show(wirelessSpinner, working);
    show(wirelessAction, action); show(wirelessContinue, flow == WirelessFlow::Success);
    show(wirelessBack, flow != WirelessFlow::LocalForgetting);
    if (message) {
        if (manual && wirelessStatus) lv_label_set_text(wirelessStatus, message);
        else if (wirelessInstruction) lv_label_set_text(wirelessInstruction, message);
    }
    if (wirelessAction) {
        auto *label = lv_obj_get_child(wirelessAction, 0);
        const char *text = flow == WirelessFlow::AlreadyPaired ? "Forget Board & replace pairing" :
            flow == WirelessFlow::Forgotten ? "Back to BLE Settings" : "Retry";
        lv_label_set_text(label, text);
    }
}

void CommunicationSelectionGui::attemptCablePairing(bool resetExisting)
{
    auto *link = deviceManager.getProtocolLinkControl();
    if (!link || wirelessFlow != WirelessFlow::CablePairing) return;
    lastPairAttempt = lv_tick_get();
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "BLE.GUI", "UART PAIR",
        "sending reset=%d elapsedMs=%lu", resetExisting,
        static_cast<unsigned long>(lv_tick_elaps(wirelessFlowStarted)));
    const auto pairing = link->requestCablePairing(resetExisting);
    if (pairing.status == CablePairingStatus::Ok) {
        debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "BLE.GUI", "UART PAIR",
            "accepted reset=%d boardId=%s", resetExisting, pairing.boardId);
        if (resetExisting) beginLocalForget(pairing, !cableForgetOnly);
        else beginTargetScan(pairing);
        return;
    }
    if (pairing.status == CablePairingStatus::AlreadyPaired) {
        setWirelessFlow(WirelessFlow::AlreadyPaired,
            "This Board is already paired.\nReplace its existing pairing?");
        return;
    }
    debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "BLE.GUI", "UART PAIR",
        "failed reset=%d error=%s", resetExisting, pairing.error[0] ? pairing.error : "-");
    if (pairing.error[0] && std::strcmp(pairing.error, "Response timeout") != 0) {
        setWirelessFlow(WirelessFlow::Error, pairing.error);
    }
}

void CommunicationSelectionGui::beginLocalForget(const CablePairingInfo &pairing, bool continuePairing)
{
    auto *link = deviceManager.getProtocolLinkControl();
    targetBoardId = pairing.boardId;
    targetPin = pairing.pin;
    localForgetForPairing = continuePairing;
    wirelessFlowStarted = lv_tick_get();
    setWirelessFlow(WirelessFlow::LocalForgetting,
        continuePairing ? "Board bond removed.\nRemoving the old pairing from Panel..." :
                          "Board bond removed.\nRemoving the pairing from Panel...");
    link->forgetWireless();
}

void CommunicationSelectionGui::beginCablePairing(bool forgetOnly)
{
    auto *link = deviceManager.getProtocolLinkControl();
    if (!link) return;
    hideWirelessKeyboard();
    deviceManager.endProtocolSession();
    link->selectCable();
    cableForgetOnly = forgetOnly;
    localForgetForPairing = false;
    targetBoardId.clear(); targetPin = 0;
    wirelessFlowStarted = lv_tick_get();
    lastPairAttempt = wirelessFlowStarted;
    setWirelessFlow(WirelessFlow::CablePairing,
        forgetOnly ? "Connect this Board to the Panel with the UART cable.\n"
                     "Removing pairing from both devices... (5 s)" :
                     "Connect the Board to the Panel with the UART cable.\n"
                     "Reading Board ID and PIN... (5 s)");
    lv_refr_now(nullptr);
    attemptCablePairing(forgetOnly); // First request is immediate; timer only retries.
}

void CommunicationSelectionGui::beginTargetScan(const CablePairingInfo &pairing)
{
    auto *link = deviceManager.getProtocolLinkControl();
    targetBoardId = pairing.boardId; targetPin = pairing.pin;
    displayedPeerCount = static_cast<size_t>(-1);
    const std::string message = "Board " + targetBoardId +
        " found.\nScanning for its BLE signal...";
    setWirelessFlow(WirelessFlow::TargetScanning, message.c_str());
    link->scanWireless();
}

void CommunicationSelectionGui::showWireless(bool remembered)
{
    auto *link = deviceManager.getProtocolLinkControl();
    if (!link) { splashMessage("BLE transport unavailable in this build."); return; }
    deviceManager.endProtocolSession();
    wirelessCompletionMode = remembered ? DefaultCommunicationMode::WIRELESS_AUTO
                                        : DefaultCommunicationMode::WIRELESS_MANUAL;
    if (!wirelessPanel) {
        wirelessPanel = lv_obj_create(ui_Widget);
        lv_obj_set_size(wirelessPanel, 720, 410); lv_obj_center(wirelessPanel);
        lv_obj_set_style_bg_color(wirelessPanel, lv_color_hex(0xFFFFFF), 0);
        auto *title = lv_label_create(wirelessPanel);
        lv_label_set_text(title, "EduBox Board - Bluetooth LE");
        lv_obj_set_pos(title, 12, 4); lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        wirelessStatus = lv_label_create(wirelessPanel);
        lv_obj_set_width(wirelessStatus, 670); lv_obj_set_pos(wirelessStatus, 12, 38);
        wirelessInstruction = lv_label_create(wirelessPanel);
        lv_obj_set_width(wirelessInstruction, 620);
        lv_obj_set_style_text_align(wirelessInstruction, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(wirelessInstruction, LV_ALIGN_CENTER, 0, -32);
        wirelessSpinner = lv_spinner_create(wirelessPanel, 900, 60);
        lv_obj_set_size(wirelessSpinner, 46, 46);
        lv_obj_align(wirelessSpinner, LV_ALIGN_CENTER, 0, 52);
        wirelessPeers = lv_dropdown_create(wirelessPanel);
        lv_obj_set_size(wirelessPeers, 520, 44); lv_obj_set_pos(wirelessPeers, 12, 88);
        lv_dropdown_set_options(wirelessPeers, "Scan for Boards...");
        wirelessPin = lv_textarea_create(wirelessPanel);
        lv_obj_set_size(wirelessPin, 180, 44); lv_obj_set_pos(wirelessPin, 12, 148);
        lv_textarea_set_one_line(wirelessPin, true); lv_textarea_set_max_length(wirelessPin, 6);
        lv_textarea_set_accepted_chars(wirelessPin, "0123456789");
        lv_textarea_set_placeholder_text(wirelessPin, "Board PIN");
        lv_textarea_set_password_mode(wirelessPin, true);
        lv_obj_add_event_cb(wirelessPin, [](lv_event_t *e) {
            const auto code = lv_event_get_code(e);
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) self->showWirelessKeyboard();
            else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) self->hideWirelessKeyboard();
        }, LV_EVENT_ALL, this);
        auto addButton = [this](const char *text, int x, int y, int width, int height,
                                uint32_t color, lv_event_cb_t callback) {
            auto *button = lv_btn_create(wirelessPanel);
            lv_obj_set_size(button, width, height); lv_obj_set_pos(button, x, y);
            lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
            lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, this);
            auto *label = lv_label_create(button);
            lv_label_set_text(label, text); lv_obj_center(label);
            return button;
        };
        wirelessConnect = addButton("Connect", 12, 214, 180, 52, 0x2E9D55, [](lv_event_t *e) {
            static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e))->startWirelessConnection(false);
        });
        wirelessScan = addButton("Scan", 550, 92, 120, 38, 0x1677C8, [](lv_event_t *e) {
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            self->hideWirelessKeyboard(); self->displayedPeerCount = static_cast<size_t>(-1);
            self->deviceManager.getProtocolLinkControl()->scanWireless();
        });
        wirelessForget = addButton("Forget pairing", 12, 282, 220, 52, 0xC73535, [](lv_event_t *e) {
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            self->hideWirelessKeyboard(); self->wirelessPending = false;
            lv_textarea_set_text(self->wirelessPin, "");
            self->beginCablePairing(true);
        });
        wirelessAction = addButton("Retry", 225, 268, 270, 50, 0xC73535, [](lv_event_t *e) {
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            auto *link = self->deviceManager.getProtocolLinkControl();
            if (self->wirelessFlow == WirelessFlow::Forgotten) {
                self->cableForgetOnly = false;
                self->setWirelessFlow(WirelessFlow::Manual, "Pairing removed from Board and Panel.");
                self->displayedPeerCount = static_cast<size_t>(-1);
                link->scanWireless();
                return;
            }
            if (self->wirelessFlow == WirelessFlow::AlreadyPaired) {
                self->cableForgetOnly = false;
                self->wirelessFlowStarted = lv_tick_get();
                self->setWirelessFlow(WirelessFlow::CablePairing, "Resetting existing Board pairing...");
                self->attemptCablePairing(true);
                return;
            }
            if (self->localForgetForPairing) {
                CablePairingInfo pairing;
                pairing.status = CablePairingStatus::Ok;
                pairing.pin = self->targetPin;
                std::snprintf(pairing.boardId, sizeof(pairing.boardId), "%s",
                    self->targetBoardId.c_str());
                self->beginLocalForget(pairing, true);
                return;
            }
            if (self->cableForgetOnly) {
                self->beginCablePairing(true);
                return;
            }
            if (self->wirelessCompletionMode == DefaultCommunicationMode::WIRELESS_MANUAL) {
                self->setWirelessFlow(WirelessFlow::Manual);
                self->displayedPeerCount = static_cast<size_t>(-1);
                link->scanWireless();
                return;
            }
            const auto info = link->info();
            if (self->wirelessCompletionMode == DefaultCommunicationMode::WIRELESS_AUTO &&
                info.rememberedAddress[0]) {
                self->setWirelessFlow(WirelessFlow::Connecting, "Connecting to remembered Board...");
                self->wirelessPending = link->connectRemembered();
                if (!self->wirelessPending)
                    self->setWirelessFlow(WirelessFlow::Error, "Remembered Board is unavailable.");
            } else self->beginCablePairing();
        });
        wirelessContinue = addButton("Continue", 260, 270, 200, 52, 0x2E9D55, [](lv_event_t *e) {
            auto *self = static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e));
            self->router.completeCommunicationSelection(self->wirelessCompletionMode);
        });
        wirelessBack = addButton("Back / Cancel", 12, 350, 145, 40, 0x59636E, [](lv_event_t *e) {
            static_cast<CommunicationSelectionGui *>(lv_event_get_user_data(e))->closeWireless();
        });
        wirelessTimer = lv_timer_create([](lv_timer_t *timer) {
            static_cast<CommunicationSelectionGui *>(timer->user_data)->refreshWireless();
        }, 200, this);
    }
    lv_obj_clear_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN);
    hideWirelessKeyboard(); wirelessPending = false;
    const auto info = link->info();
    if (!remembered) {
        setWirelessFlow(WirelessFlow::Manual);
        displayedPeerCount = static_cast<size_t>(-1); link->scanWireless();
    } else if (info.rememberedAddress[0]) {
        const std::string message = std::string("Connecting to ") +
            (info.rememberedBoardId[0] ? info.rememberedBoardId : info.rememberedAddress) + "...";
        setWirelessFlow(WirelessFlow::Connecting, message.c_str());
        wirelessPending = link->connectRemembered();
        if (!wirelessPending) setWirelessFlow(WirelessFlow::Error, "Remembered Board is unavailable.");
    } else beginCablePairing();
}

void CommunicationSelectionGui::startWirelessConnection(bool remembered)
{
    auto *link = deviceManager.getProtocolLinkControl();
    if (!link || wirelessPending) return;
    if (remembered) wirelessPending = link->connectRemembered();
    else {
        const char *pin = lv_textarea_get_text(wirelessPin);
        if (std::strlen(pin) != 6) {
            splashMessage("Enter the six-digit PIN printed on the Board label."); return;
        }
        uint32_t value = 0;
        for (size_t i = 0; i < 6; ++i) value = value * 10 + uint32_t(pin[i] - '0');
        wirelessPending = link->connectWireless(lv_dropdown_get_selected(wirelessPeers), value);
    }
    if (!wirelessPending) { splashMessage("Unable to start BLE connection."); return; }
    lv_textarea_set_text(wirelessPin, ""); hideWirelessKeyboard();
    setWirelessFlow(WirelessFlow::Connecting, "Pairing and securing the BLE connection...");
}

void CommunicationSelectionGui::refreshWireless()
{
    if (!wirelessPanel || lv_obj_has_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN)) return;
    auto *link = deviceManager.getProtocolLinkControl();
    if (!link) return;
    const auto info = link->info();
    if (wirelessFlow == WirelessFlow::CablePairing) {
        if (lv_tick_elaps(wirelessFlowStarted) >= 5000) {
            setWirelessFlow(WirelessFlow::Error, cableForgetOnly ?
                "Unable to remove pairing in 5 seconds.\nCheck the UART cable and retry." :
                "No PAIR response in 5 seconds.\nCheck the UART cable and retry.");
            return;
        }
        if (lv_tick_elaps(lastPairAttempt) >= 500) attemptCablePairing(cableForgetOnly);
        return;
    }
    if (wirelessFlow == WirelessFlow::LocalForgetting) {
        if (info.state == ProtocolLinkState::Error) {
            setWirelessFlow(WirelessFlow::Error,
                info.error[0] ? info.error : "Unable to remove pairing from Panel.");
            return;
        }
        if (info.state != ProtocolLinkState::Idle || info.rememberedAddress[0]) return;
        if (localForgetForPairing) {
            CablePairingInfo pairing;
            pairing.status = CablePairingStatus::Ok;
            pairing.pin = targetPin;
            std::snprintf(pairing.boardId, sizeof(pairing.boardId), "%s", targetBoardId.c_str());
            beginTargetScan(pairing);
        } else {
            setWirelessFlow(WirelessFlow::Forgotten,
                "Pairing removed from Board and Panel.\nYou can now pair either device again.");
        }
        return;
    }
    if (wirelessFlow == WirelessFlow::TargetScanning) {
        if (info.state == ProtocolLinkState::Scanning) return;
        if (info.state == ProtocolLinkState::Error) {
            setWirelessFlow(WirelessFlow::Error, info.error); return;
        }
        for (size_t i = 0; i < info.count; ++i) {
            if (targetBoardId == info.peers[i].name) {
                wirelessPending = link->connectWireless(i, targetPin); targetPin = 0;
                if (wirelessPending) {
                    const std::string message = "Pairing with " + targetBoardId + "...";
                    setWirelessFlow(WirelessFlow::Connecting, message.c_str());
                } else setWirelessFlow(WirelessFlow::Error, "BLE connection could not be started.");
                return;
            }
        }
        const std::string message = "Board " + targetBoardId +
            " was not found over BLE.\nMove it closer and retry.";
        setWirelessFlow(WirelessFlow::Error, message.c_str()); return;
    }
    if (wirelessFlow == WirelessFlow::Manual) {
        const char *states[] = {"Off", "Ready to scan", "Scanning...", "Connecting...",
            "Securing...", "Connected", "Retrying...", "Error", "Forgetting..."};
        lv_label_set_text_fmt(wirelessStatus, "%s   %s",
            states[static_cast<unsigned>(info.state)], info.error);
        if (info.state != ProtocolLinkState::Scanning && displayedPeerCount != info.count) {
            std::string options;
            for (size_t i = 0; i < info.count; ++i) {
                if (i) options += "\n";
                options += std::string(info.peers[i].name) + "  (" +
                    std::to_string(info.peers[i].rssi) + " dBm)";
            }
            lv_dropdown_set_options(wirelessPeers,
                options.empty() ? "No Board found - tap Scan" : options.c_str());
            displayedPeerCount = info.count;
        }
        return;
    }
    if (wirelessFlow != WirelessFlow::Connecting) return;
    if (info.state == ProtocolLinkState::Error) {
        wirelessPending = false;
        setWirelessFlow(WirelessFlow::Error, info.error[0] ? info.error : "BLE connection failed.");
        return;
    }
    if (info.state != ProtocolLinkState::Ready) return;
    wirelessPending = false;
    lv_label_set_text(wirelessInstruction, "BLE connected. Initializing protocol...");
    lv_refr_now(nullptr);
    if (!deviceManager.initializeProtocolConnection()) {
        setWirelessFlow(WirelessFlow::Error, "BLE connected, but protocol initialization failed."); return;
    }
    const auto ready = link->info();
    const char *board = ready.rememberedBoardId[0] ? ready.rememberedBoardId :
        (targetBoardId.empty() ? "Board" : targetBoardId.c_str());
    const std::string message = std::string(LV_SYMBOL_OK) + "  Success!\n" + board + " is ready.";
    setWirelessFlow(WirelessFlow::Success, message.c_str());
}

void CommunicationSelectionGui::closeWireless()
{
    hideWirelessKeyboard(); wirelessPending = false; targetPin = 0;
    if (auto *link = deviceManager.getProtocolLinkControl()) link->stopWireless();
    if (wirelessPanel) lv_obj_add_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN);
}

void CommunicationSelectionGui::showWirelessKeyboard()
{
    if (!wirelessPin) return;

    if (!wirelessKeyboardOverlay) {
        wirelessKeyboardOverlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_style_all(wirelessKeyboardOverlay);
        lv_obj_set_size(wirelessKeyboardOverlay, lv_pct(100), lv_pct(100));
        lv_obj_align(wirelessKeyboardOverlay, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(wirelessKeyboardOverlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(wirelessKeyboardOverlay, 110, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(wirelessKeyboardOverlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(wirelessKeyboardOverlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(wirelessKeyboardOverlay, [](lv_event_t* e) {
            if (lv_event_get_target(e) != lv_event_get_current_target(e)) return;
            static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e))->hideWirelessKeyboard();
        }, LV_EVENT_CLICKED, this);

        wirelessKeyboard = lv_keyboard_create(wirelessKeyboardOverlay);
        lv_obj_set_size(wirelessKeyboard, 520, 180);
        lv_obj_align(wirelessKeyboard, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_keyboard_set_mode(wirelessKeyboard, LV_KEYBOARD_MODE_NUMBER);
        lv_obj_add_event_cb(wirelessKeyboard, [](lv_event_t* e) {
            const auto code = lv_event_get_code(e);
            if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
                static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e))->hideWirelessKeyboard();
            }
        }, LV_EVENT_ALL, this);
        debugLogMessage(DEBUG_VERBOSE_ALL, "BLE.GUI", "PIN keypad", "floating numeric keypad created");
    }

    lv_keyboard_set_textarea(wirelessKeyboard, wirelessPin);
    lv_obj_clear_flag(wirelessKeyboardOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(wirelessKeyboardOverlay);
    debugLogMessage(DEBUG_VERBOSE_ALL, "BLE.GUI", "PIN keypad", "shown");
}

void CommunicationSelectionGui::hideWirelessKeyboard()
{
    if (!wirelessKeyboardOverlay) return;
    if (wirelessKeyboard) lv_keyboard_set_textarea(wirelessKeyboard, nullptr);
    lv_obj_add_flag(wirelessKeyboardOverlay, LV_OBJ_FLAG_HIDDEN);
    debugLogMessage(DEBUG_VERBOSE_ALL, "BLE.GUI", "PIN keypad", "hidden");
}

void CommunicationSelectionGui::destroyWirelessKeyboard()
{
    if (!wirelessKeyboardOverlay) return;
    if (wirelessKeyboard) lv_keyboard_set_textarea(wirelessKeyboard, nullptr);
    lv_obj_del(wirelessKeyboardOverlay);
    wirelessKeyboardOverlay = nullptr;
    wirelessKeyboard = nullptr;
}

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
    wirelessPanel = wirelessStatus = wirelessPeers = wirelessPin = nullptr;
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

void CommunicationSelectionGui::showWireless(bool remembered)
{
    auto* link = deviceManager.getProtocolLinkControl();
    if (!link) { splashMessage("BLE transport unavailable in this build."); return; }
    deviceManager.endProtocolSession();
    if (!wirelessPanel) {
        wirelessPanel = lv_obj_create(ui_Widget);
        lv_obj_set_size(wirelessPanel, 720, 410);
        lv_obj_center(wirelessPanel);
        lv_obj_set_style_bg_color(wirelessPanel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_t* title = lv_label_create(wirelessPanel);
        lv_label_set_text(title, "EduBox Board - Bluetooth LE");
        lv_obj_set_pos(title, 12, 4);
        wirelessStatus = lv_label_create(wirelessPanel);
        lv_obj_set_width(wirelessStatus, 670); lv_obj_set_pos(wirelessStatus, 12, 34);
        wirelessPeers = lv_dropdown_create(wirelessPanel);
        lv_obj_set_size(wirelessPeers, 670, 42); lv_obj_set_pos(wirelessPeers, 12, 78);
        lv_dropdown_set_options(wirelessPeers, "Scan for Boards...");
        wirelessPin = lv_textarea_create(wirelessPanel);
        lv_obj_set_size(wirelessPin, 165, 44); lv_obj_set_pos(wirelessPin, 12, 134);
        lv_textarea_set_one_line(wirelessPin, true); lv_textarea_set_max_length(wirelessPin, 6);
        lv_textarea_set_accepted_chars(wirelessPin, "0123456789");
        lv_textarea_set_placeholder_text(wirelessPin, "6-digit Board PIN");
        lv_textarea_set_password_mode(wirelessPin, true);
        lv_obj_t* keyboard = lv_keyboard_create(wirelessPanel);
        lv_obj_set_size(keyboard, 485, 145); lv_obj_set_pos(keyboard, 196, 132);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
        lv_keyboard_set_textarea(keyboard, wirelessPin);
        auto addButton = [this](const char* text, int x, int y, lv_event_cb_t callback) {
            auto* button = lv_btn_create(wirelessPanel);
            lv_obj_set_size(button, 145, 40); lv_obj_set_pos(button, x, y);
            lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, this);
            auto* label = lv_label_create(button); lv_label_set_text(label, text); lv_obj_center(label);
        };
        addButton("Connect selected", 12, 294, [](lv_event_t* e) {
            static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e))->startWirelessConnection(false);
        });
        addButton("Scan", 182, 294, [](lv_event_t* e) {
            auto* self = static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e));
            self->wirelessPending = false; self->displayedPeerCount = static_cast<size_t>(-1);
            self->deviceManager.getProtocolLinkControl()->scanWireless();
        });
        addButton("Remembered", 352, 294, [](lv_event_t* e) {
            static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e))->startWirelessConnection(true);
        });
        addButton("Forget peer", 522, 294, [](lv_event_t* e) {
            auto* self = static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e));
            self->wirelessPending = false;
            self->deviceManager.getProtocolLinkControl()->forgetWireless();
            lv_textarea_set_text(self->wirelessPin, "");
        });
        addButton("Back / cancel", 12, 348, [](lv_event_t* e) {
            static_cast<CommunicationSelectionGui*>(lv_event_get_user_data(e))->closeWireless();
        });
        auto* hint = lv_label_create(wirelessPanel);
        lv_label_set_text(hint, "First pairing: read PIN on Board console.\nBoard BOOT held 3 s after boot resets bonding.");
        lv_obj_set_pos(hint, 182, 350); lv_obj_set_width(hint, 490);
        wirelessTimer = lv_timer_create([](lv_timer_t* timer) {
            static_cast<CommunicationSelectionGui*>(timer->user_data)->refreshWireless();
        }, 200, this);
    }
    lv_obj_clear_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN);
    if (remembered && link->info().rememberedAddress[0]) startWirelessConnection(true);
    else { displayedPeerCount = static_cast<size_t>(-1); link->scanWireless(); }
}

void CommunicationSelectionGui::startWirelessConnection(bool remembered)
{
    auto* link = deviceManager.getProtocolLinkControl();
    if (!link || wirelessPending) return;
    const auto info = link->info();
    if (info.state == ProtocolLinkState::Scanning || info.state == ProtocolLinkState::Connecting ||
        info.state == ProtocolLinkState::Securing) return;
    if (remembered) {
        if (!link->connectRemembered()) { splashMessage("No bonded Board remembered. Scan and pair first."); return; }
    } else {
        const char* pin = lv_textarea_get_text(wirelessPin);
        if (std::strlen(pin) != 6) { splashMessage("Enter the six-digit PIN from the Board console."); return; }
        uint32_t value = 0;
        for (size_t i = 0; i < 6; ++i) value = value * 10 + uint32_t(pin[i] - '0');
        if (!link->connectWireless(lv_dropdown_get_selected(wirelessPeers), value)) return;
    }
    lv_textarea_set_text(wirelessPin, ""); // PIN is never saved.
    wirelessPending = true;
}

void CommunicationSelectionGui::refreshWireless()
{
    if (!wirelessPanel || lv_obj_has_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN)) return;
    auto* link = deviceManager.getProtocolLinkControl();
    const auto info = link->info();
    const char* states[] = {"Off", "Idle", "Scanning...", "Connecting...", "Securing...", "Ready", "Retrying...", "Error"};
    lv_label_set_text_fmt(wirelessStatus, "%s  MTU %u  %s\nRemembered: %s",
        states[static_cast<unsigned>(info.state)], unsigned(info.mtu), info.error, info.rememberedAddress);
    if (info.state != ProtocolLinkState::Scanning && displayedPeerCount != info.count) {
        std::string options;
        for (size_t i = 0; i < info.count; ++i) {
            if (i) options += "\n";
            options += std::string(info.peers[i].name) + "  " + info.peers[i].address +
                "  (" + std::to_string(info.peers[i].rssi) + " dBm)";
        }
        lv_dropdown_set_options(wirelessPeers, options.empty() ? "No Board — try Scan" : options.c_str());
        displayedPeerCount = info.count;
    }
    if (!wirelessPending) return;
    if (info.state == ProtocolLinkState::Error) { wirelessPending = false; return; }
    if (info.state != ProtocolLinkState::Ready) return;
    wirelessPending = false;
    // Still in the main GUI owner context, never in a BLE callback.
    lv_label_set_text(wirelessStatus, "BLE ready. Initializing VSCP...");
    lv_refr_now(nullptr);
    if (!deviceManager.initializeProtocolConnection()) {
        lv_label_set_text(wirelessStatus, "VSCP INIT failed: Board busy / firmware mismatch. Retry Connect.");
        return;
    }
    router.completeCommunicationSelection(DefaultCommunicationMode::WIRELESS_MANUAL);
    // Navigation deletes this panel/timer: do not touch any GUI object hereafter.
}

void CommunicationSelectionGui::closeWireless()
{
    wirelessPending = false;
    deviceManager.getProtocolLinkControl()->stopWireless();
    lv_obj_add_flag(wirelessPanel, LV_OBJ_FLAG_HIDDEN);
}

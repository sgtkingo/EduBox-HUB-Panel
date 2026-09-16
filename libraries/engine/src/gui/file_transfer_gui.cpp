#include "file_transfer_gui.hpp"

#include "./images/ui_images.h"
#include "expt.hpp"

#include <cstring>

#if defined(ARDUINO)
    #include <Arduino.h>
#else
    #include <cstdlib>
#endif

FileTransferGui::FileTransferGui(GuiRouter &router) : router(router)
{
}

FileTransferGui::~FileTransferGui()
{
    closeRestartPrompt();
    transferService.stop();
}

void FileTransferGui::build()
{
    ui_Widget = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_Widget);
    lv_obj_set_size(ui_Widget, 760, 440);
    lv_obj_set_align(ui_Widget, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Widget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_Widget, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Widget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Widget, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Title = lv_label_create(ui_Widget);
    lv_label_set_text(ui_Title, "Transfer Mode");
    lv_obj_align(ui_Title, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_text_font(ui_Title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SubCaption = lv_label_create(ui_Widget);
    lv_label_set_text(ui_SubCaption, "Idle");
    lv_obj_set_width(ui_SubCaption, 650);
    lv_obj_align(ui_SubCaption, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_text_font(ui_SubCaption, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_SubCaption, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BackButton = lv_btn_create(ui_Widget);
    lv_obj_set_size(ui_BackButton, 96, 38);
    lv_obj_align(ui_BackButton, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_obj_set_style_radius(ui_BackButton, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_BackButton, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<FileTransferGui *>(lv_event_get_user_data(e));
        self->router.showMainMenu();
    }, LV_EVENT_ALL, this);

    lv_obj_t *backLabel = lv_label_create(ui_BackButton);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);
    lv_obj_set_style_text_font(backLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CableImage = lv_img_create(ui_Widget);
    lv_img_set_src(ui_CableImage, &ui_img_cable_png);
    lv_img_set_zoom(ui_CableImage, 230);
    lv_obj_align(ui_CableImage, LV_ALIGN_CENTER, 0, -28);

    ui_StateSymbol = lv_label_create(ui_Widget);
    lv_label_set_text(ui_StateSymbol, "");
    lv_obj_align_to(ui_StateSymbol, ui_CableImage, LV_ALIGN_CENTER, 58, -42);
    lv_obj_set_style_text_font(ui_StateSymbol, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StatusLabel = lv_label_create(ui_Widget);
    lv_obj_set_width(ui_StatusLabel, 650);
    lv_label_set_long_mode(ui_StatusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui_StatusLabel, LV_ALIGN_TOP_MID, 0, 278);
    lv_obj_set_style_text_font(ui_StatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_StatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HintLabel = lv_label_create(ui_Widget);
    lv_obj_set_width(ui_HintLabel, 650);
    lv_label_set_long_mode(ui_HintLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui_HintLabel, LV_ALIGN_BOTTOM_MID, 0, -92);
    lv_obj_set_style_text_font(ui_HintLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_HintLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_HintLabel, lv_color_hex(0x5F6B7A), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StartButton = lv_btn_create(ui_Widget);
    lv_obj_set_size(ui_StartButton, 190, 46);
    lv_obj_align(ui_StartButton, LV_ALIGN_BOTTOM_MID, -110, -38);
    lv_obj_set_style_radius(ui_StartButton, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_StartButton, lv_color_hex(0x2EAD5F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_StartButton, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<FileTransferGui *>(lv_event_get_user_data(e));
        self->handleStart();
    }, LV_EVENT_ALL, this);

    ui_StartButtonLabel = lv_label_create(ui_StartButton);
    lv_label_set_text(ui_StartButtonLabel, "OK");
    lv_obj_center(ui_StartButtonLabel);
    lv_obj_set_style_text_font(ui_StartButtonLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StopButton = lv_btn_create(ui_Widget);
    lv_obj_set_size(ui_StopButton, 190, 46);
    lv_obj_align(ui_StopButton, LV_ALIGN_BOTTOM_MID, 110, -38);
    lv_obj_set_style_radius(ui_StopButton, 7, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_StopButton, lv_color_hex(0xD96464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_StopButton, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<FileTransferGui *>(lv_event_get_user_data(e));
        self->handleStop();
    }, LV_EVENT_ALL, this);

    ui_StopButtonLabel = lv_label_create(ui_StopButton);
    lv_label_set_text(ui_StopButtonLabel, "End session");
    lv_obj_center(ui_StopButtonLabel);
    lv_obj_set_style_text_font(ui_StopButtonLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    renderPreparation();
}

void FileTransferGui::refresh()
{
    if (!ui_Widget || !ui_SubCaption || !ui_StatusLabel || !ui_HintLabel || !ui_StartButton || !ui_StopButton) {
        return;
    }

    const FileTransferState state = transferService.getState();
    const lv_color_t darkText = lv_color_hex(0x1C1F23);
    const lv_color_t whiteText = lv_color_hex(0xFFFFFF);
    const lv_color_t mutedText = lv_color_hex(0x5F6B7A);

    lv_obj_set_style_text_color(ui_SubCaption, darkText, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_StatusLabel, darkText, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (!transferSessionAttempted && !transferService.isTransferModeActive()) {
        renderPreparation();
        return;
    }

    // An unsupported USB backend needs no SD probe and its error must stay visible.
    const bool sdPresent = state == FileTransferState::UNSUPPORTED || transferService.isSdCardPresent();

    lv_obj_add_state(ui_BackButton, LV_STATE_DISABLED);
    lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_StopButton, LV_STATE_DISABLED);

    if (!sdPresent && state != FileTransferState::READY) {
        lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xD5D8DC), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_SubCaption, "SD card missing");
        lv_label_set_text(ui_StatusLabel, "Insert SD card to use Transfer Mode.");
        lv_label_set_text(ui_HintLabel, "No connection attempt was started. SPIFFS remains internal/debug storage.");
        lv_label_set_text(ui_StateSymbol, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(ui_StateSymbol, mutedText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
        return;
    }

    if (state == FileTransferState::CONNECTING) {
        renderConnecting();
        return;
    }

    if (state == FileTransferState::READY) {
        lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0x2EAD5F), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_SubCaption, "Transfer ready");
        lv_label_set_text(ui_StatusLabel, "Now, you can copy data direct from/to HMI using computer.");
        lv_label_set_text(ui_HintLabel, "Keep the USB cable connected. Stop Transfer before returning to normal HMI mode.");
        lv_label_set_text(ui_StateSymbol, LV_SYMBOL_LOOP);
        lv_obj_set_style_text_color(ui_SubCaption, whiteText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_StatusLabel, mutedText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_StateSymbol, whiteText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_StatusLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
        return;
    }

    if (state == FileTransferState::ERROR || state == FileTransferState::UNSUPPORTED) {
        lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_SubCaption, "Connection failed");
        lv_label_set_text(ui_StatusLabel, transferService.getLastMessage().c_str());
        lv_label_set_text(ui_HintLabel, "Check USB cable, SD card, and firmware USB MSC support.");
        lv_label_set_text(ui_StateSymbol, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(ui_SubCaption, whiteText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_StatusLabel, whiteText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_StateSymbol, whiteText, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
        return;
    }

    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xF2C94C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_SubCaption, "Connection...");
    lv_label_set_text(ui_StatusLabel, "Connect USB cable to computer.");
    lv_label_set_text(ui_HintLabel, "Transfer Mode will lock SD card access from HMI while PC transfer is active.");
    lv_label_set_text(ui_StateSymbol, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(ui_StateSymbol, darkText, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_StopButton, LV_STATE_DISABLED);
}

void FileTransferGui::renderPreparation()
{
    if (!ui_Widget || !ui_SubCaption || !ui_StatusLabel || !ui_HintLabel || !ui_StartButton || !ui_StopButton || !ui_BackButton) {
        return;
    }

    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_SubCaption, "Prepare transfer");
    lv_label_set_text(ui_StatusLabel,
                      "Transfer Mode exposes the SD card to your computer. It temporarily blocks HMI storage operations, requires an SD card, and expects the USB cable to be connected to the PC.");
    lv_label_set_text(ui_HintLabel, "Press OK to start the transfer session.");
    lv_label_set_text(ui_StateSymbol, LV_SYMBOL_USB);
    lv_obj_set_style_text_color(ui_SubCaption, lv_color_hex(0x1C1F23), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0x1C1F23), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_HintLabel, lv_color_hex(0x5F6B7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_StateSymbol, lv_color_hex(0x2B5DAA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_StartButtonLabel, "OK");
    lv_label_set_text(ui_StopButtonLabel, "End session");
    lv_obj_clear_state(ui_BackButton, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_StartButton, LV_STATE_DISABLED);
    lv_obj_add_state(ui_StopButton, LV_STATE_DISABLED);
}

void FileTransferGui::renderConnecting()
{
    if (!ui_Widget || !ui_SubCaption || !ui_StatusLabel || !ui_HintLabel) {
        return;
    }

    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xF2C94C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_SubCaption, "Connection...");
    lv_label_set_text(ui_StatusLabel, "Connecting...");
    lv_label_set_text(ui_HintLabel, "Locking SD card and preparing USB transfer bridge.");
    lv_label_set_text(ui_StateSymbol, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(ui_StateSymbol, lv_color_hex(0x1C1F23), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_state(ui_StartButton, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_StopButton, LV_STATE_DISABLED);
    lv_obj_add_state(ui_BackButton, LV_STATE_DISABLED);
}

void FileTransferGui::handleStart()
{
    transferSessionAttempted = true;
    renderConnecting();
    lv_obj_invalidate(ui_Widget);
    // This runs inside LVGL's input timer. Leave rendering to the outer handler
    // instead of adding a full synchronous redraw to the input callback stack.
    transferService.start();
    refresh();
}

void FileTransferGui::handleStop()
{
    const FileTransferState previousState = transferService.getState();
    const bool wasTransferRunning = transferService.isTransferModeActive() || previousState == FileTransferState::READY;
    const bool stopped = transferService.stop();
    transferSessionAttempted = false;
    if (!stopped) {
        refresh();
        return;
    }

    renderPreparation();
    if (wasTransferRunning) {
        showRestartPrompt();
    } else {
        router.showMainMenu();
    }
}

void FileTransferGui::showRestartPrompt()
{
    closeRestartPrompt();

    static const char *buttons[] = {"Restart", "Later", ""};
    ui_RestartDialog = lv_msgbox_create(lv_scr_act(),
                                        "Restart device?",
                                        "Transfer session is closed. Restart HMI now to reload changes from SD card?",
                                        buttons,
                                        false);
    lv_obj_set_width(ui_RestartDialog, 340);
    lv_obj_center(ui_RestartDialog);
    lv_obj_move_foreground(ui_RestartDialog);
    lv_obj_add_event_cb(ui_RestartDialog, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        auto *self = static_cast<FileTransferGui *>(lv_event_get_user_data(e));
        lv_obj_t *msgbox = lv_event_get_current_target(e);
        const char *btnText = lv_msgbox_get_active_btn_text(msgbox);

        if (btnText && std::strcmp(btnText, "Restart") == 0) {
            self->closeRestartPrompt();
            self->restartDevice();
            return;
        }

        self->closeRestartPrompt();
        self->router.showMainMenu();
    }, LV_EVENT_ALL, this);
}

void FileTransferGui::closeRestartPrompt()
{
    if (!ui_RestartDialog) {
        return;
    }

    lv_obj_t *dialog = ui_RestartDialog;
    ui_RestartDialog = nullptr;
    lv_obj_del(dialog);
}

void FileTransferGui::restartDevice()
{
#if defined(ARDUINO) && defined(ESP32)
    ESP.restart();
#elif defined(ARDUINO)
    void (*resetFunc)(void) = 0;
    resetFunc();
#else
    std::exit(0);
#endif
}

void FileTransferGui::init()
{
    if (initialized) {
        return;
    }

    build();
    initialized = true;
}

void FileTransferGui::showFileTransfer()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    lv_obj_clear_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);
    if (!transferService.isTransferModeActive()) {
        transferSessionAttempted = false;
        renderPreparation();
    } else {
        refresh();
    }
}

void FileTransferGui::hideFileTransfer()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    closeRestartPrompt();
    lv_obj_add_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);
}

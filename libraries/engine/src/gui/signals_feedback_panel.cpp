#include "signals_feedback_panel.hpp"

void SignalsFeedbackPanel::showShadowOverlay()
{
    hideShadowOverlay();

    ui_ShadowOverlay = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(ui_ShadowOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_ShadowOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_ShadowOverlay, lv_pct(100), lv_pct(100));
    lv_obj_align(ui_ShadowOverlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(ui_ShadowOverlay, 0, 0);
    lv_obj_set_style_bg_color(ui_ShadowOverlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_ShadowOverlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(ui_ShadowOverlay, 0, 0);
}

void SignalsFeedbackPanel::hideShadowOverlay()
{
    if (ui_ShadowOverlay) {
        lv_obj_del(ui_ShadowOverlay);
        ui_ShadowOverlay = nullptr;
    }
}

void SignalsFeedbackPanel::showAlert(lv_obj_t *parentWidget, void *userData, const char *message, lv_event_cb_t dismissCallback)
{
    if (!parentWidget || !message) {
        return;
    }

    hideAlert();

    ui_Alert = lv_obj_create(parentWidget);
    lv_obj_remove_style_all(ui_Alert);
    lv_obj_set_width(ui_Alert, 400);
    lv_obj_set_height(ui_Alert, 40);
    lv_obj_set_x(ui_Alert, 0);
    lv_obj_set_y(ui_Alert, 10);
    lv_obj_set_align(ui_Alert, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Alert, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_Alert, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Alert, lv_color_hex(0x4C9ED3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Alert, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_Alert, dismissCallback, LV_EVENT_CLICKED, userData);
    lv_obj_add_event_cb(
        ui_Alert,
        [](lv_event_t *e) {
            auto *self = static_cast<SignalsFeedbackPanel *>(lv_event_get_user_data(e));
            self->ui_Alert = nullptr;
            self->ui_AlertLabel = nullptr;
        },
        LV_EVENT_DELETE,
        this);

    ui_AlertLabel = lv_label_create(ui_Alert);
    lv_obj_set_width(ui_AlertLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_AlertLabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_AlertLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_AlertLabel, message);
    lv_obj_set_style_text_font(ui_AlertLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_AlertLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_del_delayed(ui_Alert, 3000);
}

void SignalsFeedbackPanel::hideAlert()
{
    if (ui_Alert) {
        lv_obj_del(ui_Alert);
        ui_Alert = nullptr;
        ui_AlertLabel = nullptr;
    }
}

void SignalsFeedbackPanel::showConfirmationDialog(const char *title,
                                                  const char *message,
                                                  const char *buttons[],
                                                  void *userData,
                                                  lv_event_cb_t callback)
{
    showShadowOverlay();

    lv_obj_t *confirmDialog = lv_msgbox_create(lv_scr_act(), title, message, buttons, true);
    lv_obj_set_width(confirmDialog, 250);
    lv_obj_center(confirmDialog);
    lv_obj_move_foreground(confirmDialog);
    lv_obj_add_event_cb(confirmDialog, callback, LV_EVENT_ALL, userData);
}

bool SignalsFeedbackPanel::isConfirmationAccepted(lv_event_t *e, const char *buttonText) const
{
    if (!e || lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return false;
    }

    lv_obj_t *msgbox = lv_event_get_current_target(e);
    const char *activeText = lv_msgbox_get_active_btn_text(msgbox);
    return activeText && strcmp(activeText, buttonText) == 0;
}

void SignalsFeedbackPanel::closeConfirmationDialog(lv_event_t *e)
{
    hideShadowOverlay();

    if (!e) {
        return;
    }

    lv_obj_t *msgbox = lv_event_get_current_target(e);
    if (lv_event_get_code(e) != LV_EVENT_DELETE && msgbox) {
        lv_obj_del(msgbox);
    }
}

void SignalsFeedbackPanel::showDisconnect(lv_obj_t *parent, void *userData, lv_event_cb_t reconnectCallback)
{
    if (disconnectPanel || !parent) return;
    // Screen parent avoids clipping to the narrower device card (760 px).
    disconnectPanel = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(disconnectPanel);
    lv_obj_set_size(disconnectPanel, LV_PCT(100), 250);
    lv_obj_center(disconnectPanel);
    lv_obj_clear_flag(disconnectPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(disconnectPanel, lv_color_hex(0xB32632), 0);
    lv_obj_set_style_bg_opa(disconnectPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(disconnectPanel, 0, 0);
    lv_obj_set_style_radius(disconnectPanel, 0, 0);
    lv_obj_set_style_shadow_width(disconnectPanel, 0, 0);
    lv_obj_set_style_text_color(disconnectPanel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(disconnectPanel, &lv_font_montserrat_16, 0);

    auto *icon = lv_label_create(disconnectPanel);
    lv_label_set_text(icon, LV_SYMBOL_USB " " LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 18);

    auto *message = lv_label_create(disconnectPanel);
    lv_label_set_text(message, "DISCONNECT");
    lv_obj_set_style_text_font(message, &lv_font_montserrat_24, 0);
    lv_obj_align(message, LV_ALIGN_TOP_MID, 0, 58);

    auto *description = lv_label_create(disconnectPanel);
    lv_obj_set_width(description, LV_PCT(90));
    lv_obj_set_style_text_align(description, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(description,
        "Connection to Board was lost. Visualization is paused.\n"
        "Check the cable and Board power, then press Reconnect.");
    lv_obj_align(description, LV_ALIGN_TOP_MID, 0, 102);

    auto *button = lv_btn_create(disconnectPanel);
    lv_obj_set_size(button, 190, 44);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x8E1C27), LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x751821), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    lv_obj_set_style_text_color(button, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(button, &lv_font_montserrat_16, LV_PART_MAIN);
    auto *caption = lv_label_create(button);
    lv_label_set_text(caption, LV_SYMBOL_REFRESH " Reconnect");
    lv_obj_center(caption);
    lv_obj_add_event_cb(button, reconnectCallback, LV_EVENT_CLICKED, userData);
    lv_obj_move_foreground(disconnectPanel);
}

void SignalsFeedbackPanel::hideDisconnect()
{
    if (disconnectPanel) lv_obj_del(disconnectPanel);
    disconnectPanel = nullptr;
}

#ifndef SIGNALS_FEEDBACK_PANEL_HPP
#define SIGNALS_FEEDBACK_PANEL_HPP

#include "lvgl.h"

class SignalsFeedbackPanel
{
private:
    lv_obj_t *ui_ShadowOverlay = nullptr;
    lv_obj_t *ui_Alert = nullptr;
    lv_obj_t *ui_AlertLabel = nullptr;
    lv_obj_t *disconnectPanel = nullptr;

public:
    void showDisconnect(lv_obj_t *parent, void *userData, lv_event_cb_t reconnectCallback);
    void hideDisconnect();
    void showShadowOverlay();
    void hideShadowOverlay();

    void showAlert(lv_obj_t *parentWidget, void *userData, const char *message, lv_event_cb_t dismissCallback);
    void hideAlert();
    void showConfirmationDialog(const char *title,
                                const char *message,
                                const char *buttons[],
                                void *userData,
                                lv_event_cb_t callback);
    bool isConfirmationAccepted(lv_event_t *e, const char *buttonText) const;
    void closeConfirmationDialog(lv_event_t *e);
};

#endif

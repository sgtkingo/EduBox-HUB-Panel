#ifndef SETTINGS_GUI_HPP
#define SETTINGS_GUI_HPP

#include "lvgl.h"

#include "app_settings.hpp"
#include "gui_router.hpp"

class SettingsGui
{
private:
    GuiRouter &router;
    bool initialized = false;

    lv_obj_t *ui_Widget = nullptr;
    lv_obj_t *ui_CurrentMode = nullptr;
    lv_obj_t *ui_CommDropdown = nullptr;
    lv_obj_t *ui_ThemeDropdown = nullptr;
    lv_obj_t *ui_LanguageDropdown = nullptr;
    lv_obj_t *ui_UseDevicePictures = nullptr;
    lv_obj_t *ui_ApplicationInput = nullptr;
    lv_obj_t *ui_VersionInput = nullptr;
    lv_obj_t *ui_MetadataStatus = nullptr;
    lv_obj_t *ui_AppConfigStatus = nullptr;
    lv_obj_t *ui_Keyboard = nullptr;

    void build();
    void refresh();
    lv_obj_t *createSingleLineInput(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width);
    void buildKeyboard();
    void attachKeyboard(lv_obj_t *textarea);
    void handleKeyboardEvent(lv_event_t *e);
    void showKeyboardFor(lv_obj_t *textarea);
    void hideKeyboard();
    void saveAppConfig();
    static uint16_t getModeDropdownIndex(DefaultCommunicationMode mode);
    static DefaultCommunicationMode getModeFromDropdownIndex(uint16_t index);
    static uint16_t getThemeDropdownIndex(ThemeMode mode);
    static ThemeMode getThemeFromDropdownIndex(uint16_t index);
    static uint16_t getLanguageDropdownIndex(LanguageMode mode);
    static LanguageMode getLanguageFromDropdownIndex(uint16_t index);

public:
    explicit SettingsGui(GuiRouter &router);
    ~SettingsGui() = default;

    void init();
    bool isInitialized() const { return initialized; }
    void showSettings();
    void hideSettings();
};

#endif // SETTINGS_GUI_HPP

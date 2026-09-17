#include "settings_gui.hpp"

#include <string>

namespace
{
const char *DEFAULT_COMMUNICATION_OPTIONS = "Ask every time\nCable (UART)\nWireless Auto\nWireless Manual";
const char *THEME_OPTIONS = "White\nDark";
const char *LANGUAGE_OPTIONS = "EN - English\nCZ - Cestina\nDE - Deutsch";
}

SettingsGui::SettingsGui(GuiRouter &router) : router(router)
{
}

lv_obj_t *SettingsGui::createSingleLineInput(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width)
{
    lv_obj_t *input = lv_textarea_create(parent);
    lv_obj_set_size(input, width, 34);
    lv_obj_set_pos(input, x, y);
    lv_textarea_set_one_line(input, true);
    attachKeyboard(input);
    return input;
}

void SettingsGui::buildKeyboard()
{
    ui_Keyboard = lv_keyboard_create(lv_scr_act());
    lv_obj_set_size(ui_Keyboard, 760, 132);
    lv_obj_align(ui_Keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ui_Keyboard, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code != LV_EVENT_READY && code != LV_EVENT_CANCEL) {
            return;
        }

        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        self->hideKeyboard();
    }, LV_EVENT_ALL, this);
}

void SettingsGui::attachKeyboard(lv_obj_t *textarea)
{
    if (!textarea) {
        return;
    }

    lv_obj_add_event_cb(textarea, [](lv_event_t *e) {
        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        self->handleKeyboardEvent(e);
    }, LV_EVENT_ALL, this);
}

void SettingsGui::handleKeyboardEvent(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *textarea = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        showKeyboardFor(textarea);
        return;
    }

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        hideKeyboard();
    }
}

void SettingsGui::showKeyboardFor(lv_obj_t *textarea)
{
    if (!ui_Keyboard || !textarea) {
        return;
    }

    lv_keyboard_set_textarea(ui_Keyboard, textarea);
    lv_obj_clear_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_Keyboard);
}

void SettingsGui::hideKeyboard()
{
    if (!ui_Keyboard) {
        return;
    }

    lv_keyboard_set_textarea(ui_Keyboard, nullptr);
    lv_obj_add_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN);
}

uint16_t SettingsGui::getModeDropdownIndex(DefaultCommunicationMode mode)
{
    switch (mode) {
    case DefaultCommunicationMode::CABLE:
        return 1;
    case DefaultCommunicationMode::WIRELESS_AUTO:
        return 2;
    case DefaultCommunicationMode::WIRELESS_MANUAL:
        return 3;
    case DefaultCommunicationMode::ASK:
    default:
        return 0;
    }
}

DefaultCommunicationMode SettingsGui::getModeFromDropdownIndex(uint16_t index)
{
    switch (index) {
    case 1:
        return DefaultCommunicationMode::CABLE;
    case 2:
        return DefaultCommunicationMode::WIRELESS_AUTO;
    case 3:
        return DefaultCommunicationMode::WIRELESS_MANUAL;
    case 0:
    default:
        return DefaultCommunicationMode::ASK;
    }
}

uint16_t SettingsGui::getThemeDropdownIndex(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::DARK:
        return 1;
    case ThemeMode::LIGHT:
    default:
        return 0;
    }
}

ThemeMode SettingsGui::getThemeFromDropdownIndex(uint16_t index)
{
    return index == 1 ? ThemeMode::DARK : ThemeMode::LIGHT;
}

uint16_t SettingsGui::getLanguageDropdownIndex(LanguageMode mode)
{
    switch (mode) {
    case LanguageMode::CZECH:
        return 1;
    case LanguageMode::GERMAN:
        return 2;
    case LanguageMode::ENGLISH:
    default:
        return 0;
    }
}

LanguageMode SettingsGui::getLanguageFromDropdownIndex(uint16_t index)
{
    switch (index) {
    case 1:
        return LanguageMode::CZECH;
    case 2:
        return LanguageMode::GERMAN;
    case 0:
    default:
        return LanguageMode::ENGLISH;
    }
}

void SettingsGui::build()
{
    ui_Widget = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_Widget);
    lv_obj_set_size(ui_Widget, 760, 440);
    lv_obj_set_align(ui_Widget, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Widget, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Widget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Widget, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    buildKeyboard();

    lv_obj_t *title = lv_label_create(ui_Widget);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *communicationPanel = lv_obj_create(ui_Widget);
    lv_obj_set_size(communicationPanel, 300, 120);
    lv_obj_set_style_pad_all(communicationPanel, 8, 0);
    lv_obj_set_pos(communicationPanel, 26, 82);

    ui_CurrentMode = lv_label_create(communicationPanel);
    lv_label_set_text(ui_CurrentMode, "Default Communication");
    lv_obj_set_pos(ui_CurrentMode, 12, 12);
    lv_obj_set_style_text_font(ui_CurrentMode, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CommDropdown = lv_dropdown_create(communicationPanel);
    lv_dropdown_set_options(ui_CommDropdown, DEFAULT_COMMUNICATION_OPTIONS);
    lv_obj_set_size(ui_CommDropdown, 220, 38);
    lv_obj_set_pos(ui_CommDropdown, 12, 58);
    lv_obj_add_event_cb(ui_CommDropdown, [](lv_event_t *e) {
        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            self->hideKeyboard();
            return;
        }

        if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        self->router.setDefaultCommunicationMode(SettingsGui::getModeFromDropdownIndex(lv_dropdown_get_selected(self->ui_CommDropdown)));
    }, LV_EVENT_ALL, this);

    lv_obj_t *appearancePanel = lv_obj_create(ui_Widget);
    lv_obj_set_size(appearancePanel, 300, 160);
    lv_obj_set_pos(appearancePanel, 26, 214);
    lv_obj_set_style_pad_all(appearancePanel, 0, 0);
    lv_obj_clear_flag(appearancePanel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *appearanceTitle = lv_label_create(appearancePanel);
    lv_label_set_text(appearanceTitle, "Appearance");
    lv_obj_set_pos(appearanceTitle, 12, 12);
    lv_obj_set_style_text_font(appearanceTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *themeLabel = lv_label_create(appearancePanel);
    lv_label_set_text(themeLabel, "Theme");
    lv_obj_set_pos(themeLabel, 12, 54);

    ui_ThemeDropdown = lv_dropdown_create(appearancePanel);
    lv_dropdown_set_options(ui_ThemeDropdown, THEME_OPTIONS);
    lv_obj_set_size(ui_ThemeDropdown, 126, 34);
    lv_obj_set_pos(ui_ThemeDropdown, 114, 44);
    lv_obj_add_event_cb(ui_ThemeDropdown, [](lv_event_t *e) {
        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            self->hideKeyboard();
            return;
        }

        if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        self->router.setThemeMode(SettingsGui::getThemeFromDropdownIndex(lv_dropdown_get_selected(self->ui_ThemeDropdown)));
    }, LV_EVENT_ALL, this);

    lv_obj_t *languageLabel = lv_label_create(appearancePanel);
    lv_label_set_text(languageLabel, "Language");
    lv_obj_set_pos(languageLabel, 12, 94);

    ui_LanguageDropdown = lv_dropdown_create(appearancePanel);
    lv_dropdown_set_options(ui_LanguageDropdown, LANGUAGE_OPTIONS);
    lv_obj_set_size(ui_LanguageDropdown, 126, 34);
    lv_obj_set_pos(ui_LanguageDropdown, 114, 84);
    lv_obj_add_event_cb(ui_LanguageDropdown, [](lv_event_t *e) {
        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            self->hideKeyboard();
            return;
        }

        if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        self->router.setLanguageMode(SettingsGui::getLanguageFromDropdownIndex(lv_dropdown_get_selected(self->ui_LanguageDropdown)));
    }, LV_EVENT_ALL, this);

    ui_UseDevicePictures = lv_checkbox_create(appearancePanel);
    lv_checkbox_set_text(ui_UseDevicePictures, "Use devices pictures");
    lv_obj_set_pos(ui_UseDevicePictures, 12, 126);
    lv_obj_set_style_text_font(ui_UseDevicePictures, &lv_font_montserrat_16, 0);
    lv_obj_add_state(ui_UseDevicePictures, LV_STATE_CHECKED);

    lv_obj_t *metadataPanel = lv_obj_create(ui_Widget);
    lv_obj_set_size(metadataPanel, 340, 278);
    lv_obj_set_pos(metadataPanel, 354, 82);

    lv_obj_t *metadataTitle = lv_label_create(metadataPanel);
    lv_label_set_text(metadataTitle, "Catalog Metadata");
    lv_obj_set_pos(metadataTitle, 12, 10);
    lv_obj_set_style_text_font(metadataTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *appLabel = lv_label_create(metadataPanel);
    lv_label_set_text(appLabel, "Application");
    lv_obj_set_pos(appLabel, 12, 50);
    ui_ApplicationInput = createSingleLineInput(metadataPanel, 80, 82, 200);

    lv_obj_t *versionLabel = lv_label_create(metadataPanel);
    lv_label_set_text(versionLabel, "Version");
    lv_obj_set_pos(versionLabel, 12, 96);
    ui_VersionInput = createSingleLineInput(metadataPanel, 80, 128, 200);

    lv_obj_t *saveMetadata = lv_btn_create(metadataPanel);
    lv_obj_set_size(saveMetadata, 132, 36);
    lv_obj_set_pos(saveMetadata, 178, 208);
    lv_obj_add_event_cb(saveMetadata, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        std::string error;
        if (!self->ui_ApplicationInput || !self->ui_VersionInput || !self->ui_MetadataStatus) {
            return;
        }

        const std::string application = lv_textarea_get_text(self->ui_ApplicationInput);
        const std::string version = lv_textarea_get_text(self->ui_VersionInput);
        if (application.empty() || version.empty()) {
            lv_label_set_text(self->ui_MetadataStatus, "Application and Version are required.");
            return;
        }

        if (!self->router.saveCatalogMetadata(application, version, error)) {
            lv_label_set_text(self->ui_MetadataStatus, error.c_str());
            return;
        }

        self->hideKeyboard();
        lv_label_set_text(self->ui_MetadataStatus, "Catalog metadata saved.");
        self->refresh();
    }, LV_EVENT_ALL, this);
    lv_obj_t *saveMetadataLabel = lv_label_create(saveMetadata);
    lv_label_set_text(saveMetadataLabel, "Save Metadata");
    lv_obj_center(saveMetadataLabel);

    ui_MetadataStatus = lv_label_create(metadataPanel);
    lv_obj_set_width(ui_MetadataStatus, 390);
    lv_obj_set_pos(ui_MetadataStatus, 12, 188);
    lv_label_set_long_mode(ui_MetadataStatus, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ui_MetadataStatus, "");

    ui_AppConfigStatus = lv_label_create(ui_Widget);
    lv_obj_set_width(ui_AppConfigStatus, 440);
    lv_obj_align(ui_AppConfigStatus, LV_ALIGN_BOTTOM_RIGHT, -126, -58);
    lv_label_set_long_mode(ui_AppConfigStatus, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ui_AppConfigStatus, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_AppConfigStatus, lv_color_hex(0x5F6B7A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_AppConfigStatus, "");

    lv_obj_t *back = lv_btn_create(ui_Widget);
    lv_obj_set_size(back, 90, 36);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 16, -14);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
            self->router.showMainMenu();
        }
    }, LV_EVENT_ALL, this);
    lv_obj_t *backLabel = lv_label_create(back);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);

    lv_obj_t *save = lv_btn_create(ui_Widget);
    lv_obj_set_size(save, 96, 36);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -16, -14);
    lv_obj_set_style_bg_color(save, lv_color_hex(0x2EAD5F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(save, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<SettingsGui *>(lv_event_get_user_data(e));
        self->saveAppConfig();
    }, LV_EVENT_ALL, this);
    lv_obj_t *saveLabel = lv_label_create(save);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_center(saveLabel);
}

void SettingsGui::saveAppConfig()
{
    if (!ui_CommDropdown || !ui_ThemeDropdown || !ui_LanguageDropdown || !ui_UseDevicePictures || !ui_AppConfigStatus) {
        return;
    }

    hideKeyboard();

    const DefaultCommunicationMode communication = getModeFromDropdownIndex(lv_dropdown_get_selected(ui_CommDropdown));
    const ThemeMode theme = getThemeFromDropdownIndex(lv_dropdown_get_selected(ui_ThemeDropdown));
    const LanguageMode language = getLanguageFromDropdownIndex(lv_dropdown_get_selected(ui_LanguageDropdown));

    std::string error;
    const bool useDevicePictures = lv_obj_has_state(ui_UseDevicePictures, LV_STATE_CHECKED);
    if (!router.saveAppSettings(communication, theme, language, useDevicePictures, error)) {
        lv_obj_set_style_text_color(ui_AppConfigStatus, lv_color_hex(0xB00020), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_AppConfigStatus, error.empty() ? "Settings save failed." : error.c_str());
        return;
    }

    lv_obj_set_style_text_color(ui_AppConfigStatus, lv_color_hex(0x2E7D32), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_AppConfigStatus, "Settings saved.");
    refresh();
}

void SettingsGui::refresh()
{
    if (!ui_CommDropdown) {
        return;
    }

    lv_dropdown_set_selected(ui_CommDropdown, getModeDropdownIndex(router.getDefaultCommunicationMode()));
    if (ui_ThemeDropdown) {
        lv_dropdown_set_selected(ui_ThemeDropdown, getThemeDropdownIndex(router.getThemeMode()));
    }
    if (ui_LanguageDropdown) {
        lv_dropdown_set_selected(ui_LanguageDropdown, getLanguageDropdownIndex(router.getLanguageMode()));
    }
    if (ui_UseDevicePictures) {
        if (router.getUseDevicePictures()) lv_obj_add_state(ui_UseDevicePictures, LV_STATE_CHECKED);
        else lv_obj_clear_state(ui_UseDevicePictures, LV_STATE_CHECKED);
    }

    if (ui_ApplicationInput) {
        lv_textarea_set_text(ui_ApplicationInput, router.getCatalogApplication().c_str());
    }
    if (ui_VersionInput) {
        lv_textarea_set_text(ui_VersionInput, router.getCatalogVersion().c_str());
    }

    if(ui_AppConfigStatus){
        lv_obj_clear_flag(ui_AppConfigStatus, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_MetadataStatus) {
        lv_obj_clear_flag(ui_MetadataStatus, LV_OBJ_FLAG_HIDDEN);   
    }
}

void SettingsGui::init()
{
    if (initialized) {
        return;
    }

    build();
    initialized = true;
}

void SettingsGui::showSettings()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    refresh();
    hideKeyboard();
    lv_obj_clear_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);

    // Reset status messages
    lv_obj_add_flag(ui_AppConfigStatus, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_MetadataStatus, LV_OBJ_FLAG_HIDDEN);
}

void SettingsGui::hideSettings()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    hideKeyboard();
    lv_obj_add_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);
}

#include "library_editor_gui.hpp"

#include "../helpers.hpp"
#include "../managers/storage_manager.hpp"
#include "../managers/app_config_manager.hpp"
#include "./device_catalog_browser.hpp"
#include "./images/ui_images.h"
#include "lvgl_storage_fs.hpp"
#include "expt.hpp"

#include <algorithm>
#include <cstdint>

namespace
{
constexpr lv_coord_t EDITOR_PICTURE_PREVIEW_SIZE = 160;

std::string trimCopy(const std::string &value)
{
    const size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::vector<std::string> splitAndTrimCsv(const std::string &csv)
{
    std::vector<std::string> items;
    for (const std::string &raw : splitString(csv, ',')) {
        const std::string item = trimCopy(raw);
        if (!item.empty()) {
            items.push_back(item);
        }
    }
    return items;
}

const char *ROLE_OPTIONS = "Sensor\nActuator\nHybrid";
const char *PARAM_DTYPE_OPTIONS = "int\nfloat\ndouble\nstring";
const char *DEVICE_PICTURE_DIR = STORAGE_DEVICE_PICTURE_DIR;

const lv_img_dsc_t *placeholderImageForRole(DeviceRole role)
{
    switch (role) {
    case DeviceRole::ACTUATOR:
        return &ui_img_placeholder_actuator;
    case DeviceRole::HYBRID:
        return &ui_img_placeholder_hybrid;
    case DeviceRole::SENSOR:
    default:
        return &ui_img_placeholder_sensor;
    }
}

bool isStoragePicturePath(const std::string &path)
{
    return path.rfind(DEVICE_PICTURE_DIR, 0) == 0;
}

bool isGifPath(const std::string &path)
{
    return path.size() >= 4 && path.substr(path.size() - 4) == ".gif";
}

std::string findDevicePicturePath(const std::string &storedPicture = "")
{
    if (!AppConfigManager::useDevicePictures()) return "";
    if (storedPicture.empty() || storedPicture == "placeholder:device") {
        return "";
    }

    if (!storageManager().isAvailable() && !storageManager().init()) {
        debugLogMessage("LibraryEditorGui::findDevicePicturePath", "storage unavailable", "picture=%s", storedPicture.c_str());
        return "";
    }
    storageManager().ensureDirectory(DEVICE_PICTURE_DIR);

    if (isStoragePicturePath(storedPicture) && storageManager().exists(storedPicture)) {
        return storedPicture;
    }

    const std::string normalizedPicture = trimCopy(storedPicture);
    if (normalizedPicture.empty()) {
        return "";
    }

    const std::string candidate = std::string(DEVICE_PICTURE_DIR) + "/" + normalizedPicture;
    if (storageManager().exists(candidate)) {
        return candidate;
    }

    debugLogMessage(
        "LibraryEditorGui::findDevicePicturePath",
        "picture missing",
        "stored=%s candidate=%s",
        normalizedPicture.c_str(),
        candidate.c_str()
    );

    return "";
}
}

LibraryEditorGui::LibraryEditorGui(DeviceCatalog &deviceCatalog, DeviceBrowserState &browserState, GuiRouter &router)
    : deviceCatalog(deviceCatalog), browserState(browserState), router(router)
{
}

void LibraryEditorGui::build()
{
    debugLogMessage("LibraryEditorGui::build", "gui init", "create root");
    ui_Widget = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_Widget);
    lv_obj_set_size(ui_Widget, 760, 440);
    lv_obj_set_align(ui_Widget, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_Widget, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Widget, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Widget, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    debugLogMessage("LibraryEditorGui::build", "gui init", "build keyboard");
    buildKeyboard();

    debugLogMessage("LibraryEditorGui::build", "gui init", "build form");
    ui_Title = lv_label_create(ui_Widget);
    lv_obj_align(ui_Title, LV_ALIGN_TOP_MID, 0, 14);
    lv_obj_set_style_text_font(ui_Title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Form = lv_obj_create(ui_Widget);
    lv_obj_set_size(ui_Form, 720, 320);
    lv_obj_align(ui_Form, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_set_scroll_dir(ui_Form, LV_DIR_VER);

    createFieldLabel(ui_Form, "UID", 10, 8);
    ui_UidInput = createSingleLineInput(ui_Form, 110, 0, 210);

    createFieldLabel(ui_Form, "Type", 10, 50);
    ui_TypeInput = createSingleLineInput(ui_Form, 110, 48, 210);

    createFieldLabel(ui_Form, "Role", 10, 92);
    ui_RoleDropdown = lv_dropdown_create(ui_Form);
    lv_dropdown_set_options(ui_RoleDropdown, ROLE_OPTIONS);
    lv_obj_set_size(ui_RoleDropdown, 210, 34);
    lv_obj_set_pos(ui_RoleDropdown, 110, 94);
    lv_obj_add_event_cb(ui_RoleDropdown, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        self->updatePicturePreview(lv_textarea_get_text(self->ui_UidInput));
    }, LV_EVENT_VALUE_CHANGED, this);

    createFieldLabel(ui_Form, "Allowed Pins", 10, 144);
    ui_AllowedPinsInput = createSingleLineInput(ui_Form, 110, 144, 210);

    createFieldLabel(ui_Form, "Pins", 10, 196);
    ui_DevicePinsInput = createSingleLineInput(ui_Form, 110, 196, 210);

    createFieldLabel(ui_Form, "Default Pins", 10, 240);
    ui_DefaultPinsInput = createSingleLineInput(ui_Form, 110, 240, 210);

    lv_obj_add_event_cb(ui_UidInput, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code != LV_EVENT_READY && code != LV_EVENT_DEFOCUSED) {
            return;
        }

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        self->updatePicturePreview(lv_textarea_get_text(self->ui_UidInput));
    }, LV_EVENT_ALL, this);

    createFieldLabel(ui_Form, "Description", 340, 8);
    ui_DescriptionInput = lv_textarea_create(ui_Form);
    lv_obj_set_size(ui_DescriptionInput, 340, 98);
    lv_obj_set_pos(ui_DescriptionInput, 340, 0);
    lv_textarea_set_one_line(ui_DescriptionInput, false);
    attachKeyboard(ui_DescriptionInput);

    ui_PicturePreview = lv_obj_create(ui_Form);
    lv_obj_set_size(ui_PicturePreview, EDITOR_PICTURE_PREVIEW_SIZE, EDITOR_PICTURE_PREVIEW_SIZE);
    lv_obj_set_pos(ui_PicturePreview, 340, 122);
    lv_obj_clear_flag(ui_PicturePreview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_PicturePreview, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PicturePreview, lv_color_hex(0xF5F5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PicturePreview, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PicturePreview, lv_color_hex(0xD8D8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_PicturePreview, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    debugLogMessage("LibraryEditorGui::build", "gui init", "build picture preview");
    ui_PictureImage = lv_img_create(ui_PicturePreview);
    lv_img_set_src(ui_PictureImage, placeholderImageForRole(DeviceRole::SENSOR));
    lv_img_set_zoom(ui_PictureImage, 256);
    lv_obj_center(ui_PictureImage);
    ui_PictureCanvas = lv_canvas_create(ui_PicturePreview);
    lv_obj_remove_style_all(ui_PictureCanvas);
    lv_obj_set_size(ui_PictureCanvas, EDITOR_PICTURE_PREVIEW_SIZE, EDITOR_PICTURE_PREVIEW_SIZE);
    lv_obj_center(ui_PictureCanvas);
    lv_obj_add_flag(ui_PictureCanvas, LV_OBJ_FLAG_HIDDEN);

    ui_PictureFallbackLabel = lv_label_create(ui_PicturePreview);
    lv_label_set_text(ui_PictureFallbackLabel, "Picture not provided");
    lv_obj_set_style_text_color(ui_PictureFallbackLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_width(ui_PictureFallbackLabel, 146);
    lv_obj_set_style_text_align(ui_PictureFallbackLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_PictureFallbackLabel, LV_ALIGN_BOTTOM_MID, 0, -16);
    ui_PictureWarningBadge = nullptr;

    debugLogMessage("LibraryEditorGui::build", "gui init", "build params");
    ui_ValuesPanel = lv_obj_create(ui_Form);
    lv_obj_set_size(ui_ValuesPanel, 330, 250);
    lv_obj_set_pos(ui_ValuesPanel, 2, 360);
    buildParamListSection(ui_ValuesPanel, "Values", &ui_ValuesList, false);

    ui_ConfigsPanel = lv_obj_create(ui_Form);
    lv_obj_set_size(ui_ConfigsPanel, 330, 250);
    lv_obj_set_pos(ui_ConfigsPanel, 342, 360);
    buildParamListSection(ui_ConfigsPanel, "Configs", &ui_ConfigsList, true);

    debugLogMessage("LibraryEditorGui::build", "gui init", "build param editor");
    buildParamEditor();

    lv_obj_t *back = lv_btn_create(ui_Widget);
    lv_obj_set_size(back, 90, 36);
    lv_obj_align(back, LV_ALIGN_BOTTOM_LEFT, 16, -14);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
            self->router.showLibrary();
        }
    }, LV_EVENT_ALL, this);
    lv_obj_t *backLabel = lv_label_create(back);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);

    lv_obj_t *save = lv_btn_create(ui_Widget);
    lv_obj_set_size(save, 90, 36);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -16, -14);
    lv_obj_add_event_cb(save, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
            self->saveDraft();
        }
    }, LV_EVENT_ALL, this);
    lv_obj_t *saveLabel = lv_label_create(save);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_center(saveLabel);
    debugLogMessage("LibraryEditorGui::build", "gui init", "build done");
}

void LibraryEditorGui::buildParamListSection(lv_obj_t *panel, const char *title, lv_obj_t **listOut, bool configSection)
{
    lv_obj_t *titleLabel = lv_label_create(panel);
    lv_label_set_text(titleLabel, title);
    lv_obj_set_pos(titleLabel, 8, 8);

    lv_obj_t *add = lv_btn_create(panel);
    lv_obj_set_size(add, 54, 26);
    lv_obj_align(add, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_set_user_data(add, reinterpret_cast<void *>(static_cast<intptr_t>(configSection ? 1 : 0)));
    lv_obj_add_event_cb(add, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        const bool isConfig = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e))) != 0;
        self->showParamEditor(isConfig, -1);
    }, LV_EVENT_ALL, this);
    lv_obj_t *addLabel = lv_label_create(add);
    lv_label_set_text(addLabel, "Add");
    lv_obj_center(addLabel);

    *listOut = lv_list_create(panel);
    lv_obj_set_size(*listOut, 240, 160);
    lv_obj_set_pos(*listOut, 2, 42);
}

void LibraryEditorGui::buildKeyboard()
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

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        self->hideKeyboard();
    }, LV_EVENT_ALL, this);
}

void LibraryEditorGui::attachKeyboard(lv_obj_t *textarea)
{
    if (!textarea) {
        return;
    }

    lv_obj_add_event_cb(textarea, [](lv_event_t *e) {
        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        self->handleKeyboardEvent(e);
    }, LV_EVENT_ALL, this);
}

void LibraryEditorGui::handleKeyboardEvent(lv_event_t *e)
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

void LibraryEditorGui::showKeyboardFor(lv_obj_t *textarea)
{
    if (!ui_Keyboard || !textarea) {
        return;
    }

    lv_keyboard_set_textarea(ui_Keyboard, textarea);
    lv_obj_clear_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_Keyboard);
}

void LibraryEditorGui::hideKeyboard()
{
    if (!ui_Keyboard) {
        return;
    }

    lv_keyboard_set_textarea(ui_Keyboard, nullptr);
    lv_obj_add_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN);
}

void LibraryEditorGui::updatePicturePreview(const std::string &deviceUid, const std::string &storedPicture)
{
    if (!ui_PictureImage || !ui_PictureFallbackLabel) {
        return;
    }
    (void)deviceUid;

    pictureSourcePath.clear();
    const std::string picturePath = findDevicePicturePath(storedPicture);
    const bool hasPicture = !picturePath.empty();
    const bool pictureConfigured = AppConfigManager::useDevicePictures() && !storedPicture.empty() && storedPicture != "placeholder:device";

#if LV_USE_GIF
    if (ui_PictureGif) {
        lv_obj_del(ui_PictureGif);
        ui_PictureGif = nullptr;
    }
#endif

    if (!hasPicture) {
        DeviceRole role = DeviceRole::SENSOR;
        if (ui_RoleDropdown) {
            role = getRoleFromDropdownIndex(lv_dropdown_get_selected(ui_RoleDropdown));
        }

        DeviceCatalogBrowserRenderer::applyImagePreview(
            ui_PicturePreview,
            ui_PictureImage,
            ui_PictureCanvas,
            placeholderImageForRole(role),
            EDITOR_PICTURE_PREVIEW_SIZE,
            EDITOR_PICTURE_PREVIEW_SIZE,
            &ui_PicturePreviewCache
        );
        lv_obj_clear_flag(ui_PictureImage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_PictureFallbackLabel, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_PictureFallbackLabel, !AppConfigManager::useDevicePictures()
            ? "Pictures disabled" : (pictureConfigured ? "Picture not found" : "Picture not provided"));
        if (pictureConfigured) {
            if (!ui_PictureWarningBadge) {
                ui_PictureWarningBadge = lv_obj_create(ui_PicturePreview);
                lv_obj_remove_style_all(ui_PictureWarningBadge);
                lv_obj_set_size(ui_PictureWarningBadge, 28, 28);
                lv_obj_align(ui_PictureWarningBadge, LV_ALIGN_TOP_RIGHT, -8, 8);
                lv_obj_set_style_radius(ui_PictureWarningBadge, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(ui_PictureWarningBadge, lv_color_hex(0xF28C28), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(ui_PictureWarningBadge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_t *warningText = lv_label_create(ui_PictureWarningBadge);
                lv_label_set_text(warningText, "!");
                lv_obj_set_style_text_color(warningText, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(warningText, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_center(warningText);
            }
            lv_obj_clear_flag(ui_PictureWarningBadge, LV_OBJ_FLAG_HIDDEN);
        } else if (ui_PictureWarningBadge) {
            lv_obj_add_flag(ui_PictureWarningBadge, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    ensureLvglStorageFsRegistered();
    pictureSourcePath = "S:" + picturePath;
    lv_obj_add_flag(ui_PictureFallbackLabel, LV_OBJ_FLAG_HIDDEN);
    if (ui_PictureWarningBadge) {
        lv_obj_add_flag(ui_PictureWarningBadge, LV_OBJ_FLAG_HIDDEN);
    }

#if LV_USE_GIF
    if (isGifPath(picturePath)) {
        if (ui_PicturePreviewCache) {
            lv_img_buf_free(ui_PicturePreviewCache);
            ui_PicturePreviewCache = nullptr;
        }
        if (ui_PictureCanvas) {
            lv_obj_add_flag(ui_PictureCanvas, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(ui_PictureImage, LV_OBJ_FLAG_HIDDEN);
        ui_PictureGif = lv_gif_create(ui_PicturePreview);
        lv_gif_set_src(ui_PictureGif, pictureSourcePath.c_str());
        lv_obj_center(ui_PictureGif);
        return;
    }
#endif

    DeviceCatalogBrowserRenderer::applyImagePreview(
        ui_PicturePreview,
        ui_PictureImage,
        ui_PictureCanvas,
        pictureSourcePath.c_str(),
        EDITOR_PICTURE_PREVIEW_SIZE,
        EDITOR_PICTURE_PREVIEW_SIZE,
        &ui_PicturePreviewCache
    );
}

void LibraryEditorGui::buildParamEditor()
{
    ui_ParamEditorOverlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ui_ParamEditorOverlay);
    lv_obj_set_size(ui_ParamEditorOverlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(ui_ParamEditorOverlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ParamEditorOverlay, LV_OPA_40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(ui_ParamEditorOverlay, LV_OBJ_FLAG_HIDDEN);

    ui_ParamEditorPanel = lv_obj_create(ui_ParamEditorOverlay);
    lv_obj_set_size(ui_ParamEditorPanel, 500, 320);
    lv_obj_center(ui_ParamEditorPanel);

    ui_ParamEditorTitle = lv_label_create(ui_ParamEditorPanel);
    lv_obj_align(ui_ParamEditorTitle, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_font(ui_ParamEditorTitle, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    createFieldLabel(ui_ParamEditorPanel, "Key", 16, 48);
    ui_ParamKeyInput = createSingleLineInput(ui_ParamEditorPanel, 110, 40, 120);

    createFieldLabel(ui_ParamEditorPanel, "Type", 240, 48);
    ui_ParamTypeDropdown = lv_dropdown_create(ui_ParamEditorPanel);
    lv_dropdown_set_options(ui_ParamTypeDropdown, PARAM_DTYPE_OPTIONS);
    lv_obj_set_size(ui_ParamTypeDropdown, 120, 34);
    lv_obj_set_pos(ui_ParamTypeDropdown, 304, 40);

    createFieldLabel(ui_ParamEditorPanel, "Unit", 16, 88);
    ui_ParamUnitInput = createSingleLineInput(ui_ParamEditorPanel, 110, 80, 120);

    createFieldLabel(ui_ParamEditorPanel, "Value", 240, 88);
    ui_ParamValueInput = createSingleLineInput(ui_ParamEditorPanel, 304, 80, 120);

    createFieldLabel(ui_ParamEditorPanel, "Min", 16, 128);
    ui_ParamMinInput = createSingleLineInput(ui_ParamEditorPanel, 110, 120, 120);

    createFieldLabel(ui_ParamEditorPanel, "Max", 240, 128);
    ui_ParamMaxInput = createSingleLineInput(ui_ParamEditorPanel, 304, 120, 120);

    createFieldLabel(ui_ParamEditorPanel, "Step", 16, 168);
    ui_ParamStepInput = createSingleLineInput(ui_ParamEditorPanel, 110, 160, 120);

    createFieldLabel(ui_ParamEditorPanel, "Options", 240, 168);
    ui_ParamOptionsInput = createSingleLineInput(ui_ParamEditorPanel, 304, 160, 120);

    lv_obj_t *cancel = lv_btn_create(ui_ParamEditorPanel);
    lv_obj_set_size(cancel, 72, 32);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 16, -16);
    lv_obj_add_event_cb(cancel, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
            self->hideParamEditor();
        }
    }, LV_EVENT_ALL, this);
    lv_obj_t *cancelLabel = lv_label_create(cancel);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(cancelLabel);

    lv_obj_t *remove = lv_btn_create(ui_ParamEditorPanel);
    lv_obj_set_size(remove, 80, 32);
    lv_obj_align(remove, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_add_event_cb(remove, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        std::vector<DeviceParamSchema> &params = self->currentEditedParamCollection();
        if (self->editingParamIndex >= 0 && self->editingParamIndex < static_cast<int>(params.size())) {
            params.erase(params.begin() + self->editingParamIndex);
            self->populateParamList(self->editingConfigParam ? self->ui_ConfigsList : self->ui_ValuesList,
                                    params,
                                    self->editingConfigParam);
        }
        self->hideParamEditor();
    }, LV_EVENT_ALL, this);
    lv_obj_t *removeLabel = lv_label_create(remove);
    lv_label_set_text(removeLabel, "Remove");
    lv_obj_set_style_bg_color(remove, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(removeLabel);

    lv_obj_t *save = lv_btn_create(ui_ParamEditorPanel);
    lv_obj_set_size(save, 72, 32);
    lv_obj_align(save, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
    lv_obj_add_event_cb(save, [](lv_event_t *e) {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
            return;
        }

        auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
        std::string error;
        if (!self->saveEditedParam(error)) {
            splashMessage(error.c_str());
            return;
        }
        self->hideParamEditor();
    }, LV_EVENT_ALL, this);
    lv_obj_t *saveLabel = lv_label_create(save);
    lv_label_set_text(saveLabel, "OK");
    lv_obj_center(saveLabel);
}

lv_obj_t *LibraryEditorGui::createFieldLabel(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *LibraryEditorGui::createSingleLineInput(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t width)
{
    lv_obj_t *input = lv_textarea_create(parent);
    lv_obj_set_size(input, width, 34);
    lv_obj_set_pos(input, x, y);
    lv_textarea_set_one_line(input, true);
    attachKeyboard(input);
    return input;
}

uint16_t LibraryEditorGui::getRoleDropdownIndex(DeviceRole role)
{
    switch (role) {
    case DeviceRole::ACTUATOR:
        return 1;
    case DeviceRole::HYBRID:
        return 2;
    case DeviceRole::SENSOR:
    default:
        return 0;
    }
}

uint16_t LibraryEditorGui::getDataTypeDropdownIndex(DeviceDataType dtype)
{
    switch (dtype) {
    case DeviceDataType::FLOAT:
        return 1;
    case DeviceDataType::DOUBLE:
        return 2;
    case DeviceDataType::STRING:
        return 3;
    case DeviceDataType::INT:
    default:
        return 0;
    }
}

DeviceRole LibraryEditorGui::getRoleFromDropdownIndex(uint16_t index)
{
    switch (index) {
    case 1:
        return DeviceRole::ACTUATOR;
    case 2:
        return DeviceRole::HYBRID;
    case 0:
    default:
        return DeviceRole::SENSOR;
    }
}

DeviceDataType LibraryEditorGui::getDataTypeFromDropdownIndex(uint16_t index)
{
    switch (index) {
    case 1:
        return DeviceDataType::FLOAT;
    case 2:
        return DeviceDataType::DOUBLE;
    case 3:
        return DeviceDataType::STRING;
    case 0:
    default:
        return DeviceDataType::INT;
    }
}

std::vector<std::string> LibraryEditorGui::parsePinsCsv(const std::string &csv)
{
    return splitAndTrimCsv(csv);
}

std::string LibraryEditorGui::formatPinsCsv(const std::vector<std::string> &pins)
{
    std::string csv;
    for (const std::string &pin : pins) {
        if (pin.empty()) {
            continue;
        }

        if (!csv.empty()) {
            csv += ",";
        }
        csv += pin;
    }
    return csv;
}

std::map<std::string, std::string> LibraryEditorGui::parseDefaultPinsMap(const std::string &csv)
{
    std::map<std::string, std::string> pins;
    for (const std::string &entry : splitAndTrimCsv(csv)) {
        const size_t separator = entry.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string tag = trimCopy(entry.substr(0, separator));
        const std::string pin = trimCopy(entry.substr(separator + 1));
        if (tag.empty() || pin.empty()) {
            continue;
        }

        pins[tag] = pin;
    }
    return pins;
}

std::string LibraryEditorGui::formatDefaultPinsMap(const std::map<std::string, std::string> &pins)
{
    std::string csv;
    for (const auto &pinAssignment : pins) {
        if (pinAssignment.first.empty() || pinAssignment.second.empty()) {
            continue;
        }

        if (!csv.empty()) {
            csv += ",";
        }
        csv += pinAssignment.first + ":" + pinAssignment.second;
    }
    return csv;
}

void LibraryEditorGui::populateParamList(lv_obj_t *list, const std::vector<DeviceParamSchema> &params, bool configSection)
{
    if (!list) {
        return;
    }

    lv_obj_clean(list);
    for (size_t i = 0; i < params.size(); ++i) {
        const DeviceParamSchema &schema = params[i];
        std::string label = schema.key;
        if (!schema.param.Unit.empty()) {
            label += " [" + schema.param.Unit + "]";
        }

        lv_obj_t *button = lv_list_add_btn(list, nullptr, label.c_str());
        lv_obj_set_user_data(button,
                             reinterpret_cast<void *>(static_cast<intptr_t>(i | (configSection ? 0x10000 : 0))));
        lv_obj_add_event_cb(button, [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
                return;
            }

            auto *self = static_cast<LibraryEditorGui *>(lv_event_get_user_data(e));
            const intptr_t packed = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_current_target(e)));
            const bool isConfig = (packed & 0x10000) != 0;
            const int index = static_cast<int>(packed & 0xFFFF);
            self->showParamEditor(isConfig, index);
        }, LV_EVENT_ALL, this);
    }
}

void LibraryEditorGui::showParamEditor(bool configSection, int index)
{
    editingConfigParam = configSection;
    editingParamIndex = index;

    lv_label_set_text(ui_ParamEditorTitle, configSection ? "Edit Config" : "Edit Value");
    lv_textarea_set_text(ui_ParamKeyInput, "");
    lv_dropdown_set_selected(ui_ParamTypeDropdown, 0);
    lv_textarea_set_text(ui_ParamUnitInput, "");
    lv_textarea_set_text(ui_ParamValueInput, "");
    lv_textarea_set_text(ui_ParamMinInput, "");
    lv_textarea_set_text(ui_ParamMaxInput, "");
    lv_textarea_set_text(ui_ParamStepInput, "");
    lv_textarea_set_text(ui_ParamOptionsInput, "");

    std::vector<DeviceParamSchema> &params = currentEditedParamCollection();
    if (index >= 0 && index < static_cast<int>(params.size())) {
        const DeviceParamSchema &schema = params[index];
        lv_textarea_set_text(ui_ParamKeyInput, schema.key.c_str());
        lv_dropdown_set_selected(ui_ParamTypeDropdown, getDataTypeDropdownIndex(schema.param.DType));
        lv_textarea_set_text(ui_ParamUnitInput, schema.param.Unit.c_str());
        lv_textarea_set_text(ui_ParamValueInput, schema.param.Value.c_str());
        lv_textarea_set_text(ui_ParamMinInput, schema.param.Restrictions.Min.c_str());
        lv_textarea_set_text(ui_ParamMaxInput, schema.param.Restrictions.Max.c_str());
        lv_textarea_set_text(ui_ParamStepInput, schema.param.Restrictions.Step.c_str());
        lv_textarea_set_text(ui_ParamOptionsInput, schema.param.Restrictions.Options.c_str());
    }

    lv_obj_move_foreground(ui_ParamEditorOverlay);
    lv_obj_clear_flag(ui_ParamEditorOverlay, LV_OBJ_FLAG_HIDDEN);
}

void LibraryEditorGui::hideParamEditor()
{
    editingParamIndex = -1;
    hideKeyboard();
    lv_obj_add_flag(ui_ParamEditorOverlay, LV_OBJ_FLAG_HIDDEN);
}

std::vector<DeviceParamSchema> &LibraryEditorGui::currentEditedParamCollection()
{
    return editingConfigParam ? configDraftParams : valueDraftParams;
}

const std::vector<DeviceParamSchema> &LibraryEditorGui::currentEditedParamCollection() const
{
    return editingConfigParam ? configDraftParams : valueDraftParams;
}

bool LibraryEditorGui::validateParamSchema(const DeviceParamSchema &schema, std::string &error)
{
    if (schema.key.empty()) {
        error = "Parameter key is required.";
        return false;
    }

    if (schema.param.Value.empty()) {
        error = "Parameter value is required.";
        return false;
    }

    auto validateNumeric = [&](const std::string &text, const char *label) -> bool {
        if (text.empty()) {
            return true;
        }

        try {
            convertStringToType<double>(text);
            return true;
        } catch (...) {
            error = std::string(label) + " must be numeric.";
            return false;
        }
    };

    if (schema.param.DType == DeviceDataType::STRING) {
        if (!schema.param.Restrictions.Min.empty() ||
            !schema.param.Restrictions.Max.empty() ||
            !schema.param.Restrictions.Step.empty()) {
            error = "String params cannot use Min/Max/Step restrictions.";
            return false;
        }
    } else {
        if (!validateNumeric(schema.param.Value, "Value") ||
            !validateNumeric(schema.param.Restrictions.Min, "Min") ||
            !validateNumeric(schema.param.Restrictions.Max, "Max") ||
            !validateNumeric(schema.param.Restrictions.Step, "Step")) {
            return false;
        }

        if (!schema.param.Restrictions.Min.empty() && !schema.param.Restrictions.Max.empty()) {
            if (convertStringToType<double>(schema.param.Restrictions.Min) >
                convertStringToType<double>(schema.param.Restrictions.Max)) {
                error = "Min cannot be greater than Max.";
                return false;
            }
        }
    }

    if (!schema.param.Restrictions.Options.empty()) {
        const std::vector<std::string> options = splitAndTrimCsv(schema.param.Restrictions.Options);
        if (options.empty()) {
            error = "Options cannot be empty when provided.";
            return false;
        }

        if (std::find(options.begin(), options.end(), schema.param.Value) == options.end()) {
            error = "Value must match one of the Options.";
            return false;
        }
    }

    return true;
}

bool LibraryEditorGui::saveEditedParam(std::string &error)
{
    DeviceParamSchema schema;
    schema.key = trimCopy(lv_textarea_get_text(ui_ParamKeyInput));
    schema.param.DType = getDataTypeFromDropdownIndex(lv_dropdown_get_selected(ui_ParamTypeDropdown));
    schema.param.Unit = trimCopy(lv_textarea_get_text(ui_ParamUnitInput));
    schema.param.Value = trimCopy(lv_textarea_get_text(ui_ParamValueInput));
    schema.param.Restrictions.Min = trimCopy(lv_textarea_get_text(ui_ParamMinInput));
    schema.param.Restrictions.Max = trimCopy(lv_textarea_get_text(ui_ParamMaxInput));
    schema.param.Restrictions.Step = trimCopy(lv_textarea_get_text(ui_ParamStepInput));
    schema.param.Restrictions.Options = trimCopy(lv_textarea_get_text(ui_ParamOptionsInput));
    schema.param.lastHistoryIndex = 0;
    for (int i = 0; i < HISTORY_CAP; ++i) {
        schema.param.History[i] = schema.param.Value;
    }

    if (!validateParamSchema(schema, error)) {
        return false;
    }

    std::vector<DeviceParamSchema> &params = currentEditedParamCollection();
    for (size_t i = 0; i < params.size(); ++i) {
        if (static_cast<int>(i) == editingParamIndex) {
            continue;
        }
        if (params[i].key == schema.key) {
            error = "Parameter key already exists in this section.";
            return false;
        }
    }

    if (editingParamIndex >= 0 && editingParamIndex < static_cast<int>(params.size())) {
        schema.sourceIndex = params[editingParamIndex].sourceIndex;
        schema.order = params[editingParamIndex].order;
        params[editingParamIndex] = schema;
    } else {
        schema.sourceIndex = params.size();
        schema.order = static_cast<int>(schema.sourceIndex);
        params.push_back(schema);
    }

    populateParamList(editingConfigParam ? ui_ConfigsList : ui_ValuesList,
                      params,
                      editingConfigParam);
    return true;
}

bool LibraryEditorGui::validateTopLevelDraft(const DeviceDefinitionSchema &draft, std::string &error) const
{
    if (draft.uid.empty() || draft.type.empty()) {
        error = "UID and Type are required.";
        return false;
    }

    if (draft.values.empty() && draft.configs.empty()) {
        error = "At least one Value or Config is required.";
        return false;
    }

    if (draft.pins.empty()) {
        error = "Pins must define at least one logical pin.";
        return false;
    }

    std::vector<std::string> keys;
    for (const DeviceParamSchema &schema : draft.values) {
        if (!validateParamSchema(schema, error)) {
            return false;
        }
        keys.push_back(schema.key);
    }

    for (const DeviceParamSchema &schema : draft.configs) {
        if (!validateParamSchema(schema, error)) {
            return false;
        }
        if (std::find(keys.begin(), keys.end(), schema.key) != keys.end()) {
            error = "Value and Config keys must be unique across the device.";
            return false;
        }
    }

    return true;
}

void LibraryEditorGui::ensureDraftLoaded()
{
    if (browserState.hasActiveLibraryDraft()) {
        return;
    }

    if (browserState.beginLibraryDraftFromLibraryDevice()) {
        return;
    }

    browserState.beginNewLibraryDraft();
}

void LibraryEditorGui::refresh()
{
    (void)deviceCatalog;
    ensureDraftLoaded();

    const DeviceDefinitionSchema *draft = browserState.getLibraryDraft();
    if (!draft) {
        return;
    }

    lv_label_set_text(ui_Title, browserState.isLibraryDraftNewEntity() ? "New Entity" : "Edit Entity");
    lv_textarea_set_text(ui_UidInput, draft->uid.c_str());
    lv_textarea_set_text(ui_TypeInput, draft->type.c_str());
    lv_dropdown_set_selected(ui_RoleDropdown, getRoleDropdownIndex(draft->role));
    lv_textarea_set_text(ui_AllowedPinsInput, draft->allowedPinsCsv.c_str());
    const std::string pinsText = formatPinsCsv(draft->pins);
    const std::string defaultPinsText = formatDefaultPinsMap(draft->defaultPins);
    lv_textarea_set_text(ui_DevicePinsInput, pinsText.c_str());
    lv_textarea_set_text(ui_DefaultPinsInput, defaultPinsText.c_str());
    lv_textarea_set_text(ui_DescriptionInput, draft->description.c_str());
    updatePicturePreview(draft->uid, draft->picture);

    valueDraftParams = draft->values;
    configDraftParams = draft->configs;
    populateParamList(ui_ValuesList, valueDraftParams, false);
    populateParamList(ui_ConfigsList, configDraftParams, true);
}

void LibraryEditorGui::saveDraft()
{
    DeviceDefinitionSchema draft;
    BaseDevice *currentLibraryDevice = browserState.getLibraryDevice();
    const DeviceDefinitionSchema *currentDraft = browserState.getLibraryDraft();
    const std::string originalUid = currentLibraryDevice ? currentLibraryDevice->UID : "";
    const bool isNewEntity = browserState.isLibraryDraftNewEntity();

    draft.uid = trimCopy(lv_textarea_get_text(ui_UidInput));
    draft.type = trimCopy(lv_textarea_get_text(ui_TypeInput));
    draft.description = trimCopy(lv_textarea_get_text(ui_DescriptionInput));
    draft.picture = currentDraft ? currentDraft->picture : "";
    if (draft.picture.empty()) {
        draft.picture = "placeholder:device";
    }
    draft.allowedPinsCsv = trimCopy(lv_textarea_get_text(ui_AllowedPinsInput));
    draft.pins = parsePinsCsv(lv_textarea_get_text(ui_DevicePinsInput));
    draft.defaultPins = parseDefaultPinsMap(lv_textarea_get_text(ui_DefaultPinsInput));
    draft.role = getRoleFromDropdownIndex(lv_dropdown_get_selected(ui_RoleDropdown));
    draft.values = valueDraftParams;
    draft.configs = configDraftParams;

    std::string error;
    if (!validateTopLevelDraft(draft, error)) {
        splashMessage(error.c_str());
        return;
    }

    browserState.setLibraryDraft(draft, isNewEntity);
    if (!router.saveLibraryDraft(draft, originalUid, isNewEntity, error)) {
        splashMessage(error.c_str());
        return;
    }

    splashMessage("Device schema saved to storage.");
    router.showLibrary();
}

void LibraryEditorGui::init()
{
    if (initialized) {
        return;
    }

    build();
    initialized = true;
}

void LibraryEditorGui::showEditor()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    refresh();
    hideParamEditor();
    hideKeyboard();
    lv_obj_clear_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);
}

void LibraryEditorGui::hideEditor()
{
    if (!initialized || !ui_Widget) {
        return;
    }

    hideParamEditor();
    hideKeyboard();
    lv_obj_add_flag(ui_Widget, LV_OBJ_FLAG_HIDDEN);
}

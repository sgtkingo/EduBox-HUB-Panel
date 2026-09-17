/**
 * @file device_catalog_browser.cpp
 * @brief Shared catalog browser model and formatting helpers for catalog-based GUIs.
 */

#include "device_catalog_browser.hpp"

#include "./images/ui_images.h"
#include "lvgl_storage_fs.hpp"
#include "../config.hpp"
#include "../managers/storage_manager.hpp"
#include "../managers/app_config_manager.hpp"

namespace
{
constexpr uint32_t COLOR_TEXT = 0x1C1F23;
constexpr uint32_t COLOR_MUTED = 0x6C7680;
constexpr uint32_t COLOR_PANEL = 0xFFFFFF;
constexpr uint32_t COLOR_BORDER = 0xD8DEE8;
constexpr uint32_t COLOR_VALUE = 0x2F80ED;
constexpr uint32_t COLOR_CONFIG = 0xD96464;
constexpr uint32_t COLOR_PIN = 0xE9F2FF;
constexpr uint32_t COLOR_ALLOWED_PIN = 0xEEF7ED;
constexpr uint16_t LVGL_ZOOM_BASE = 256;
constexpr lv_coord_t DEVICE_DETAIL_PICTURE_SIZE = 112;

std::string pictureSourcePath;

void freeImagePreview(lv_img_dsc_t *preview)
{
    if (preview) {
        lv_img_buf_free(preview);
    }
}

void releaseOwnedPreviewOnDelete(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE) {
        return;
    }

    freeImagePreview(static_cast<lv_img_dsc_t *>(lv_event_get_user_data(e)));
}

void centerImage(lv_obj_t *image)
{
    if (!image) {
        return;
    }

    lv_img_set_angle(image, 0);
    lv_img_set_offset_x(image, 0);
    lv_img_set_offset_y(image, 0);
    lv_img_set_zoom(image, LVGL_ZOOM_BASE);
    lv_obj_center(image);
}

lv_img_dsc_t *buildFilePreview(const void *src, lv_coord_t targetWidth, lv_coord_t targetHeight)
{
    if (!src || targetWidth <= 0 || targetHeight <= 0) {
        return nullptr;
    }

    lv_img_decoder_dsc_t decoder;
    debugLogMessage("DeviceCatalogBrowserRenderer::buildFilePreview", "picture decode start", "path=%s", static_cast<const char *>(src));
    lv_memset_00(&decoder, sizeof(decoder));
    if (lv_img_decoder_open(&decoder, src, lv_color_black(), 0) != LV_RES_OK ||
        !decoder.img_data ||
        decoder.header.w == 0 ||
        decoder.header.h == 0) {
        debugLogMessage(
            "DeviceCatalogBrowserRenderer::buildFilePreview",
            "picture decode failed",
            "size=%dx%d",
            static_cast<int>(targetWidth),
            static_cast<int>(targetHeight)
        );
        lv_img_decoder_close(&decoder);
        return nullptr;
    }

    lv_img_dsc_t sourceImage;
    lv_memset_00(&sourceImage, sizeof(sourceImage));
    sourceImage.header = decoder.header;
    sourceImage.data = decoder.img_data;
    sourceImage.data_size = lv_img_buf_get_img_size(
        static_cast<lv_coord_t>(decoder.header.w),
        static_cast<lv_coord_t>(decoder.header.h),
        decoder.header.cf
    );

    lv_img_dsc_t *preview = lv_img_buf_alloc(targetWidth, targetHeight, LV_IMG_CF_TRUE_COLOR_ALPHA);
    if (!preview) {
        debugLogMessage(
            "DeviceCatalogBrowserRenderer::buildFilePreview",
            "preview alloc failed",
            "size=%dx%d",
            static_cast<int>(targetWidth),
            static_cast<int>(targetHeight)
        );
        lv_img_decoder_close(&decoder);
        return nullptr;
    }

    lv_memset_00(const_cast<uint8_t *>(preview->data), preview->data_size);

    const double sourceWidth = static_cast<double>(decoder.header.w);
    const double sourceHeight = static_cast<double>(decoder.header.h);
    const double scaleX = static_cast<double>(targetWidth) / sourceWidth;
    const double scaleY = static_cast<double>(targetHeight) / sourceHeight;
    const double coverScale = scaleX > scaleY ? scaleX : scaleY;
    const double sampledWidth = static_cast<double>(targetWidth) / coverScale;
    const double sampledHeight = static_cast<double>(targetHeight) / coverScale;
    const double offsetX = (sourceWidth - sampledWidth) * 0.5;
    const double offsetY = (sourceHeight - sampledHeight) * 0.5;

    for (lv_coord_t y = 0; y < targetHeight; ++y) {
        const double srcYf = offsetY + ((static_cast<double>(y) + 0.5) * sampledHeight / static_cast<double>(targetHeight));
        lv_coord_t srcY = static_cast<lv_coord_t>(srcYf);
        if (srcY < 0) {
            srcY = 0;
        } else if (srcY >= static_cast<lv_coord_t>(decoder.header.h)) {
            srcY = static_cast<lv_coord_t>(decoder.header.h - 1);
        }

        for (lv_coord_t x = 0; x < targetWidth; ++x) {
            const double srcXf = offsetX + ((static_cast<double>(x) + 0.5) * sampledWidth / static_cast<double>(targetWidth));
            lv_coord_t srcX = static_cast<lv_coord_t>(srcXf);
            if (srcX < 0) {
                srcX = 0;
            } else if (srcX >= static_cast<lv_coord_t>(decoder.header.w)) {
                srcX = static_cast<lv_coord_t>(decoder.header.w - 1);
            }

            const lv_color_t color = lv_img_buf_get_px_color(&sourceImage, srcX, srcY, lv_color_black());
            const lv_opa_t alpha = lv_img_buf_get_px_alpha(&sourceImage, srcX, srcY);
            lv_img_buf_set_px_color(preview, x, y, color);
            lv_img_buf_set_px_alpha(preview, x, y, alpha);
        }
    }

    lv_img_decoder_close(&decoder);
    return preview;
}

std::string buildAllowedPinsText(const BaseDevice *device)
{
    if (!device) {
        return "Any";
    }

    std::string allowedPinsText;
    const auto allowedPins = device->getAllowedPinsList();
    if (allowedPins.empty()) {
        return "Any";
    }

    for (size_t i = 0; i < allowedPins.size(); ++i) {
        if (i > 0) {
            allowedPinsText += ", ";
        }
        allowedPinsText += std::to_string(allowedPins[i]);
    }

    return allowedPinsText;
}

std::string buildPinsText(const BaseDevice *device)
{
    if (!device || device->getPinDefinitions().empty()) {
        return "None";
    }

    std::string pinsText;
    for (const auto &pin : device->getPinDefinitions()) {
        if (!pinsText.empty()) {
            pinsText += ", ";
        }
        pinsText += pin;
    }
    return pinsText;
}

std::string buildValuesText(const BaseDevice *device)
{
    if (!device) {
        return "- none\n";
    }

    std::string valuesText;
    const auto valueKeys = device->getValuesKeys();
    if (valueKeys.empty()) {
        return "- none\n";
    }

    for (const auto &key : valueKeys) {
        valuesText += "- " + key + " [" + device->getValueUnits(key) + "]\n";
    }

    return valuesText;
}

std::string buildConfigsText(const BaseDevice *device)
{
    if (!device) {
        return "- none\n";
    }

    std::string configsText;
    const auto configKeys = device->getConfigsKeys();
    if (configKeys.empty()) {
        return "- none\n";
    }

    for (const auto &key : configKeys) {
        configsText += "- " + key + "\n";
    }

    return configsText;
}

std::string ellipsize(const std::string &text, size_t maxLength)
{
    if (text.size() <= maxLength) {
        return text;
    }
    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }
    return text.substr(0, maxLength - 3) + "...";
}

uint32_t roleColor(DeviceRole role)
{
    switch (role) {
    case DeviceRole::ACTUATOR:
        return 0xF2A23A;
    case DeviceRole::HYBRID:
        return 0x7B61FF;
    case DeviceRole::SENSOR:
    default:
        return 0x2EAD5F;
    }
}

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

lv_obj_t *createLabel(lv_obj_t *parent,
                      const std::string &text,
                      lv_coord_t x,
                      lv_coord_t y,
                      lv_coord_t width,
                      const lv_font_t *font,
                      uint32_t color = COLOR_TEXT)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text.c_str());
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    return label;
}

lv_obj_t *createBadge(lv_obj_t *parent,
                      const std::string &text,
                      lv_coord_t x,
                      lv_coord_t y,
                      lv_coord_t width,
                      uint32_t color,
                      uint32_t textColor = 0xFFFFFF)
{
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, width, 24);
    lv_obj_set_pos(badge, x, y);
    lv_obj_set_style_radius(badge, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(badge, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(badge);
    lv_label_set_text(label, text.c_str());
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(textColor), LV_PART_MAIN | LV_STATE_DEFAULT);
    return badge;
}

lv_coord_t createChipRow(lv_obj_t *parent,
                         const std::vector<std::string> &items,
                         lv_coord_t x,
                         lv_coord_t y,
                         lv_coord_t maxWidth,
                         uint32_t color,
                         uint32_t textColor = COLOR_TEXT)
{
    if (items.empty()) {
        createBadge(parent, "None", x, y, 64, 0xEEF0F3, COLOR_MUTED);
        return y + 30;
    }

    lv_coord_t cursorX = x;
    lv_coord_t cursorY = y;
    for (const auto &item : items) {
        const lv_coord_t width = static_cast<lv_coord_t>(item.size() * 8 + 24);
        const lv_coord_t chipWidth = width < 48 ? 48 : (width > 132 ? 132 : width);
        if (cursorX + chipWidth > x + maxWidth) {
            cursorX = x;
            cursorY += 30;
        }

        createBadge(parent, item, cursorX, cursorY, chipWidth, color, textColor);
        cursorX += chipWidth + 6;
    }
    return cursorY + 30;
}

std::vector<std::string> buildAllowedPinItems(const BaseDevice *device)
{
    std::vector<std::string> items;
    if (!device) {
        return items;
    }

    const auto allowedPins = device->getAllowedPinsList();
    for (int pin : allowedPins) {
        items.push_back(std::to_string(pin));
    }
    if (items.empty()) {
        items.push_back("Any");
    }
    return items;
}

std::vector<std::string> buildValueItems(const BaseDevice *device)
{
    std::vector<std::string> items;
    if (!device) {
        return items;
    }

    const auto values = device->getValues();
    for (const auto &key : device->getValuesKeys()) {
        auto it = values.find(key);
        std::string item = key;
        if (it != values.end() && !it->second.Unit.empty()) {
            item += " " + it->second.Unit;
        }
        if (it != values.end() && it->second.Access == DeviceParamAccess::WRITE) {
            item += " OUT";
        }
        items.push_back(item);
    }
    return items;
}

std::vector<std::string> buildConfigItems(const BaseDevice *device)
{
    std::vector<std::string> items;
    if (!device) {
        return items;
    }

    const auto configs = device->getConfigs();
    for (const auto &key : device->getConfigsKeys()) {
        auto it = configs.find(key);
        std::string item = key;
        if (it != configs.end() && !it->second.Unit.empty()) {
            item += " " + it->second.Unit;
        }
        items.push_back(item);
    }
    return items;
}

bool isStoragePicturePath(const std::string &path)
{
    return path.rfind(STORAGE_DEVICE_PICTURE_DIR, 0) == 0;
}

std::string findDevicePicturePath(const BaseDevice *device)
{
    if (!AppConfigManager::useDevicePictures()) return "";
    if (!device || (!storageManager().isAvailable() && !storageManager().init())) {
        return "";
    }

    const std::string storedPicture = device->getPicture();
    if (storedPicture.empty() || storedPicture == "placeholder:device") {
        return "";
    }

    if (isStoragePicturePath(storedPicture) && storageManager().exists(storedPicture)) {
        return storedPicture;
    }

    const std::string candidate = std::string(STORAGE_DEVICE_PICTURE_DIR) + "/" + storedPicture;
    if (storageManager().exists(candidate)) {
        return candidate;
    }

    debugLogMessage(
        "DeviceCatalogBrowser::findDevicePicturePath",
        "picture missing",
        "uid=%s picture=%s candidate=%s",
        device->UID.c_str(),
        storedPicture.c_str(),
        candidate.c_str()
    );

    return "";
}

void renderPicture(lv_obj_t *parent, const BaseDevice *device)
{
    lv_obj_t *picture = lv_obj_create(parent);
    lv_obj_set_size(picture, 112, 112);
    lv_obj_set_pos(picture, 280, 52);
    lv_obj_clear_flag(picture, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(picture, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(picture, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(picture, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(picture, lv_color_hex(COLOR_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(picture, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(picture, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *image = lv_img_create(picture);
    lv_obj_t *canvas = lv_canvas_create(picture);
    lv_obj_remove_style_all(canvas);
    lv_obj_set_size(canvas, DEVICE_DETAIL_PICTURE_SIZE, DEVICE_DETAIL_PICTURE_SIZE);
    lv_obj_center(canvas);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
    const std::string imagePath = findDevicePicturePath(device);
    const bool pictureConfigured = AppConfigManager::useDevicePictures() && device && !device->getPicture().empty() && device->getPicture() != "placeholder:device";
    if (imagePath.empty()) {
        lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(image, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(image, placeholderImageForRole(device ? device->getRole() : DeviceRole::SENSOR));
        lv_img_set_zoom(image, LVGL_ZOOM_BASE);
        lv_obj_center(image);

        if (pictureConfigured) {
            lv_obj_t *warningBadge = lv_obj_create(picture);
            lv_obj_remove_style_all(warningBadge);
            lv_obj_set_size(warningBadge, 24, 24);
            lv_obj_align(warningBadge, LV_ALIGN_TOP_RIGHT, -6, 6);
            lv_obj_set_style_radius(warningBadge, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(warningBadge, lv_color_hex(0xF28C28), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(warningBadge, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_t *warningText = lv_label_create(warningBadge);
            lv_label_set_text(warningText, "!");
            lv_obj_set_style_text_color(warningText, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(warningText, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_center(warningText);
        }
        return;
    }

    ensureLvglStorageFsRegistered();
    pictureSourcePath = "S:" + imagePath;
    DeviceCatalogBrowserRenderer::applyImagePreview(
        picture,
        image,
        canvas,
        pictureSourcePath.c_str(),
        DEVICE_DETAIL_PICTURE_SIZE,
        DEVICE_DETAIL_PICTURE_SIZE
    );
}

void createSectionTitle(lv_obj_t *parent, const std::string &title, lv_coord_t x, lv_coord_t y, uint32_t color)
{
    createLabel(parent, title, x, y, 360, &lv_font_montserrat_14, color);
}
}

DeviceCatalogBrowserModel::DeviceCatalogBrowserModel(DeviceCatalog &deviceCatalog)
    : deviceCatalog(deviceCatalog)
{
}

const std::vector<BaseDevice *> &DeviceCatalogBrowserModel::getDevices() const
{
    return deviceCatalog.getDevices();
}

BaseDevice *DeviceCatalogBrowserModel::getSelectedDevice() const
{
    const auto &devices = getDevices();
    if (devices.empty() || selectedDeviceIndex < 0 || selectedDeviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }

    return devices[selectedDeviceIndex];
}

void DeviceCatalogBrowserModel::setSelectedDeviceIndex(int index)
{
    selectedDeviceIndex = index;
}

std::string DeviceCatalogBrowserFormatter::buildDeviceListLabel(const BaseDevice *device)
{
    if (!device) {
        return "Unknown";
    }

    return device->getName() + " [" + device->getRoleLabel() + "]";
}

std::string DeviceCatalogBrowserFormatter::buildSelectionInfoText(const BaseDevice *device)
{
    if (!device) {
        return "No device information available.";
    }

    std::string info = device->getDescription();
    if (info.empty()) {
        info = "No description available.";
    }

    info += "\nRole: " + device->getRoleLabel();
    info += "\nPicture: " + device->getPicture();
    info += "\nPins: " + buildPinsText(device);
    info += "\n\nAllowed Pins: " + buildAllowedPinsText(device);
    return info;
}

std::string DeviceCatalogBrowserFormatter::buildSelectionSpecsText(const BaseDevice *device)
{
    if (!device) {
        return "";
    }

    std::string specs = "Values:\n";
    specs += buildValuesText(device);
    specs += "\nConfigs:\n";
    specs += buildConfigsText(device);
    return specs;
}

std::string DeviceCatalogBrowserFormatter::buildLibraryDetailText(const BaseDevice *device)
{
    if (!device) {
        return "No entity selected.";
    }

    std::string detail = "Entity\n";
    detail += device->getName() + "\n\n";
    detail += "Role:\n" + device->getRoleLabel() + "\n\n";
    detail += "Picture:\n" + device->getPicture() + "\n\n";
    detail += "Description:\n" + device->getDescription() + "\n\n";
    detail += "Pins:\n" + buildPinsText(device) + "\n\n";
    detail += "Allowed Pins:\n" + buildAllowedPinsText(device) + "\n\n";
    detail += "Values:\n";
    detail += buildValuesText(device);
    detail += "\nConfigs:\n";
    detail += buildConfigsText(device);
    return detail;
}

void DeviceCatalogBrowserRenderer::styleDeviceListButton(lv_obj_t *button)
{
    if (!button) {
        return;
    }

    lv_obj_set_height(button, 58);
    lv_obj_set_style_pad_top(button, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(button, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(button, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label) {
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    }
}

void DeviceCatalogBrowserRenderer::applyImagePreview(lv_obj_t *frame,
                                                     lv_obj_t *image,
                                                     lv_obj_t *canvas,
                                                     const void *src,
                                                     lv_coord_t targetWidth,
                                                     lv_coord_t targetHeight,
                                                     lv_img_dsc_t **ownedPreview)
{
    if (!frame || !image || !canvas || !src) {
        return;
    }

    if (lv_img_src_get_type(src) != LV_IMG_SRC_FILE) {
        if (ownedPreview && *ownedPreview) {
            freeImagePreview(*ownedPreview);
            *ownedPreview = nullptr;
        }
        lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(image, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(image, src);
        centerImage(image);
        return;
    }

    if (targetWidth <= 0 || targetHeight <= 0) {
        debugLogMessage(
            "DeviceCatalogBrowserRenderer::applyImagePreview",
            "invalid target size",
            "size=%dx%d",
            static_cast<int>(targetWidth),
            static_cast<int>(targetHeight)
        );
        return;
    }

    lv_img_dsc_t *preview = buildFilePreview(src, targetWidth, targetHeight);
    if (!preview) {
        if (ownedPreview && *ownedPreview) {
            freeImagePreview(*ownedPreview);
            *ownedPreview = nullptr;
        }
        lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(image, LV_OBJ_FLAG_HIDDEN);
        lv_img_set_src(image, &ui_img_placeholder_sensor);
        centerImage(image);
        return;
    }

    if (ownedPreview) {
        if (*ownedPreview) {
            freeImagePreview(*ownedPreview);
        }
        *ownedPreview = preview;
    } else {
        lv_obj_add_event_cb(canvas, releaseOwnedPreviewOnDelete, LV_EVENT_DELETE, preview);
    }

    lv_canvas_set_buffer(
        canvas,
        const_cast<uint8_t *>(preview->data),
        targetWidth,
        targetHeight,
        preview->header.cf
    );
    lv_obj_set_size(canvas, targetWidth, targetHeight);
    lv_obj_center(canvas);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    debugLogMessage(
        "DeviceCatalogBrowserRenderer::applyImagePreview",
        "preview ready",
        "size=%dx%d",
        static_cast<int>(preview->header.w),
        static_cast<int>(preview->header.h)
    );
}

void DeviceCatalogBrowserRenderer::renderDeviceDetail(lv_obj_t *panel, const BaseDevice *device, const std::string &footerText)
{
    if (!panel) {
        return;
    }

    lv_obj_clean(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(COLOR_PANEL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);

    if (!device) {
        createLabel(panel, "No device selected", 14, 14, 360, &lv_font_montserrat_20);
        createLabel(panel, "Choose a device from the list.", 14, 48, 360, &lv_font_montserrat_14, COLOR_MUTED);
        return;
    }

    createLabel(panel, ellipsize(device->getName(), 30), 14, 14, 370, &lv_font_montserrat_20);
    createLabel(panel, device->UID, 14, 42, 252, &lv_font_montserrat_12, COLOR_MUTED);
    renderPicture(panel, device);
    createBadge(panel, device->getRoleLabel(), 14, 66, 104, roleColor(device->getRole()));

    const std::string description = device->getDescription().empty() ? "No description available." : device->getDescription();
    lv_obj_t *descriptionLabel = createLabel(panel, ellipsize(description, 75), 14, 96, 252, &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_height(descriptionLabel, 40);
    lv_label_set_long_mode(descriptionLabel, LV_LABEL_LONG_DOT);

    lv_coord_t y = 176;
    if (!footerText.empty()) {
        createBadge(panel, footerText, 14, y, 116, 0xFFF8D6, COLOR_TEXT);
        y += 34;
    }

    createSectionTitle(panel, "Pins", 14, y, COLOR_TEXT);
    y = createChipRow(panel, device->getPinDefinitions(), 14, y + 22, 370, COLOR_PIN, COLOR_TEXT);

    createSectionTitle(panel, "Allowed Pins", 14, y + 4, COLOR_TEXT);
    y = createChipRow(panel, buildAllowedPinItems(device), 14, y + 26, 370, COLOR_ALLOWED_PIN, COLOR_TEXT);

    createSectionTitle(panel, "Values", 14, y + 4, COLOR_VALUE);
    y = createChipRow(panel, buildValueItems(device), 14, y + 26, 370, COLOR_VALUE, 0xFFFFFF);

    createSectionTitle(panel, "Configs", 14, y + 4, COLOR_CONFIG);
    y = createChipRow(panel, buildConfigItems(device), 14, y + 26, 370, COLOR_CONFIG, 0xFFFFFF);

}

DeviceCatalogBrowserLayout DeviceCatalogBrowserLayoutFactory::createLayout(const DeviceCatalogBrowserLayoutConfig &config)
{
    DeviceCatalogBrowserLayout layout;

    layout.root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(layout.root);
    lv_obj_set_size(layout.root, 760, 440);
    lv_obj_set_align(layout.root, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(layout.root, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(layout.root, lv_color_hex(config.backgroundColor), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(layout.root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(layout.root, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(layout.root, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(layout.root, LV_OBJ_FLAG_HIDDEN);

    layout.titleLabel = lv_label_create(layout.root);
    lv_label_set_text(layout.titleLabel, config.title ? config.title : "");
    lv_obj_align(layout.titleLabel, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_font(layout.titleLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    layout.primaryList = lv_list_create(layout.root);
    lv_obj_set_size(layout.primaryList, 250, 285);
    lv_obj_set_pos(layout.primaryList, 10, 50);

    layout.detailPanel = lv_obj_create(layout.root);
    if (config.includeSecondaryList) {
        lv_obj_set_size(layout.detailPanel, 300, 285);
        lv_obj_set_pos(layout.detailPanel, 230, 50);
    } else {
        lv_obj_set_size(layout.detailPanel, 410, 320);
        lv_obj_set_pos(layout.detailPanel, 270, 50);
    }

    layout.detailLabel = lv_label_create(layout.detailPanel);
    lv_obj_set_width(layout.detailLabel, config.includeSecondaryList ? 270 : lv_pct(100));
    lv_label_set_long_mode(layout.detailLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(layout.detailLabel, LV_ALIGN_TOP_LEFT, 10, 10);

    if (config.includeSecondaryList) {
        layout.secondaryList = lv_list_create(layout.root);
        lv_obj_set_size(layout.secondaryList, 210, 285);
        lv_obj_set_pos(layout.secondaryList, 540, 50);
    }

    return layout;
}

lv_obj_t *DeviceCatalogBrowserLayoutFactory::createFooterButton(lv_obj_t *parent,
                                                                const char *text,
                                                                lv_align_t align,
                                                                lv_coord_t xOffset,
                                                                lv_coord_t yOffset)
{
    if (!parent) {
        return nullptr;
    }

    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 120, 36);
    lv_obj_align(button, align, xOffset, yOffset);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text ? text : "");
    lv_obj_center(label);
    return button;
}

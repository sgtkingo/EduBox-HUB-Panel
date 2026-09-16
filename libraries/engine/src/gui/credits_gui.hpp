/**
 * @file credits_gui.hpp
 * @brief Header file for the CreditsGui class
 *
 * This header file declares and defines the CreditsGui class responsible for
 * displaying the static About page.
 *
 * @copyright 2025 MTA
 * @author Ondřej Wrubel
 */

#ifndef CREDITS_GUI_HPP
#define CREDITS_GUI_HPP

#include "lvgl.h"

#include "gui_router.hpp"
#include "images/ui_images.h"

/**
 * @class CreditsGui
 * @brief Handles the creation, display, and destruction of the static credits page.
 *
 * This class is optimized for memory efficiency while maintaining visual styling:
 * - Uses Flex Layout to organize sections without manual positioning.
 * - Uses multiple static labels to apply different styles (Title, Body, Footer).
 * - Uses zero-copy static text (Flash memory) rather than Heap allocation.
 */
class CreditsGui
{
private:
    GuiRouter &router;
    bool initialized = false;                       ///< Initialization state flag
    lv_obj_t *ui_CreditsScreen = nullptr;           ///< Main screen/container widget
    lv_obj_t *ui_btnBackGroup = nullptr;            ///< Group container for back button
    lv_obj_t *ui_btnBack = nullptr;                 ///< Back to menu button
    lv_obj_t *ui_btnBackLabel = nullptr;            ///< Label for back button
    lv_obj_t *ui_btnBackCornerBottomLeft = nullptr; ///< Decorative corner for back button
    lv_obj_t *ui_btnBackCornerTopRight = nullptr;   ///< Decorative corner for back button

    // Persistent styles for the different text sections
    lv_style_t style_title;
    lv_style_t style_section;
    lv_style_t style_footer;

    // --- Static Text Assets (Stored in Flash/RODATA) ---
    static constexpr const char *txt_title = "About EduBox HUB Panel";

    static constexpr const char *txt_intro =
        "EduBox HUB Panel is a touchscreen interface for VSCP devices. It visualizes live sensor data, records measurements and reviews DataBundles.\n"
        "Developed as part of the EduBox HUB ecosystem by MTA (www.m-ta.cz).";

    static constexpr const char *txt_section_authors =
        "Authors:\n"
        "Ing. Jiri Konecny\n"
        "Ondrej Wrubel";

    static constexpr const char *txt_section_icons =
        "Icon Policy:\n"
        "Bluetooth Icon by Icons8 (icons8.com)\n"
        "General Icons by Flaticon (flaticon.com)";

    static constexpr const char *txt_footer =
        "Full icon license terms are available from the providers above.\n"
        "All trademarks and registered trademarks are the property of their respective owners.";

    /**
     * @brief Helper to create a standard static label with wrapping
     * @param text Pointer to the static string constant
     * @return The created label object
     */
    lv_obj_t *create_static_label(const char *text)
    {
        lv_obj_t *lbl = lv_label_create(ui_CreditsScreen);
        lv_label_set_text_static(lbl, text);
        lv_obj_set_width(lbl, lv_pct(90));               // Fill width
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP); // Wrap text
        return lbl;
    }

    void create_corner_button() {
        lv_obj_t *ui_btnBackGroup = lv_obj_create(ui_CreditsScreen);
        lv_obj_remove_style_all(ui_btnBackGroup);
        lv_obj_set_width(ui_btnBackGroup, 100);
        lv_obj_set_height(ui_btnBackGroup, 40);
        lv_obj_add_flag(ui_btnBackGroup, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(ui_btnBackGroup, LV_ALIGN_TOP_LEFT, -15, -15);
        lv_obj_clear_flag(ui_btnBackGroup, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE); /// Flags

        // Back button for returning to menu
        lv_obj_t *ui_btnBack = lv_btn_create(ui_btnBackGroup);
        lv_obj_set_width(ui_btnBack, 100);
        lv_obj_set_height(ui_btnBack, 40);
        lv_obj_set_align(ui_btnBack, LV_ALIGN_CENTER);
        lv_obj_add_event_cb(ui_btnBack, [](lv_event_t *e)
                            {
        auto self = static_cast<CreditsGui*>(lv_event_get_user_data(e));
        // // logMessage("Back button pressed - returning to menu\n");
        self->handleBackButtonClick(); }, LV_EVENT_CLICKED, this);

        lv_obj_t * ui_btnBackLabel = lv_label_create(ui_btnBack);
        lv_label_set_text(ui_btnBackLabel, "Back");
        lv_obj_center(ui_btnBackLabel);
        lv_obj_set_style_text_font(ui_btnBackLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /**
     * @brief Handle back button click event
     */
    void handleBackButtonClick()
    {
        hideCredits();
        router.showMainMenu();
    }

public:
    /**
     * @brief Constructor
     */
    explicit CreditsGui(GuiRouter &router) : router(router) {}

    /**
     * @brief Destructor
     */
    ~CreditsGui()
    {
        hideCredits();
    }

    /**
     * @brief Initialize the Credits GUI
     * Sets up styles and creates the screen layout.
     */
    void init()
    {
        if (initialized)
            return;

        lv_style_init(&style_title);
        lv_style_set_text_font(&style_title, &lv_font_montserrat_20);

        lv_style_init(&style_section);
        lv_style_set_text_font(&style_section, &lv_font_montserrat_14);

        lv_style_init(&style_footer);
        lv_style_set_text_color(&style_footer, lv_palette_main(LV_PALETTE_GREY));
        lv_style_set_text_font(&style_footer, &lv_font_montserrat_10);

        ui_CreditsScreen = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(ui_CreditsScreen);
        lv_obj_set_width(ui_CreditsScreen, 760);
        lv_obj_set_height(ui_CreditsScreen, 440);
        lv_obj_set_align(ui_CreditsScreen, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_CreditsScreen, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(ui_CreditsScreen, 15, 0);
        lv_obj_set_style_pad_row(ui_CreditsScreen, 15, 0); // Gap between text blocks
        lv_obj_set_style_radius(ui_CreditsScreen, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_CreditsScreen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_CreditsScreen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_CreditsScreen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_CreditsScreen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_CreditsScreen, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

        //Back Button Group
        create_corner_button();

        // Product identity stays in About; the main menu keeps the MTA logo.
        lv_obj_t *panelLogo = lv_img_create(ui_CreditsScreen);
        lv_img_set_src(panelLogo, &ui_img_edubox_hub_panel_logo);
        lv_obj_add_flag(panelLogo, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(panelLogo, LV_ALIGN_TOP_RIGHT, -18, 8);
        lv_obj_clear_flag(panelLogo, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        // Title
        lv_obj_t *lbl_title = create_static_label(txt_title);
        lv_obj_set_style_pad_top(lbl_title, 45, 0);
        lv_obj_add_style(lbl_title, &style_title, 0);

        // Intro
        create_static_label(txt_intro);

        // Divider Line
        lv_obj_t *line = lv_obj_create(ui_CreditsScreen);
        lv_obj_set_size(line, lv_pct(100), 2);
        lv_obj_set_style_bg_color(line, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

        // Authors Section
        lv_obj_t *lbl_authors = create_static_label(txt_section_authors);
        lv_obj_add_style(lbl_authors, &style_section, 0);

        // Icons Section
        lv_obj_t *lbl_icons = create_static_label(txt_section_icons);
        lv_obj_add_style(lbl_icons, &style_section, 0);

        // Footer
        lv_obj_t *lbl_footer = create_static_label(txt_footer);
        lv_obj_add_style(lbl_footer, &style_footer, 0);

        initialized = true;
    }

    /**
     * @brief Check if the Credits GUI has been initialized
     * @return True if initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Show the Credits screen
     */
    void showCredits()
    {
        init();
    }

    /**
     * @brief Hide the Credits GUI and clean up resources
     */
    void hideCredits()
    {
        if (!initialized)
            return;

        if (ui_CreditsScreen)
        {
            lv_obj_del(ui_CreditsScreen);
            ui_CreditsScreen = nullptr;
        }

        // Clean up styles to prevent memory leaks if styles allocate anything
        lv_style_reset(&style_title);
        lv_style_reset(&style_section);
        lv_style_reset(&style_footer);

        initialized = false;
    }
};

#endif // CREDITS_GUI_HPP

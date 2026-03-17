/**
 * SmartWatch Theme System — Implementation
 */

#include "theme.h"

// ============================================================
// Style storage (static lifetime)
// ============================================================
lv_style_t style_screen;
lv_style_t style_card;
lv_style_t style_card_pressed;
lv_style_t style_btn_primary;
lv_style_t style_btn_primary_pressed;
lv_style_t style_btn_ghost;
lv_style_t style_btn_ghost_pressed;
lv_style_t style_label_time;
lv_style_t style_label_heading;
lv_style_t style_label_body;
lv_style_t style_label_caption;
lv_style_t style_status_bar;
lv_style_t style_list_item;

// Transition for press feedback
static lv_style_transition_dsc_t _press_transition;
static const lv_style_prop_t _press_props[] = {
    LV_STYLE_BG_COLOR,
    LV_STYLE_TRANSFORM_ZOOM,
    LV_STYLE_PROP_INV  // sentinel
};

void theme_init(void) {
    // --- Transition for press states ---
    lv_style_transition_dsc_init(&_press_transition, _press_props, lv_anim_path_ease_out, 150, 0, NULL);

    // === Screen ===
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, COLOR_BG_PRIMARY);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_radius(&style_screen, 0);

    // === Card (default state) ===
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_BG_CARD);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, RADIUS_CARD);
    lv_style_set_pad_all(&style_card, SPACE_MD);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, COLOR_DIVIDER);
    lv_style_set_border_opa(&style_card, LV_OPA_60);
    lv_style_set_shadow_width(&style_card, 6);
    lv_style_set_shadow_color(&style_card, lv_color_black());
    lv_style_set_shadow_opa(&style_card, LV_OPA_30);
    lv_style_set_shadow_ofs_y(&style_card, 2);

    // === Card (pressed state) ===
    lv_style_init(&style_card_pressed);
    lv_style_set_bg_color(&style_card_pressed, COLOR_BG_ELEVATED);
    lv_style_set_transform_zoom(&style_card_pressed, 245);  // ~96% scale
    lv_style_set_transition(&style_card_pressed, &_press_transition);

    // === Primary Button (default) ===
    lv_style_init(&style_btn_primary);
    lv_style_set_bg_color(&style_btn_primary, COLOR_ACCENT);
    lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_primary, RADIUS_BUTTON);
    lv_style_set_min_height(&style_btn_primary, TOUCH_MIN_SIZE);
    lv_style_set_pad_hor(&style_btn_primary, SPACE_XL);
    lv_style_set_pad_ver(&style_btn_primary, SPACE_SM);
    lv_style_set_text_color(&style_btn_primary, lv_color_white());
    lv_style_set_text_font(&style_btn_primary, &lv_font_montserrat_20);
    lv_style_set_border_width(&style_btn_primary, 0);
    lv_style_set_shadow_width(&style_btn_primary, 8);
    lv_style_set_shadow_color(&style_btn_primary, COLOR_ACCENT);
    lv_style_set_shadow_opa(&style_btn_primary, LV_OPA_30);
    lv_style_set_shadow_ofs_y(&style_btn_primary, 3);

    // === Primary Button (pressed) ===
    lv_style_init(&style_btn_primary_pressed);
    lv_style_set_bg_color(&style_btn_primary_pressed, lv_color_darken(COLOR_ACCENT, LV_OPA_20));
    lv_style_set_transform_zoom(&style_btn_primary_pressed, 245);
    lv_style_set_shadow_opa(&style_btn_primary_pressed, LV_OPA_10);
    lv_style_set_transition(&style_btn_primary_pressed, &_press_transition);

    // === Ghost Button (default) ===
    lv_style_init(&style_btn_ghost);
    lv_style_set_bg_opa(&style_btn_ghost, LV_OPA_TRANSP);
    lv_style_set_radius(&style_btn_ghost, RADIUS_BUTTON);
    lv_style_set_min_height(&style_btn_ghost, TOUCH_MIN_SIZE);
    lv_style_set_pad_hor(&style_btn_ghost, SPACE_XL);
    lv_style_set_pad_ver(&style_btn_ghost, SPACE_SM);
    lv_style_set_border_width(&style_btn_ghost, 2);
    lv_style_set_border_color(&style_btn_ghost, COLOR_ACCENT);
    lv_style_set_text_color(&style_btn_ghost, COLOR_ACCENT);
    lv_style_set_text_font(&style_btn_ghost, &lv_font_montserrat_20);

    // === Ghost Button (pressed) ===
    lv_style_init(&style_btn_ghost_pressed);
    lv_style_set_bg_color(&style_btn_ghost_pressed, COLOR_ACCENT);
    lv_style_set_bg_opa(&style_btn_ghost_pressed, LV_OPA_20);
    lv_style_set_transform_zoom(&style_btn_ghost_pressed, 245);
    lv_style_set_transition(&style_btn_ghost_pressed, &_press_transition);

    // === Label: Time (48px) ===
    lv_style_init(&style_label_time);
    lv_style_set_text_font(&style_label_time, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_label_time, COLOR_TEXT_PRIMARY);

    // === Label: Heading (24px) ===
    lv_style_init(&style_label_heading);
    lv_style_set_text_font(&style_label_heading, &lv_font_montserrat_24);
    lv_style_set_text_color(&style_label_heading, COLOR_TEXT_PRIMARY);

    // === Label: Body (20px) ===
    lv_style_init(&style_label_body);
    lv_style_set_text_font(&style_label_body, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_label_body, COLOR_TEXT_PRIMARY);

    // === Label: Caption (14px) ===
    lv_style_init(&style_label_caption);
    lv_style_set_text_font(&style_label_caption, &lv_font_montserrat_14);
    lv_style_set_text_color(&style_label_caption, COLOR_TEXT_SECONDARY);

    // === Status Bar ===
    lv_style_init(&style_status_bar);
    lv_style_set_bg_color(&style_status_bar, lv_color_hex(0x0A0A0A));
    lv_style_set_bg_opa(&style_status_bar, LV_OPA_80);
    lv_style_set_pad_hor(&style_status_bar, SPACE_SM);
    lv_style_set_pad_ver(&style_status_bar, SPACE_XS);
    lv_style_set_border_width(&style_status_bar, 0);
    lv_style_set_radius(&style_status_bar, 0);

    // === List Item ===
    lv_style_init(&style_list_item);
    lv_style_set_bg_opa(&style_list_item, LV_OPA_TRANSP);
    lv_style_set_min_height(&style_list_item, 56);
    lv_style_set_pad_all(&style_list_item, SPACE_MD);
    lv_style_set_border_width(&style_list_item, 1);
    lv_style_set_border_color(&style_list_item, COLOR_DIVIDER);
    lv_style_set_border_side(&style_list_item, LV_BORDER_SIDE_BOTTOM);
}

// ============================================================
// Helper functions
// ============================================================

void theme_apply_screen(lv_obj_t *obj) {
    lv_obj_add_style(obj, &style_screen, 0);
}

lv_obj_t *theme_create_card(lv_obj_t *parent) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    return card;
}

lv_obj_t *theme_create_btn_primary(lv_obj_t *parent, const char *text) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &style_btn_primary, 0);
    lv_obj_add_style(btn, &style_btn_primary_pressed, LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

lv_obj_t *theme_create_btn_ghost(lv_obj_t *parent, const char *text) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_add_style(btn, &style_btn_ghost, 0);
    lv_obj_add_style(btn, &style_btn_ghost_pressed, LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

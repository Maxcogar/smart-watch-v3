/**
 * SmartWatch Theme System
 *
 * Defines the complete visual design system: colors, fonts, spacing, radii,
 * and LVGL style objects for a modern dark smartwatch aesthetic.
 */

#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

// ============================================================
// Color Palette
// ============================================================
#define COLOR_BG_PRIMARY     lv_color_hex(0x0D0D0D)
#define COLOR_BG_CARD        lv_color_hex(0x1A1A2E)
#define COLOR_BG_ELEVATED    lv_color_hex(0x252540)
#define COLOR_ACCENT         lv_color_hex(0x4FC3F7)
#define COLOR_ACCENT_GREEN   lv_color_hex(0x66BB6A)
#define COLOR_ACCENT_AMBER   lv_color_hex(0xFFA726)
#define COLOR_ACCENT_RED     lv_color_hex(0xEF5350)
#define COLOR_TEXT_PRIMARY   lv_color_hex(0xF5F5F5)
#define COLOR_TEXT_SECONDARY lv_color_hex(0x9E9E9E)
#define COLOR_TEXT_DISABLED  lv_color_hex(0x616161)
#define COLOR_DIVIDER        lv_color_hex(0x2A2A3E)

// ============================================================
// Spacing (4px base grid)
// ============================================================
#define SPACE_XS   4
#define SPACE_SM   8
#define SPACE_MD   12
#define SPACE_LG   16
#define SPACE_XL   24

// ============================================================
// Border Radii
// ============================================================
#define RADIUS_CARD    12
#define RADIUS_BUTTON  22
#define RADIUS_PILL    50
#define RADIUS_SMALL   8

// ============================================================
// Touch
// ============================================================
#define TOUCH_MIN_SIZE 44

// ============================================================
// Screen dimensions
// ============================================================
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// ============================================================
// Status bar
// ============================================================
#define STATUS_BAR_HEIGHT 28

// ============================================================
// Style accessors (initialized by theme_init)
// ============================================================

// Screens
extern lv_style_t style_screen;

// Cards
extern lv_style_t style_card;
extern lv_style_t style_card_pressed;

// Buttons
extern lv_style_t style_btn_primary;
extern lv_style_t style_btn_primary_pressed;
extern lv_style_t style_btn_ghost;
extern lv_style_t style_btn_ghost_pressed;

// Labels
extern lv_style_t style_label_time;
extern lv_style_t style_label_heading;
extern lv_style_t style_label_body;
extern lv_style_t style_label_caption;

// Status bar
extern lv_style_t style_status_bar;

// List items
extern lv_style_t style_list_item;

// ============================================================
// Functions
// ============================================================

/** Initialize all theme styles. Call once after lv_init(). */
void theme_init(void);

/** Apply screen base style to an object. */
void theme_apply_screen(lv_obj_t *obj);

/** Create a styled card container. Returns the card object. */
lv_obj_t *theme_create_card(lv_obj_t *parent);

/** Create a primary styled button with label. */
lv_obj_t *theme_create_btn_primary(lv_obj_t *parent, const char *text);

/** Create a ghost styled button with label. */
lv_obj_t *theme_create_btn_ghost(lv_obj_t *parent, const char *text);

#endif // THEME_H

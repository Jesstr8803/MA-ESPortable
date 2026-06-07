// screens_aux.cpp — the remaining screens (Library, List, Queue, Settings, Lock,
// Boot, Connecting, Charging) in LVGL 8.x. Same pattern as Now Playing:
// build once, return the screen object. Data binding / nav events are wired in
// the nav layer later; these establish the visual structure (ported from
// ui-prototype/). All use montserrat_14 (the default-enabled font) for now.
#include "lvgl.h"
#include "ui.h"
#include "ui_theme.h"

// ---- helpers -------------------------------------------------------------

static lv_obj_t *make_screen(void) {
    lv_obj_t *s = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s, UI_COL_BG, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    return s;
}

static lv_obj_t *make_header(lv_obj_t *parent, const char *text) {
    lv_obj_t *h = lv_label_create(parent);
    lv_label_set_text(h, text);
    lv_obj_set_style_text_color(h, UI_COL_FG, 0);
    lv_obj_set_style_text_font(h, &lv_font_montserrat_14, 0);
    lv_obj_align(h, LV_ALIGN_TOP_LEFT, 12, 10);
    return h;
}

// A scrollable list of touch-friendly rows (thumb + title + subtitle).
static lv_obj_t *make_list(lv_obj_t *parent) {
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, UI_W, UI_H - 40);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(list, UI_COL_BG, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    return list;
}

void ui_list_add_row(lv_obj_t *list, const char *title, const char *subtitle) {
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_size(row, UI_W, UI_ROW_H);
    lv_obj_set_style_bg_color(row, UI_COL_BG, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x15151A), 0);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *thumb = lv_obj_create(row);   // placeholder; real art = decoded JPEG
    lv_obj_set_size(thumb, 46, 46);
    lv_obj_align(thumb, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(thumb, 6, 0);
    lv_obj_set_style_bg_color(thumb, lv_color_hex(0x3A2B6E), 0);
    lv_obj_set_style_border_width(thumb, 0, 0);

    lv_obj_t *t = lv_label_create(row);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, UI_COL_FG, 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
    lv_obj_align(t, LV_ALIGN_LEFT_MID, 58, -9);

    lv_obj_t *s = lv_label_create(row);
    lv_label_set_text(s, subtitle ? subtitle : "");
    lv_obj_set_style_text_color(s, UI_COL_DIM, 0);
    lv_obj_set_style_text_font(s, &lv_font_montserrat_14, 0);
    lv_obj_align(s, LV_ALIGN_LEFT_MID, 58, 11);
}

// ---- Library: 2x2 tile menu (Artists/Albums top, Playlists/Queue bottom) ---

static lv_obj_t *make_tile(lv_obj_t *parent, const char *icon, const char *label) {
    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_set_size(tile, 96, 96);     // two per row within the 220px inner width
    lv_obj_set_style_bg_color(tile, UI_COL_CARD, 0);
    lv_obj_set_style_radius(tile, 14, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ic = lv_label_create(tile);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, UI_COL_ACCENT, 0);
    lv_obj_set_style_text_font(ic, &lv_font_montserrat_14, 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *l = lv_label_create(tile);
    lv_label_set_text(l, label);
    lv_obj_set_style_text_color(l, UI_COL_FG, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, 14);
    return tile;
}

lv_obj_t *screen_library_create(void) {
    lv_obj_t *s = make_screen();
    lv_obj_t *grid = lv_obj_create(s);
    lv_obj_set_size(grid, UI_W, UI_H);
    lv_obj_set_style_bg_color(grid, UI_COL_BG, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);   // don't lay tiles in a scroll row
    lv_obj_set_style_pad_all(grid, 10, 0);             // outer margin
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(grid, 12, 0);
    lv_obj_set_style_pad_column(grid, 12, 0);
    ui_attach_tile_cb(make_tile(grid, LV_SYMBOL_LIST, "Artists"), 0);
    ui_attach_tile_cb(make_tile(grid, LV_SYMBOL_AUDIO, "Albums"), 1);
    ui_attach_tile_cb(make_tile(grid, LV_SYMBOL_DIRECTORY, "Playlists"), 2);
    ui_attach_tile_cb(make_tile(grid, LV_SYMBOL_LIST, "Queue"), 3);
    return s;
}

lv_obj_t *screen_list_create(void) {
    lv_obj_t *s = make_screen();
    make_header(s, LV_SYMBOL_LEFT "  Albums");
    lv_obj_t *list = make_list(s);
    // sample rows (replaced by paged MA data at runtime)
    ui_list_add_row(list, "Hurry Up, We're Dreaming", "M83 - 2011");
    ui_list_add_row(list, "In Rainbows", "Radiohead - 2007");
    ui_list_add_row(list, "Random Access Memories", "Daft Punk - 2013");
    ui_list_add_row(list, "Currents", "Tame Impala - 2015");
    return s;
}

lv_obj_t *screen_queue_create(void) {
    lv_obj_t *s = make_screen();
    make_header(s, LV_SYMBOL_LEFT "  Up Next");
    lv_obj_t *list = make_list(s);
    ui_list_add_row(list, "Midnight City", "M83");
    ui_list_add_row(list, "Reunion", "M83");
    ui_list_add_row(list, "Nightcall", "Kavinsky");
    return s;
}

lv_obj_t *screen_settings_create(void) {
    lv_obj_t *s = make_screen();
    make_header(s, LV_SYMBOL_LEFT "  Settings");
    lv_obj_t *list = make_list(s);
    ui_list_add_row(list, "Brightness", "80%");
    ui_list_add_row(list, "Sleep timeout", "30 s");
    ui_list_add_row(list, "Audio sync delay", "0 ms");
    ui_list_add_row(list, "Calibrate remote", "");
    ui_list_add_row(list, "Wi-Fi", "HomeNet");
    ui_list_add_row(list, "Music Assistant", "Connected");
    ui_list_add_row(list, "Firmware", "v0.1.0");
    return s;
}

lv_obj_t *screen_lock_create(void) {
    lv_obj_t *s = make_screen();
    lv_obj_t *clock = lv_label_create(s);
    lv_label_set_text(clock, "9:41");
    lv_obj_set_style_text_color(clock, UI_COL_FG, 0);
    lv_obj_set_style_text_font(clock, &lv_font_montserrat_14, 0);
    lv_obj_align(clock, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *ring = lv_btn_create(s);   // press-and-hold to unlock
    lv_obj_set_size(ring, 60, 60);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_0, 0);
    lv_obj_set_style_border_color(ring, UI_COL_DIM2, 0);
    lv_obj_set_style_border_width(ring, 3, 0);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, 40);
    lv_obj_t *lockic = lv_label_create(ring);
    lv_label_set_text(lockic, LV_SYMBOL_POWER);
    lv_obj_set_style_text_color(lockic, UI_COL_DIM, 0);
    lv_obj_center(lockic);
    ui_attach_unlock_cb(ring);   // long-press the ring to unlock

    lv_obj_t *hint = lv_label_create(s);
    lv_label_set_text(hint, "lift wakes the screen - hold ring to unlock");
    lv_obj_set_style_text_color(hint, UI_COL_DIM2, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -16);
    return s;
}

lv_obj_t *screen_state_create(const char *big, const char *sub) {
    lv_obj_t *s = make_screen();
    lv_obj_t *l = lv_label_create(s);
    lv_label_set_text(l, big);
    lv_obj_set_style_text_color(l, UI_COL_FG, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, -10);
    if (sub) {
        lv_obj_t *sl = lv_label_create(s);
        lv_label_set_text(sl, sub);
        lv_obj_set_style_text_color(sl, UI_COL_DIM, 0);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_14, 0);
        lv_obj_align(sl, LV_ALIGN_CENTER, 0, 16);
    }
    return s;
}

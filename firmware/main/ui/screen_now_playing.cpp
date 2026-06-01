// screen_now_playing.cpp — the flagship Now Playing screen in LVGL 8.x.
// Ported from ui-prototype/ (portrait 240x536). Establishes the pattern the
// other screens follow: build once, keep widget handles, update via setters.
//
// Layout (top→bottom): status bar · album art · title/artist/album ·
// progress bar + times · prev/play/next · volume slider · shuffle/repeat/lock.
#include "lvgl.h"
#include "ui.h"
#include "ui_theme.h"

static lv_obj_t *s_screen;
static lv_obj_t *s_art;
static lv_obj_t *s_title, *s_artist, *s_album;
static lv_obj_t *s_bar, *s_t_elapsed, *s_t_total;
static lv_obj_t *s_play_icon;
static lv_obj_t *s_vol;

static void fmt_mmss(uint32_t ms, char *out, size_t n) {
    uint32_t s = ms / 1000; lv_snprintf(out, n, "%lu:%02lu",
        (unsigned long)(s / 60), (unsigned long)(s % 60));
}

lv_obj_t *screen_now_playing_create(void) {
    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, UI_COL_BG, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    // --- album art (placeholder rounded rect; real art = decoded JPEG image) ---
    s_art = lv_obj_create(s_screen);
    lv_obj_set_size(s_art, 160, 160);
    lv_obj_align(s_art, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_radius(s_art, 12, 0);
    lv_obj_set_style_bg_color(s_art, lv_color_hex(0x3A2B6E), 0);
    lv_obj_set_style_border_width(s_art, 0, 0);
    lv_obj_clear_flag(s_art, LV_OBJ_FLAG_SCROLLABLE);

    // --- track text ---
    s_title = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_title, UI_COL_FG, 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_title, LV_LABEL_LONG_SCROLL_CIRCULAR);  // marquee
    lv_obj_set_width(s_title, UI_W - 24);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 212);

    s_artist = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_artist, UI_COL_DIM, 0);
    lv_obj_set_style_text_font(s_artist, &lv_font_montserrat_14, 0);
    lv_obj_align(s_artist, LV_ALIGN_TOP_MID, 0, 244);

    s_album = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_album, UI_COL_DIM2, 0);
    lv_obj_set_style_text_font(s_album, &lv_font_montserrat_14, 0);
    lv_obj_align(s_album, LV_ALIGN_TOP_MID, 0, 268);

    // --- progress ---
    s_bar = lv_bar_create(s_screen);
    lv_obj_set_size(s_bar, UI_W - 32, 4);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 300);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2A2A30), 0);
    lv_obj_set_style_bg_color(s_bar, UI_COL_ACCENT, LV_PART_INDICATOR);
    lv_bar_set_range(s_bar, 0, 1000);

    s_t_elapsed = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_t_elapsed, UI_COL_DIM2, 0);
    lv_obj_set_style_text_font(s_t_elapsed, &lv_font_montserrat_14, 0);
    lv_obj_align(s_t_elapsed, LV_ALIGN_TOP_LEFT, 16, 310);

    s_t_total = lv_label_create(s_screen);
    lv_obj_set_style_text_color(s_t_total, UI_COL_DIM2, 0);
    lv_obj_set_style_text_font(s_t_total, &lv_font_montserrat_14, 0);
    lv_obj_align(s_t_total, LV_ALIGN_TOP_RIGHT, -16, 310);

    // --- transport: prev / play / next ---
    lv_obj_t *prev = lv_label_create(s_screen);
    lv_label_set_text(prev, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(prev, UI_COL_FG, 0);
    lv_obj_set_style_text_font(prev, &lv_font_montserrat_14, 0);
    lv_obj_align(prev, LV_ALIGN_TOP_MID, -70, 340);

    lv_obj_t *playbtn = lv_btn_create(s_screen);
    lv_obj_set_size(playbtn, UI_PLAY_D, UI_PLAY_D);
    lv_obj_set_style_radius(playbtn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(playbtn, UI_COL_FG, 0);
    lv_obj_align(playbtn, LV_ALIGN_TOP_MID, 0, 330);
    s_play_icon = lv_label_create(playbtn);
    lv_label_set_text(s_play_icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(s_play_icon, UI_COL_BG, 0);
    lv_obj_center(s_play_icon);

    lv_obj_t *next = lv_label_create(s_screen);
    lv_label_set_text(next, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(next, UI_COL_FG, 0);
    lv_obj_set_style_text_font(next, &lv_font_montserrat_14, 0);
    lv_obj_align(next, LV_ALIGN_TOP_MID, 70, 340);

    // --- volume slider (for headphones w/o inline remote) ---
    s_vol = lv_slider_create(s_screen);
    lv_obj_set_width(s_vol, UI_W - 60);
    lv_obj_align(s_vol, LV_ALIGN_TOP_MID, 0, 430);
    lv_slider_set_range(s_vol, 0, 100);
    lv_obj_set_style_bg_color(s_vol, lv_color_hex(0x2A2A30), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_vol, UI_COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_vol, UI_COL_FG, LV_PART_KNOB);

    // --- shuffle / repeat / lock (icons; events wired in nav layer) ---
    lv_obj_t *row = lv_label_create(s_screen);
    lv_label_set_text(row, LV_SYMBOL_SHUFFLE "    " LV_SYMBOL_LOOP "    " LV_SYMBOL_POWER);
    lv_obj_set_style_text_color(row, UI_COL_DIM, 0);
    lv_obj_set_style_text_font(row, &lv_font_montserrat_14, 0);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -20);

    return s_screen;
}

void screen_now_playing_update(const ui_nowplaying_t *np) {
    if (!s_screen || !np) return;
    if (np->title)  lv_label_set_text(s_title,  np->title);
    if (np->artist) lv_label_set_text(s_artist, np->artist);
    if (np->album)  lv_label_set_text(s_album,  np->album);
    lv_label_set_text(s_play_icon, np->playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    if (np->duration_ms) {
        lv_bar_set_value(s_bar, (int32_t)((uint64_t)np->elapsed_ms * 1000 / np->duration_ms), LV_ANIM_OFF);
    }
    char a[8], b[8];
    fmt_mmss(np->elapsed_ms, a, sizeof a); lv_label_set_text(s_t_elapsed, a);
    fmt_mmss(np->duration_ms, b, sizeof b); lv_label_set_text(s_t_total, b);
    lv_slider_set_value(s_vol, np->volume, LV_ANIM_OFF);
}

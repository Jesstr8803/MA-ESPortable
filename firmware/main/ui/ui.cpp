// ui.cpp — screen manager. Builds screens once, switches between them, routes
// now-playing updates. (Other screens are stubbed; now-playing is the model.)
#include "lvgl.h"
#include "ui.h"
#include "ui_theme.h"

// from screen_now_playing.cpp
lv_obj_t *screen_now_playing_create(void);
void      screen_now_playing_update(const ui_nowplaying_t *np);

static lv_obj_t   *s_screens[UI_SCREEN_LOCK + 1];
static ui_screen_t s_current = UI_SCREEN_BOOT;

// Minimal centered-label placeholder for screens not yet ported.
static lv_obj_t *make_stub(const char *text) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, UI_COL_BG, 0);
    lv_obj_t *l = lv_label_create(scr);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, UI_COL_DIM, 0);
    lv_obj_center(l);
    return scr;
}

void ui_init(void) {
    s_screens[UI_SCREEN_BOOT]        = make_stub("MA-ESPortable");
    s_screens[UI_SCREEN_CONNECTING]  = make_stub("Connecting" LV_SYMBOL_REFRESH);
    s_screens[UI_SCREEN_NOW_PLAYING] = screen_now_playing_create();
    s_screens[UI_SCREEN_LIBRARY]     = make_stub("Library");      // TODO: 2x2 tiles
    s_screens[UI_SCREEN_LIST]        = make_stub("List");         // TODO: paged list + lazy art
    s_screens[UI_SCREEN_QUEUE]       = make_stub("Up Next");      // TODO
    s_screens[UI_SCREEN_SETTINGS]    = make_stub("Settings");     // TODO
    s_screens[UI_SCREEN_CHARGING]    = make_stub(LV_SYMBOL_CHARGE);
    s_screens[UI_SCREEN_LOCK]        = make_stub(LV_SYMBOL_POWER "  hold to unlock");
    ui_show(UI_SCREEN_BOOT);
}

void ui_show(ui_screen_t screen) {
    if (screen > UI_SCREEN_LOCK || !s_screens[screen]) return;
    s_current = screen;
    lv_scr_load(s_screens[screen]);
}

ui_screen_t ui_current(void) { return s_current; }

void ui_update_nowplaying(const ui_nowplaying_t *np) {
    screen_now_playing_update(np);
}

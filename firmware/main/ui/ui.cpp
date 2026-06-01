// ui.cpp — screen manager. Builds screens once, switches between them, routes
// now-playing updates. (Other screens are stubbed; now-playing is the model.)
#include "lvgl.h"
#include "ui.h"
#include "ui_theme.h"

// from screen_now_playing.cpp
lv_obj_t *screen_now_playing_create(void);
void      screen_now_playing_update(const ui_nowplaying_t *np);
// from screens_aux.cpp
lv_obj_t *screen_library_create(void);
lv_obj_t *screen_list_create(void);
lv_obj_t *screen_queue_create(void);
lv_obj_t *screen_settings_create(void);
lv_obj_t *screen_lock_create(void);
lv_obj_t *screen_state_create(const char *big, const char *sub);

static lv_obj_t   *s_screens[UI_SCREEN_LOCK + 1];
static ui_screen_t s_current = UI_SCREEN_BOOT;

void ui_init(void) {
    s_screens[UI_SCREEN_BOOT]        = screen_state_create("MA-ESPortable", "v0.1.0");
    s_screens[UI_SCREEN_CONNECTING]  = screen_state_create("Connecting" LV_SYMBOL_REFRESH,
                                                           "Music Assistant");
    s_screens[UI_SCREEN_NOW_PLAYING] = screen_now_playing_create();
    s_screens[UI_SCREEN_LIBRARY]     = screen_library_create();
    s_screens[UI_SCREEN_LIST]        = screen_list_create();
    s_screens[UI_SCREEN_QUEUE]       = screen_queue_create();
    s_screens[UI_SCREEN_SETTINGS]    = screen_settings_create();
    s_screens[UI_SCREEN_CHARGING]    = screen_state_create(LV_SYMBOL_CHARGE "  72%", "Charging");
    s_screens[UI_SCREEN_LOCK]        = screen_lock_create();
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

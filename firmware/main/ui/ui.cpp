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

// --- simple navigation back-stack -----------------------------------------
static ui_screen_t s_stack[8];
static int         s_sp = 0;   // stack pointer (depth)

static void nav_push(ui_screen_t s) { if (s_sp < 8) s_stack[s_sp++] = s_current; ui_show(s); }
static void nav_back(void)          { if (s_sp > 0) ui_show(s_stack[--s_sp]); }

// Per-screen gesture handler: swipe up/down/left/right -> navigate.
// Attached to each screen object so it fires regardless of what's focused.
static void screen_gesture_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    switch (s_current) {
        case UI_SCREEN_NOW_PLAYING:
            if (dir == LV_DIR_TOP)    nav_push(UI_SCREEN_LIBRARY);   // swipe up -> Library
            else if (dir == LV_DIR_BOTTOM) ui_show(UI_SCREEN_LOCK);  // swipe down -> Lock
            break;
        case UI_SCREEN_LIBRARY:
            if (dir == LV_DIR_BOTTOM || dir == LV_DIR_LEFT) ui_show(UI_SCREEN_NOW_PLAYING);
            break;
        case UI_SCREEN_LIST:
        case UI_SCREEN_QUEUE:
        case UI_SCREEN_SETTINGS:
            if (dir == LV_DIR_LEFT || dir == LV_DIR_BOTTOM) nav_back();  // swipe left = back
            break;
        default: break;
    }
}

// Library tile tapped -> open the corresponding list/screen.
static void tile_cb(lv_event_t *e) {
    intptr_t which = (intptr_t)lv_event_get_user_data(e);
    if (which == 3) nav_push(UI_SCREEN_QUEUE);     // Queue tile
    else            nav_push(UI_SCREEN_LIST);      // Artists/Albums/Playlists -> list
}

void ui_attach_tile_cb(lv_obj_t *tile, int index) {
    lv_obj_add_event_cb(tile, tile_cb, LV_EVENT_CLICKED, (void *)(intptr_t)index);
}

// Lock ring: long-press (~hold) to unlock -> Now Playing.
static void unlock_cb(lv_event_t *e) { ui_show(UI_SCREEN_NOW_PLAYING); }

void ui_attach_unlock_cb(lv_obj_t *ring) {
    lv_obj_add_event_cb(ring, unlock_cb, LV_EVENT_LONG_PRESSED, nullptr);
}

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

    // Attach the gesture handler to every screen.
    for (int i = 0; i <= UI_SCREEN_LOCK; i++) {
        if (s_screens[i]) lv_obj_add_event_cb(s_screens[i], screen_gesture_cb, LV_EVENT_GESTURE, nullptr);
    }
    ui_show(UI_SCREEN_NOW_PLAYING);
}

void ui_show(ui_screen_t screen) {
    if (screen > UI_SCREEN_LOCK || !s_screens[screen]) return;
    s_current = screen;
    lv_scr_load(s_screens[screen]);
    // full_refresh + manual-rotation flush: force a complete repaint so no
    // stale pixels from the previous screen linger.
    lv_obj_invalidate(s_screens[screen]);
}

ui_screen_t ui_current(void) { return s_current; }

void ui_update_nowplaying(const ui_nowplaying_t *np) {
    screen_now_playing_update(np);
}

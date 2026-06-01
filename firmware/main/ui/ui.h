// ui.h — top-level UI entry + screen manager for MA-ESPortable.
// LVGL 8.x. All LVGL calls happen from the single UI task (see architecture.md).
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_SCREEN_BOOT,
    UI_SCREEN_CONNECTING,
    UI_SCREEN_NOW_PLAYING,
    UI_SCREEN_LIBRARY,     // 2x2 tiles
    UI_SCREEN_LIST,        // a chosen category / album track list
    UI_SCREEN_QUEUE,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_CHARGING,
    UI_SCREEN_LOCK,
} ui_screen_t;

// Now-playing data the UI renders (fed from app_state; MA is source of truth).
typedef struct {
    const char *title;
    const char *artist;
    const char *album;
    uint32_t    elapsed_ms;
    uint32_t    duration_ms;
    uint8_t     volume;        // 0-100
    bool        playing;
    bool        shuffle;
    uint8_t     repeat;        // 0 off / 1 one / 2 all
} ui_nowplaying_t;

// Create the whole UI (call once from the UI task after lvgl + display init).
void ui_init(void);

// Switch the active screen.
void ui_show(ui_screen_t screen);
ui_screen_t ui_current(void);

// Push fresh now-playing data (call when state changes; UI redraws).
void ui_update_nowplaying(const ui_nowplaying_t *np);

#ifdef __cplusplus
}
#endif

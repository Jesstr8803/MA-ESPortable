// app_state.h — central state + cross-task event plumbing for MA-ESPortable.
// Architecture: MA is the source of truth. Producer tasks (Sendspin, MA-API,
// IMU, remote-sense, button) post events to a single app-event queue; the app
// task applies them to player_state and tells the UI to redraw. User actions
// become OUTBOUND commands (Sendspin controller / MA-API) — the UI never
// optimistically mutates state. See architecture.md.
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Player state (UI reads this; updated only by the app task) ----
typedef struct {
    char     title[96];
    char     artist[96];
    char     album[96];
    uint32_t elapsed_ms;
    uint32_t duration_ms;
    uint8_t  volume;        // 0-100
    bool     playing;
    bool     muted;
    bool     shuffle;
    uint8_t  repeat;        // 0 off / 1 one / 2 all
    bool     ma_connected;  // Sendspin/MA link up
    uint8_t  battery_pct;
    bool     charging;
    bool     locked;
} player_state_t;

// ---- Events posted to the app task ----
typedef enum {
    // inbound (from MA / sensors)
    EVT_METADATA_CHANGED,    // title/artist/album/progress updated (data in state already staged)
    EVT_PLAYBACK_CHANGED,    // play/pause/volume/shuffle/repeat
    EVT_CONN_CHANGED,        // ma_connected toggled
    EVT_BATTERY,             // battery_pct / charging
    // user intents (UI / headphone remote / button) -> become outbound commands
    EVT_USER_PLAY_PAUSE,
    EVT_USER_NEXT,
    EVT_USER_PREV,
    EVT_USER_VOLUME,         // arg = 0..100
    EVT_USER_SEEK,           // arg = ms
    EVT_USER_SHUFFLE,
    EVT_USER_REPEAT,
    EVT_USER_PLAY_MEDIA,     // arg unused here; uri passed via ma_api directly
    // lock / power
    EVT_WAKE,                // IMU pickup -> turn screen on (still locked)
    EVT_LOCK,                // timeout / button / pocket
    EVT_UNLOCK,              // hold-ring completed
    EVT_BTN_SHORT,
    EVT_BTN_LONG,
} app_evt_type_t;

typedef struct {
    app_evt_type_t type;
    int32_t        arg;      // volume / seek-ms / etc. as needed
} app_evt_t;

// ---- API ----
void            app_state_init(void);                 // create queue + mutex
const player_state_t *app_state_get(void);            // snapshot pointer (read in app task / UI task)
bool            app_post(app_evt_type_t type, int32_t arg);   // from any task/ISR-safe wrapper
bool            app_post_from_isr(app_evt_type_t type, int32_t arg);
bool            app_wait(app_evt_t *out, uint32_t timeout_ms); // app task blocks here

// Staging setters for inbound data (call then post EVT_*_CHANGED).
void app_stage_metadata(const char *title, const char *artist, const char *album,
                        uint32_t elapsed_ms, uint32_t duration_ms);
void app_stage_playback(bool playing, uint8_t volume, bool shuffle, uint8_t repeat);

#ifdef __cplusplus
}
#endif

// app_state.cpp — central state + event queue implementation.
#include "app_state.h"
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

static player_state_t   s_state;
static QueueHandle_t    s_queue;     // app_evt_t
static SemaphoreHandle_t s_mutex;    // guards s_state staging

void app_state_init(void) {
    std::memset(&s_state, 0, sizeof(s_state));
    s_state.volume = 50;
    s_queue = xQueueCreate(16, sizeof(app_evt_t));
    s_mutex = xSemaphoreCreateMutex();
}

const player_state_t *app_state_get(void) { return &s_state; }

bool app_post(app_evt_type_t type, int32_t arg) {
    if (!s_queue) return false;
    app_evt_t e = { type, arg };
    return xQueueSend(s_queue, &e, 0) == pdTRUE;
}

bool app_post_from_isr(app_evt_type_t type, int32_t arg) {
    if (!s_queue) return false;
    app_evt_t e = { type, arg };
    BaseType_t hp = pdFALSE;
    BaseType_t ok = xQueueSendFromISR(s_queue, &e, &hp);
    if (hp) portYIELD_FROM_ISR();
    return ok == pdTRUE;
}

bool app_wait(app_evt_t *out, uint32_t timeout_ms) {
    if (!s_queue) return false;
    return xQueueReceive(s_queue, out, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void app_stage_metadata(const char *title, const char *artist, const char *album,
                        uint32_t elapsed_ms, uint32_t duration_ms) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
    if (title)  { std::strncpy(s_state.title,  title,  sizeof(s_state.title) - 1);  s_state.title[sizeof(s_state.title)-1] = 0; }
    if (artist) { std::strncpy(s_state.artist, artist, sizeof(s_state.artist) - 1); s_state.artist[sizeof(s_state.artist)-1] = 0; }
    if (album)  { std::strncpy(s_state.album,  album,  sizeof(s_state.album) - 1);  s_state.album[sizeof(s_state.album)-1] = 0; }
    s_state.elapsed_ms = elapsed_ms;
    s_state.duration_ms = duration_ms;
    xSemaphoreGive(s_mutex);
}

void app_stage_playback(bool playing, uint8_t volume, bool shuffle, uint8_t repeat) {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
    s_state.playing = playing;
    s_state.volume = volume;
    s_state.shuffle = shuffle;
    s_state.repeat = repeat;
    xSemaphoreGive(s_mutex);
}

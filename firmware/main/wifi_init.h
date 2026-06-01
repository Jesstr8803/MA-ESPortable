// wifi_init.h — STA WiFi bring-up. Ported from sendspin-xiao, adapted for a
// BATTERY device: enable WiFi power-save (the deep audio buffer absorbs the
// jitter) instead of the predecessor's mains-tuned WIFI_PS_NONE.
#pragma once
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_init_only(void);
esp_err_t wifi_start_and_wait(const char* ssid, const char* password, uint32_t timeout_ms);
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

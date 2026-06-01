// wifi_init.cpp — ported from sendspin-xiao, adapted for battery (power-save on).
#include "wifi_init.h"
#include <cstring>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char* TAG = "wifi";
static EventGroupHandle_t s_group = nullptr;
static constexpr int CONNECTED_BIT = BIT0;
static constexpr int FAIL_BIT = BIT1;
static int s_retry = 0;

static void evt(void*, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_group, CONNECTED_BIT);
        if (s_retry < 20) { esp_wifi_connect(); s_retry++; ESP_LOGW(TAG, "retry %d", s_retry); }
        else { xEventGroupSetBits(s_group, FAIL_BIT); ESP_LOGE(TAG, "giving up"); }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* e = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        s_retry = 0; xEventGroupSetBits(s_group, CONNECTED_BIT);
    }
}

bool wifi_is_connected(void) {
    return s_group && (xEventGroupGetBits(s_group) & CONNECTED_BIT);
}

esp_err_t wifi_init_only(void) {
    if (!s_group) s_group = xEventGroupCreate();
    static bool inited = false;
    if (inited) return ESP_OK;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    inited = true;
    return ESP_OK;
}

esp_err_t wifi_start_and_wait(const char* ssid, const char* password, uint32_t timeout_ms) {
    wifi_init_only();
    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == nullptr) {
        esp_netif_create_default_wifi_sta();
    }
    esp_event_handler_instance_t h1, h2;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, evt, nullptr, &h1));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, evt, nullptr, &h2));

    wifi_config_t wc = {};
    std::strncpy(reinterpret_cast<char*>(wc.sta.ssid), ssid, sizeof(wc.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(wc.sta.password), password, sizeof(wc.sta.password) - 1);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    ESP_ERROR_CHECK(esp_wifi_start());

    // BATTERY DEVICE: modem-sleep power-save (vs predecessor's WIFI_PS_NONE).
    // The deep audio buffer absorbs the added latency/jitter. Solo listener =
    // no µs multi-room sync to protect. This is a major battery lever.
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    EventBits_t bits = xEventGroupWaitBits(s_group, CONNECTED_BIT | FAIL_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(timeout_ms));
    return (bits & CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}

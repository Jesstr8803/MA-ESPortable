// wifi_provisioning.cpp — SKELETON.
// Fills in once the board is in hand. Structure mirrors the predecessor
// (sendspin-xiao): STA from NVS creds, SoftAP captive portal on failure.
#include "wifi_provisioning.h"
#include "esp_log.h"

static const char *TAG = "wifi_prov";
static wifi_state_t s_state = WIFI_STATE_BOOT;

void wifi_prov_init(void)
{
    s_state = WIFI_STATE_CONNECTING;
    ESP_LOGI(TAG, "wifi_prov_init() — TODO: NVS creds -> STA connect; "
                  "SoftAP captive portal fallback (port 80, 192.168.4.1).");
    // TODO:
    //  1. esp_netif + event loop init
    //  2. read SSID/pass from NVS
    //  3. if present -> WIFI_MODE_STA, connect, on GOT_IP set CONNECTED
    //  4. else / on repeated fail -> WIFI_MODE_AP "MA-ESPortable-XXXX",
    //     DNS hijack + http_server serving the setup page; save creds -> reboot
    //  5. enable WiFi power-save (WIFI_PS_MIN_MODEM) once connected (battery)
}

wifi_state_t wifi_prov_state(void) { return s_state; }

void wifi_prov_forget_and_reboot(void)
{
    ESP_LOGW(TAG, "forget WiFi creds + reboot to setup mode — TODO");
    // TODO: erase NVS wifi namespace; esp_restart();
}

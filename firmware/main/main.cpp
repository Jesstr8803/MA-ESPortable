// main.cpp — MA-ESPortable firmware entry point.
//
// Boot flow (built up over the milestones below):
//   1. NVS + board init
//   2. Display (SH8601 QSPI) + LVGL + touch (FT3168)         [Milestone 1: bring-up]
//   3. IMU (QMI8658) for wake/orientation                    [Milestone 4]
//   4. WiFi (STA from NVS, else SoftAP provisioning)          [Milestone 3]
//   5. Sendspin client -> connect to Music Assistant          [Milestone 5]
//        - player (audio), metadata (now-playing), controller, artwork
//   6. UI screens (port from ui-prototype/)                   [Milestone 2]
//   7. Audio out via CS43131 over I2S                         [carrier / spike]
//
// THE SPIKE (decides ESP32-S3 vs P4): prove LVGL + WiFi + I2S audio + a taste
// of the Tier-2 scrolling/thumbnail UI all coexist without contention.
//
// Most of milestones 1-6 run on the BARE Waveshare board (no carrier needed).

#include "board_pins.h"
#include "wifi_init.h"
#include "wifi_provisioning.h"
#include "display.h"
#include "ui/ui.h"
#include "secrets.h"   // LOCAL, gitignored: SECRET_WIFI_SSID/PASS/MA_URL

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ma-esportable";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "MA-ESPortable booting…");
    init_nvs();

    // --- Milestone 1: display + LVGL + UI ---
    display_init();             // SH8601 QSPI panel + LVGL + lvgl task + ui_init()
    if (display_lock(-1)) { ui_show(UI_SCREEN_BOOT); display_unlock(); }

    // --- Milestone 3: WiFi (temporary hardcoded creds from secrets.h) ---
    // TODO: replace with NVS-stored creds + SoftAP captive-portal provisioning.
    if (display_lock(-1)) { ui_show(UI_SCREEN_CONNECTING); display_unlock(); }
    esp_err_t werr = wifi_start_and_wait(SECRET_WIFI_SSID, SECRET_WIFI_PASS, 30000);
    if (werr == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected");
    } else {
        ESP_LOGW(TAG, "WiFi connect failed (err=%d) — continuing offline", werr);
    }
    if (display_lock(-1)) { ui_show(UI_SCREEN_NOW_PLAYING); display_unlock(); }

    // --- Milestone 4: IMU (QMI8658) for wake-on-pickup ---
    // TODO: init over I2C (I2C_ADDR_IMU), motion interrupt on PIN_IMU_INT1.

    // --- Milestone 5: Sendspin client ---
    // TODO: construct SendspinClient, add player/metadata/controller/artwork
    //       roles, mDNS advertise _sendspin._tcp on 8928, start server.

    // --- Audio (carrier): I2S -> CS43131 ---
    // TODO: i2s_std init on PIN_I2S_* ; deep buffer; gate on time-sync.

    ESP_LOGI(TAG, "init done — entering idle loop (scaffold).");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

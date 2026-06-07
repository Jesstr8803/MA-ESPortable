// mdns_init.cpp — ported from sendspin-xiao.
#include "mdns_init.h"
#include "esp_log.h"
#include "mdns.h"

static const char* TAG = "mdns";

esp_err_t mdns_advertise_sendspin(const char* instance_name, uint16_t port, const char* path) {
    // Don't ESP_ERROR_CHECK these — if mDNS is already inited (or the SDK inits
    // it), a non-fatal error must NOT abort/reboot. Log and continue.
    esp_err_t e = mdns_init();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "mdns_init: %s", esp_err_to_name(e));
    }
    mdns_hostname_set("ma-esportable");
    mdns_instance_name_set(instance_name);

    mdns_txt_item_t txt[] = {
        {"path", path},
        {"name", instance_name},
    };
    esp_err_t err = mdns_service_add(instance_name, "_sendspin", "_tcp", port,
                                     txt, sizeof(txt) / sizeof(txt[0]));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mdns_service_add: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "advertising _sendspin._tcp on %u (path=%s)", port, path);
    return ESP_OK;
}

// mdns_init.cpp — ported from sendspin-xiao.
#include "mdns_init.h"
#include "esp_log.h"
#include "mdns.h"

static const char* TAG = "mdns";

esp_err_t mdns_advertise_sendspin(const char* instance_name, uint16_t port, const char* path) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("ma-esportable"));
    ESP_ERROR_CHECK(mdns_instance_name_set(instance_name));

    mdns_txt_item_t txt[] = {
        {"path", path},
        {"name", instance_name},
    };
    esp_err_t err = mdns_service_add(instance_name, "_sendspin", "_tcp", port,
                                     txt, sizeof(txt) / sizeof(txt[0]));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_service_add failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "advertising _sendspin._tcp on %u (path=%s)", port, path);
    return ESP_OK;
}

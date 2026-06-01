// nvs_store.cpp — ported from sendspin-xiao NvsPersistence (+ MA token, UI prefs).
#include "nvs_store.h"
#include "esp_log.h"
#include "nvs.h"

static const char* NS = "maesp";

namespace {
bool set_u32(const char* k, uint32_t v) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_u32(h, k, v) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}
std::optional<uint32_t> get_u32(const char* k) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return std::nullopt;
    uint32_t v = 0;
    esp_err_t e = nvs_get_u32(h, k, &v);
    nvs_close(h);
    if (e != ESP_OK) return std::nullopt;
    return v;
}
bool set_u16(const char* k, uint16_t v) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_u16(h, k, v) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}
std::optional<uint16_t> get_u16(const char* k) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return std::nullopt;
    uint16_t v = 0;
    esp_err_t e = nvs_get_u16(h, k, &v);
    nvs_close(h);
    if (e != ESP_OK) return std::nullopt;
    return v;
}
bool set_u8(const char* k, uint8_t v) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_u8(h, k, v) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}
std::optional<uint8_t> get_u8(const char* k) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return std::nullopt;
    uint8_t v = 0;
    esp_err_t e = nvs_get_u8(h, k, &v);
    nvs_close(h);
    if (e != ESP_OK) return std::nullopt;
    return v;
}
bool set_str(const char* k, const char* v) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = (nvs_set_str(h, k, v) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}
std::optional<std::string> get_str(const char* k) {
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return std::nullopt;
    size_t len = 0;
    if (nvs_get_str(h, k, nullptr, &len) != ESP_OK || len == 0) {
        nvs_close(h);
        return std::nullopt;
    }
    std::string out(len, '\0');
    esp_err_t e = nvs_get_str(h, k, out.data(), &len);
    nvs_close(h);
    if (e != ESP_OK) return std::nullopt;
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}
}  // namespace

bool NvsStore::save_last_server_hash(uint32_t h) { return set_u32("last_srv", h); }
std::optional<uint32_t> NvsStore::load_last_server_hash() { return get_u32("last_srv"); }
bool NvsStore::save_static_delay(uint16_t ms) { return set_u16("static_dly", ms); }
std::optional<uint16_t> NvsStore::load_static_delay() { return get_u16("static_dly"); }

bool NvsStore::save_volume(uint8_t v) { return set_u8("volume", v); }
std::optional<uint8_t> NvsStore::load_volume() { return get_u8("volume"); }
bool NvsStore::save_muted(bool m) { return set_u8("muted", m ? 1 : 0); }
std::optional<bool> NvsStore::load_muted() { auto v = get_u8("muted"); if (!v) return std::nullopt; return *v != 0; }

bool NvsStore::save_wifi_credentials(const char* ssid, const char* pwd) {
    if (!ssid) return false;
    if (!set_str("wifi_ssid", ssid)) return false;
    return set_str("wifi_pwd", pwd ? pwd : "");
}
std::optional<std::string> NvsStore::load_wifi_ssid() { return get_str("wifi_ssid"); }
std::optional<std::string> NvsStore::load_wifi_password() { return get_str("wifi_pwd"); }
bool NvsStore::clear_wifi_credentials() { return save_wifi_credentials("", ""); }

bool NvsStore::save_device_name(const char* n) { return set_str("dev_name", n ? n : ""); }
std::optional<std::string> NvsStore::load_device_name() { return get_str("dev_name"); }

bool NvsStore::save_ma_server(const char* url) { return set_str("ma_url", url ? url : ""); }
std::optional<std::string> NvsStore::load_ma_server() { return get_str("ma_url"); }
bool NvsStore::save_ma_token(const char* t) { return set_str("ma_token", t ? t : ""); }
std::optional<std::string> NvsStore::load_ma_token() { return get_str("ma_token"); }

bool NvsStore::save_brightness(uint8_t pct) { return set_u8("bright", pct); }
std::optional<uint8_t> NvsStore::load_brightness() { return get_u8("bright"); }
bool NvsStore::save_sleep_timeout_s(uint16_t s) { return set_u16("sleep_s", s); }
std::optional<uint16_t> NvsStore::load_sleep_timeout_s() { return get_u16("sleep_s"); }

// nvs_store.h — NVS-backed persistence for MA-ESPortable.
// Ported from the predecessor (sendspin-xiao) NvsPersistence, with additions:
// the Music Assistant API token (Tier-2) and UI prefs (brightness, sleep, etc).
// Implements sendspin's persistence provider for volume/mute/static-delay/server.
#pragma once
#include <optional>
#include <string>
#include "sendspin/client.h"   // sendspin::SendspinPersistenceProvider

class NvsStore : public sendspin::SendspinPersistenceProvider {
public:
    // --- sendspin::SendspinPersistenceProvider ---
    bool save_last_server_hash(uint32_t hash) override;
    std::optional<uint32_t> load_last_server_hash() override;
    bool save_static_delay(uint16_t delay_ms) override;
    std::optional<uint16_t> load_static_delay() override;

    // --- player state ---
    bool save_volume(uint8_t volume);
    std::optional<uint8_t> load_volume();
    bool save_muted(bool muted);
    std::optional<bool> load_muted();

    // --- WiFi creds ---
    bool save_wifi_credentials(const char* ssid, const char* password);
    std::optional<std::string> load_wifi_ssid();
    std::optional<std::string> load_wifi_password();
    bool clear_wifi_credentials();

    // --- identity ---
    bool save_device_name(const char* name);
    std::optional<std::string> load_device_name();

    // --- NEW: Music Assistant API (Tier-2) ---
    bool save_ma_server(const char* url);            // e.g. http://192.168.x.x:8095
    std::optional<std::string> load_ma_server();
    bool save_ma_token(const char* token);           // long-lived token
    std::optional<std::string> load_ma_token();

    // --- NEW: UI prefs ---
    bool save_brightness(uint8_t pct);
    std::optional<uint8_t> load_brightness();
    bool save_sleep_timeout_s(uint16_t seconds);
    std::optional<uint16_t> load_sleep_timeout_s();
};

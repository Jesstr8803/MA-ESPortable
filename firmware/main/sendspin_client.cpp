// sendspin_client.cpp — connect to Music Assistant via the Sendspin SDK.
// For this milestone: appear in MA as a player, receive METADATA (now-playing)
// and push it to the UI. Audio is STUBBED (on_audio_write discards) until the
// carrier/DAC exists. Controller role added for future transport control.
#include "sendspin_client.h"
#include "board_pins.h"
#include "wifi_init.h"
#include "mdns_init.h"
#include "ui/ui.h"
#include "display.h"

#include <thread>
#include <chrono>
#include <cstring>
#include "esp_log.h"
#include "esp_pthread.h"
#include "esp_app_desc.h"   // esp_app_get_description (esp_app_format)
#include "esp_mac.h"        // esp_read_mac
#include "esp_timer.h"      // esp_timer_get_time
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sendspin/client.h"
#include "sendspin/player_role.h"
#include "sendspin/metadata_role.h"
#include "sendspin/controller_role.h"

using namespace sendspin;
static const char *TAG = "sendspin";

// --- network readiness: just defer to our WiFi state ---
class NetProvider : public SendspinNetworkProvider {
public:
    bool is_network_ready() override { return wifi_is_connected(); }
};

// --- player listener: no DAC yet, but PACE realistically so MA stays happy ---
// Returning instantly (consume-infinitely-fast) makes the SDK's time-sync
// diverge and MA drops/reconnects. So we consume bytes at the true playback
// rate (sleep for their duration) and report them via notify_audio_played.
class PacedPlayer : public PlayerRoleListener {
public:
    PlayerRole *player = nullptr;        // set after add_player
    void on_stream_start() override { stream_pos_ = 0; }
    size_t on_audio_write(uint8_t * /*data*/, size_t length, uint32_t /*timeout_ms*/) override {
        // Decoded PCM is 16-bit stereo. bytes/sec = rate*2ch*2bytes.
        const uint32_t bytes_per_sec = 44100u * 2u * 2u;     // assume 44.1k
        uint32_t frames = length / 4;                        // 4 bytes/frame (2ch*16b)
        // Sleep for the audio duration so we "play" in real time.
        uint32_t us = (uint32_t)((uint64_t)length * 1000000ull / bytes_per_sec);
        if (us > 0) std::this_thread::sleep_for(std::chrono::microseconds(us));
        // Report consumed frames to the sync engine.
        if (player) player->notify_audio_played(frames, esp_timer_get_time());
        stream_pos_ += frames;
        return length;
    }
private:
    uint64_t stream_pos_ = 0;
};

// --- metadata listener: push now-playing into the UI ---
class MetaListener : public MetadataRoleListener {
public:
    void on_metadata(const ServerMetadataStateObject &m) override {
        ui_nowplaying_t np = {};
        static std::string t, a, al;   // keep strings alive for the UI
        t  = m.title.value_or("");
        a  = m.artist.value_or("");
        al = m.album.value_or("");
        np.title  = t.empty() ? "Unknown" : t.c_str();
        np.artist = a.c_str();
        np.album  = al.c_str();
        if (m.progress) {
            np.elapsed_ms  = m.progress->track_progress;
            np.duration_ms = m.progress->track_duration;
        }
        np.playing = true;
        ESP_LOGI(TAG, "metadata: %s - %s", np.title, np.artist);
        // Thread-safe post (no LVGL here). The LVGL task applies it via ui_pump.
        ui_post_nowplaying(&np);
    }
    void on_metadata_clear() override {
        ui_nowplaying_t np = {};
        np.title = "Not playing"; np.artist = ""; np.album = "";
        ui_post_nowplaying(&np);
    }
};

static SendspinClient *s_client = nullptr;
static NetProvider     s_net;
static PacedPlayer     s_player_listener;
static MetaListener    s_meta_listener;

static std::string make_client_id() {
    uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[32];
    snprintf(buf, sizeof(buf), "ma-esportable-%02x%02x%02x", mac[3], mac[4], mac[5]);
    return buf;
}

static void sendspin_task(void *arg) {
    const char *device_name = (const char *)arg;
    // Verbose Sendspin logging during bring-up to see the handshake.
    esp_log_level_set("sendspin", ESP_LOG_DEBUG);
    esp_log_level_set("sendspin.client", ESP_LOG_DEBUG);
    esp_log_level_set("sendspin.connection", ESP_LOG_DEBUG);
    esp_log_level_set("sendspin.ws_server", ESP_LOG_DEBUG);
    esp_log_level_set("sendspin.connection_manager", ESP_LOG_DEBUG);

    SendspinClientConfig cfg;
    cfg.client_id = make_client_id();
    cfg.name = device_name ? device_name : "MA-ESPortable";
    cfg.product_name = "ESP32-S3-AMOLED-1.91";
    cfg.manufacturer = "DIY";
    const char *ver = esp_app_get_description()->version;
    cfg.software_version = (ver[0] == 'v') ? ver + 1 : ver;
    cfg.time_burst_interval_ms = 1000;
    cfg.time_burst_size = 64;
    cfg.httpd_psram_stack = true;   // big httpd task stack in PSRAM (predecessor)

    static SendspinClient client(std::move(cfg));
    s_client = &client;

    // Player role (stub audio) so MA treats us as a playable endpoint.
    // We have NO DAC yet and discard audio, so advertise ONLY Opus (lowest
    // bitrate, ~10x less than FLAC). On weak WiFi this keeps the stream light
    // enough that it doesn't choke metadata. Add FLAC/PCM back once the carrier
    // DAC exists and WiFi is solid.
    PlayerRoleConfig pc;
    pc.audio_formats = {
        {SendspinCodecFormat::OPUS, 2, 48000, 16},
    };
    // 2 MB buffer (in PSRAM) — MA bursts ~2 MB to pre-buffer; a small buffer
    // overflows instantly ("Failed to send audio chunk") and the player never
    // reaches ready, so MA never sends metadata. Predecessor used 2 MB.
    pc.audio_buffer_capacity = 2000000;
    pc.psram_stack = true;
    auto &player = client.add_player(std::move(pc));
    s_player_listener.player = &player;     // so it can notify_audio_played
    player.set_listener(&s_player_listener);

    // Metadata (now-playing) + controller (future transport).
    auto &meta = client.add_metadata();
    meta.set_listener(&s_meta_listener);
    client.add_controller();

    client.set_network_provider(&s_net);

    // Advertise so MA discovers us. (Use device_name; cfg was moved into client.)
    const char *adv_name = device_name ? device_name : "MA-ESPortable";
    mdns_advertise_sendspin(adv_name, 8928, "/sendspin");

    // CRITICAL (predecessor): the SDK spawns its worker threads as pthreads.
    // The default pthread stack is too small for the SDK's WS/sync/handshake
    // work -> stack overflow -> scheduler crash on connect. Give pthreads a big
    // stack + pin to core 1 BEFORE start_server() creates them.
    esp_pthread_cfg_t pth = esp_pthread_get_default_config();
    pth.stack_size = 8192;
    pth.prio = 6;
    pth.pin_to_core = 1;
    esp_pthread_set_cfg(&pth);

    if (!client.start_server()) {
        ESP_LOGE(TAG, "start_server failed");
        vTaskDelete(nullptr);
        return;
    }
    ESP_LOGI(TAG, "Sendspin server started — advertising '%s' on :8928", adv_name);

    while (true) {
        client.loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void sendspin_start(const char *device_name) {
    static char name[40];
    strncpy(name, device_name ? device_name : "MA-ESPortable", sizeof(name) - 1);
    // Set pthread defaults HERE (caller's context) so threads the SDK spawns
    // inherit a big stack — matches the predecessor's app_main placement.
    esp_pthread_cfg_t pth = esp_pthread_get_default_config();
    pth.stack_size = 8192;
    pth.prio = 6;
    pth.pin_to_core = 1;
    esp_pthread_set_cfg(&pth);
    // Bigger task stack too (12K) — handshake + callbacks are stack-heavy.
    xTaskCreatePinnedToCore(sendspin_task, "sendspin", 12288, name, 6, nullptr, 1);
}

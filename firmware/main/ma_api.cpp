// ma_api.cpp — Tier-2 MA WebSocket client. STRUCTURAL STUB.
// Built on esp_websocket_client (already a dependency). Connection lifecycle,
// the {message_id, command, args} envelope, and message-id->callback routing
// are scaffolded; the TODOs are filled during the Tier-2 milestone.
// Protocol reference: firmware/tier2-ma-api.md.
#include "ma_api.h"
#include <cstdio>
#include <cstring>
#include <string>
#include "esp_log.h"

static const char *TAG = "ma_api";
static ma_state_t  s_state = MA_DISCONNECTED;
static std::string s_token;
static uint32_t    s_msg_id = 1;   // monotonic message_id source

// Build ws URL from an http(s) base: replace scheme, append /ws.
static std::string ws_url_from(const char *server_url) {
    std::string u = server_url ? server_url : "";
    if (u.rfind("https", 0) == 0)      u.replace(0, 5, "wss");
    else if (u.rfind("http", 0) == 0)  u.replace(0, 4, "ws");
    if (u.size() < 3 || u.substr(u.size() - 3) != "/ws") u += "/ws";
    return u;
}

bool ma_api_start(const char *server_url, const char *token) {
    s_token = token ? token : "";
    std::string url = ws_url_from(server_url);
    s_state = MA_CONNECTING;
    ESP_LOGI(TAG, "ma_api_start -> %s (token %s)", url.c_str(), s_token.empty() ? "none" : "set");
    // TODO: esp_websocket_client_init({.uri=url}), register WEBSOCKET_EVENT_DATA,
    //       on connect read ServerInfoMessage, if schema>=28 send {command:"auth",
    //       args:{token}}, then MA_READY. Route DATA frames by message_id ->
    //       pending-callback map; accumulate partial:true chunks.
    return true;
}

void ma_api_stop(void) {
    s_state = MA_DISCONNECTED;
    // TODO: esp_websocket_client_stop/destroy
}

ma_state_t ma_api_state(void) { return s_state; }

// Serialize + send one command. (Stub logs; real version uses ArduinoJson +
// esp_websocket_client_send_text and stores cb under the message_id.)
static void send_command(const char *command, const std::string &args_json,
                         ma_result_cb cb, void *user) {
    uint32_t id = s_msg_id++;
    (void)cb; (void)user;
    ESP_LOGI(TAG, "{\"message_id\":\"%u\",\"command\":\"%s\",\"args\":%s}",
             (unsigned)id, command, args_json.c_str());
    // TODO: register (id -> cb,user); esp_websocket_client_send_text(...)
}

void ma_get_library(const char *kind, uint32_t limit, uint32_t offset, ma_result_cb cb, void *user) {
    char cmd[48];
    std::snprintf(cmd, sizeof(cmd), "music/%s/library_items", kind);
    char args[96];
    std::snprintf(args, sizeof(args), "{\"limit\":%u,\"offset\":%u}", (unsigned)limit, (unsigned)offset);
    send_command(cmd, args, cb, user);
}

void ma_get_album_tracks(const char *item_id, const char *provider, ma_result_cb cb, void *user) {
    char args[128];
    std::snprintf(args, sizeof(args), "{\"item_id\":\"%s\",\"provider\":\"%s\"}",
                  item_id ? item_id : "", provider ? provider : "");
    send_command("music/albums/album_tracks", args, cb, user);
}

void ma_search(const char *query, const char *media_types, uint32_t limit, ma_result_cb cb, void *user) {
    char args[192];
    std::snprintf(args, sizeof(args),
                  "{\"search_query\":\"%s\",\"media_types\":%s,\"limit\":%u}",
                  query ? query : "", media_types ? media_types : "null", (unsigned)limit);
    send_command("music/search", args, cb, user);
}

void ma_recently_played(uint32_t limit, ma_result_cb cb, void *user) {
    char args[48];
    std::snprintf(args, sizeof(args), "{\"limit\":%u}", (unsigned)limit);
    send_command("music/recently_played_items", args, cb, user);
}

void ma_play_media(const char *queue_id, const char *uri, const char *option) {
    char args[192];
    std::snprintf(args, sizeof(args),
                  "{\"queue_id\":\"%s\",\"media\":\"%s\",\"option\":\"%s\"}",
                  queue_id ? queue_id : "", uri ? uri : "", option ? option : "play");
    send_command("player_queues/play_media", args, nullptr, nullptr);
}

void ma_queue_cmd(const char *queue_id, const char *cmd) {
    char command[48];
    std::snprintf(command, sizeof(command), "player_queues/%s", cmd ? cmd : "play");
    char args[96];
    std::snprintf(args, sizeof(args), "{\"queue_id\":\"%s\"}", queue_id ? queue_id : "");
    send_command(command, args, nullptr, nullptr);
}

void ma_get_queue_items(const char *queue_id, uint32_t limit, uint32_t offset,
                        ma_result_cb cb, void *user) {
    char args[128];
    std::snprintf(args, sizeof(args), "{\"queue_id\":\"%s\",\"limit\":%u,\"offset\":%u}",
                  queue_id ? queue_id : "", (unsigned)limit, (unsigned)offset);
    send_command("player_queues/items", args, cb, user);
}

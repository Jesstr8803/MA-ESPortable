// ma_api.h — Music Assistant WebSocket API client (Tier-2 library browse/queue).
// Second WebSocket (separate from Sendspin). See firmware/tier2-ma-api.md for the
// full protocol design. This is the structural stub: connection + command
// envelope + the command senders we need. Runtime logic filled in during Tier-2.
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MA_DISCONNECTED,
    MA_CONNECTING,
    MA_AUTHED,        // server_info received + auth ok
    MA_READY,
} ma_state_t;

// Result callback: raw JSON result string for a given message_id (chunks may
// arrive with partial=true; the client accumulates and calls back when whole).
// NOTE (verified vs live MA 2.8.8): result is a BARE JSON ARRAY of media items.
// Per item we use: item_id, provider, uri (-> play_media), name, media_type,
// is_playable; albums also year + artists[]. Album art = metadata.images[] entry
// with type=="thumb" -> fetch GET {base}/imageproxy?path=<urlenc path>&size=NN
// (the PATH form, not /imageproxy/{id}).
typedef void (*ma_result_cb)(const char *json_result, void *user);

// Connect to ws://<server>/ws ; token used if schema_version >= 28.
// server_url like "http://192.168.86.37:8095".
bool ma_api_start(const char *server_url, const char *token);
void ma_api_stop(void);
ma_state_t ma_api_state(void);

// --- commands (args serialized to the {message_id, command, args} envelope) ---
// Library browse / search (paged via limit/offset).
void ma_get_library(const char *kind /*artists|albums|tracks|playlists*/,
                    uint32_t limit, uint32_t offset, ma_result_cb cb, void *user);
void ma_get_album_tracks(const char *item_id, const char *provider, ma_result_cb cb, void *user);
void ma_search(const char *query, const char *media_types, uint32_t limit, ma_result_cb cb, void *user);
void ma_recently_played(uint32_t limit, ma_result_cb cb, void *user);

// Playback on OUR queue (queue_id = our player id).
void ma_play_media(const char *queue_id, const char *uri, const char *option /*play|add|next*/);
void ma_queue_cmd(const char *queue_id, const char *cmd /*play|pause|next|previous|...*/);
void ma_get_queue_items(const char *queue_id, uint32_t limit, uint32_t offset,
                        ma_result_cb cb, void *user);

#ifdef __cplusplus
}
#endif

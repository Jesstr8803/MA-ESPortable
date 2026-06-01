# Tier-2: Music Assistant WebSocket API — design notes

On-device library browse / search / queue, by integrating MA's own WebSocket API (separate from
Sendspin). Researched from the official `music-assistant/client` + `music-assistant/models` repos —
these are the authoritative spec. *(No hardware needed for this; it's the riskiest software piece.)*

## Connection

- **WebSocket URL:** take the MA base URL (`http://<ip>:8095`), replace `http`→`ws`, append `/ws`
  → **`ws://<ip>:8095/ws`**. (HTTPS → `wss://`.)
- On connect the server immediately sends a **ServerInfoMessage** (server id, version,
  **schema_version**, base_url). Read it first.
- **Auth:** if `schema_version >= 28`, a token is required → after connect, send
  `{"command": "auth", "args": {"token": "<token>"}}` before anything else.
  - Token obtained once via HTTP `POST <base>/auth/login` (user/pass) → `login_with_token` creates a
    **long-lived token**. We do this in the **on-LAN setup page / captive portal** (paste or
    user+pass form), store the token in NVS. Never typed on the device touchscreen.

## Message envelope (every command)

```jsonc
// client → server
{ "message_id": "<unique>", "command": "music/search", "args": { ... } }

// server → client (matched by message_id)
{ "message_id": "<same>", "result": <any> }                 // success
{ "message_id": "<same>", "result": <chunk>, "partial": true } // chunked (see below)
{ "message_id": "<same>", "error_code": <int>, "details": "" } // error
```
- **`message_id`** is ours (uuid/counter); responses match by it → keep a small map of
  in-flight requests → callbacks.
- **`partial: true`** = large results stream in **chunks**; accumulate until a final non-partial
  message. *Important for the S3:* we can render rows as chunks arrive instead of buffering the whole
  list — good for memory.
- Events (state changes) also arrive on the same socket with no `message_id` — route those separately.

## Commands we need

**Browse / library (paged via `limit`/`offset`, default limit 50):**
| Purpose | command | key args |
|---|---|---|
| Artists | `music/artists/library_items` | favorite, search, limit, offset, order_by |
| Albums | `music/albums/library_items` | ″ |
| Tracks | `music/tracks/library_items` | ″ |
| Playlists | `music/playlists/library_items` | ″ |
| Albums of artist | `music/artists/artist_albums` | item_id, provider |
| Tracks of album | `music/albums/album_tracks` | item_id, provider |
| Tracks of playlist | `music/playlists/playlist_tracks` | item_id, provider |
| Recently played | `music/recently_played_items` | limit, media_types |
| Generic tree browse | `music/browse` | path (e.g. `library://artist/1`) |
| **Search** | `music/search` | search_query, media_types, limit (def 50), library_only |

**Playback / queue** (`queue_id` = our own player's id; we're already a player in MA):
| Purpose | command | key args |
|---|---|---|
| **Play media** | `player_queues/play_media` | queue_id, **media** (uri or item), **option**, radio_mode, start_item |
| Get queue items | `player_queues/items` | queue_id, limit, offset |
| Active queue | `player_queues/get_active_queue` | player_id |
| play/pause/stop/resume | `player_queues/{play,pause,stop,resume}` | queue_id |
| next / previous | `player_queues/{next,previous}` | queue_id |
| seek / skip | `player_queues/{seek,skip}` | queue_id, position/seconds |
| shuffle / repeat | `player_queues/{shuffle,repeat}` | queue_id, shuffle_enabled / repeat_mode |
| play index | `player_queues/play_index` | queue_id, index |
| move / delete item | `player_queues/{move_item,delete_item}` | queue_id, item id |

**Enums:**
- `QueueOption`: `play` (insert@current + play), `replace` (replace all), `next`, `replace_next`,
  `add` (append). → "Play now" = `play`/`replace`; "Add to queue" = `add`; "Play next" = `next`.
- `MediaType`: `artist, album, track, playlist, radio, audiobook, podcast`.
- `RepeatMode`: `off, one, all`.

## Album art — the thumbnail problem is SOLVED server-side

MA exposes an **image proxy that resizes server-side**:
- schema ≥ 31: `GET {base_url}/imageproxy/{proxy_id}?size={px}`
- else: `GET {base_url}/imageproxy?path={url-encoded}&size={px}`

So we request **small (e.g. 64–96 px) JPEGs already scaled by the server** — no full-res download, no
on-device decode of huge images. This is the key mitigation for scrolling lists:
- request thumbnails at the row's pixel size, lazy-load only **visible** rows,
- LRU-cache decoded thumbs in **PSRAM**, placeholder while loading, prefetch a few ahead.

## On-device data model (sketch)

```c
typedef struct { char id[40]; char provider[16]; char name[96];
                 char subtitle[96]; char img_proxy_id[64]; } ma_item_t;  // a list row
// paged list: ring/window of ma_item_t for the visible range + a little overscan;
// fetch next page on scroll near the end (offset += limit).
```
- Keep only a **window** of rows in RAM, not the whole library (libraries can be huge).
- Two WS sockets total: **Sendspin** (audio/now-playing/transport) + **MA-API** (this). Independent
  reconnect; if MA-API drops, Tier-1 keeps working.

## Playback loop (how a tap becomes sound)

1. User browses (MA-API) → picks a track/album/playlist → we have its **uri** (or item id+provider).
2. `player_queues/play_media` with **queue_id = our own player**, media = that uri, option = `play`.
3. MA resolves + starts streaming the audio **back to us over Sendspin** (we're the player).
4. Now-playing/transport continue via the Sendspin metadata/controller roles as today.
   (We could also target *another* room's queue_id → control other players. Bonus.)

## Open questions for when we build it
- Exact `result` JSON shape per command (fields of artist/album/track) — capture live from a real
  server's `/api-docs` or by logging responses; model only the fields the UI shows.
- C WebSocket client: reuse `esp_websocket_client` (already in the Sendspin stack) for a 2nd socket.
- JSON: ArduinoJson (already pulled in) — stream-parse chunked/`partial` results to bound memory.
- Token UX: paste vs. user+pass form on the setup page; store in NVS.

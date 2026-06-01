# MA-ESPortable firmware — architecture

How the firmware is structured: tasks, core assignment, state machine, event flow, and what we reuse
from the predecessor (`sendspin-xiao`). Design-on-paper so the build goes fast.

## Big picture

Three concurrent concerns on the ESP32-S3 (dual-core LX7):
1. **Audio** — Sendspin player: receive timestamped chunks → I²S → CS43131. Latency/glitch-sensitive.
2. **Network** — two WebSockets (Sendspin + MA-API), WiFi/lwIP, mDNS.
3. **UI** — LVGL on the SH8601 AMOLED + FT3168 touch + QMI8658 IMU.

Guiding rule (from the predecessor's hard-won lesson): **keep audio + its network off the core doing
the heavy/bursty work.** There it was WiFi vs. audio; here we add LVGL rendering as another bursty load.

## Task / core layout

| Task | Core | Priority | Notes |
|---|---|---|---|
| Sendspin client loop + sync | **1** | high (10) | `client.loop()`; SDK pthreads pinned to core 1 (predecessor proved this) |
| I²S write / audio sink | 1 | high | `PlayerRoleListener::on_audio_write` → DMA; deep buffer |
| WiFi / lwIP | 0 | (system) | ESP-IDF default |
| LVGL UI task | **0** | low-med (2) | render + touch read; ~2 ms tick. Keep OFF core 1 so it can't stall audio |
| MA-API client (Tier-2) | 0 | med | 2nd websocket; browse/queue; only active when user is in library |
| IMU poll / wake | 0 | low | QMI8658 motion → wake/lock events |
| Status/housekeeping | any | low | battery %, OTA server, `/status` |

Rationale: **core 1 = realtime audio** (Sendspin sync + I²S), **core 0 = everything else** (WiFi,
LVGL, MA-API, IMU). LVGL is bursty but tolerant of jitter; audio is not — so they're separated. This
is the arrangement the spike will validate (and the main thing that could push us to the P4).

## State machine (top level)

```
BOOT ─► (NVS, display init, splash)
  └─► WIFI: try NVS creds
        ├─ ok ─► CONNECTING (mDNS + Sendspin connect to MA)
        │           └─► CONNECTED ─► [normal operation]
        └─ fail/none ─► PROVISIONING (SoftAP captive portal) ─► reboot
```
**Normal operation** sub-states (UI): `NOW_PLAYING ⇄ LIBRARY ⇄ LIST ⇄ ALBUM`, `QUEUE`, `SETTINGS`,
and `LOCKED`. Lock is orthogonal (overlay): triggered by timeout / power-button / IMU-pocket; exits
only via hold-the-ring. Screen-sleep (panel off) pairs with lock for battery.

## Event flow

- **MA → device (state):** Sendspin metadata/group/server events + MA-API events → update a central
  `player_state` struct → LVGL observes it and redraws now-playing/progress. One-way: MA is truth.
- **User → MA (commands):** touch/headphone-remote → intent (play/pause/next/vol/seek) →
  Sendspin `controller` command (transport) OR MA-API `play_media`/queue (content). UI does **not**
  optimistically change state — it reflects what MA echoes back (avoids the controller feedback loop).
- **Cross-task:** FreeRTOS queues. e.g. headphone-remote ADC task → input queue → UI task; IMU task →
  event queue → lock controller. Keep LVGL single-threaded (all LVGL calls from the UI task).

## Reuse from the predecessor (`sendspin-xiao`)

Read-access at `…/ESP MA Endpoint/sendspin-xiao/main`. Verdict per module:

| Module | LOC | Reuse |
|---|---|---|
| `nvs_persistence.{h,cpp}` | ~180 | ✅ **near-as-is** — it's the `SendspinPersistenceProvider` + volume/mute/wifi/name. Add token + UI prefs. |
| `wifi_init.{h,cpp}` | ~110 | ✅ **as-is** — STA connect + `wifi_is_connected()`. Add WiFi power-save (battery). |
| `wifi_provisioning.{h,cpp}` | ~265 | ✅ **strong base** — SoftAP captive portal; extend the page for MA token + on-LAN config. |
| `mdns_init.{h,cpp}` | ~30 | ✅ as-is (advertise `_sendspin._tcp`). |
| `ota_server.{h,cpp}` | ~285 | ✅ reuse — HTTP server + `/ota` + `/status`; becomes our on-LAN config server too. |
| `i2s_audio_sink.{h,cpp}` | ~400 | ⚠️ **adapt** — same `PlayerRoleListener` shape + metrics + deep buffer, but retarget PCM5102A→**CS43131** (I²C init + level-shifted I²S) and drop XSMT. The notify-timing logic carries over. |
| `status_led.{h,cpp}` | ~75 | ❌ drop — no status LED; the AMOLED shows state. |
| `main.cpp` boot flow | ~180 | ✅ **template** — NVS→wifi→(prov)→mDNS→client+player→pin core 1→loop. We graft UI + MA-API onto it. |

The single biggest carry-over lesson: **deep audio buffer + pin Sendspin to core 1**, and (new for
battery) **WiFi power-save is OK because the buffer absorbs the jitter** — don't copy the
predecessor's `WIFI_PS_NONE`.

## Module plan (new `firmware/main/`)

```
main.cpp              boot flow + task creation (from predecessor template)
board_pins.h          verified pin map (done)
wifi_*.{h,cpp}        STA + SoftAP provisioning (reuse)
nvs_store.{h,cpp}     persistence (reuse + token/UI prefs)
mdns_init, ota/http   reuse (on-LAN config + OTA + /status)
audio_sink.{h,cpp}    CS43131 I²S sink (adapt from i2s_audio_sink)
codec_cs43131.{h,cpp} NEW — I²C init/volume for the DAC
imu_qmi8658.{h,cpp}   NEW — wake/orientation
ui/                   NEW — LVGL: screens (port from ui-prototype/), nav, lock controller
ma_api.{h,cpp}        NEW — Tier-2 MA WebSocket client (see tier2-ma-api.md)
remote_sense.{h,cpp}  NEW — headphone inline-remote ADC decode + calibration
app_state.{h,cpp}     NEW — central player_state + event queues
```

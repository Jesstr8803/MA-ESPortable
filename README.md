# MA-ESPortable

**A minimalist AMOLED glass-slab WiFi DAP for Music Assistant.**
*Think "an iPod that streams your whole Music Assistant library over WiFi" — a pocket digital audio player that streams to your wired headphones and shows now-playing + library controls on a premium touchscreen.*

> ⚠️ **Status: concept / planning in progress.** This document is a living design doc, not a
> locked spec. Decisions below are current direction and will keep evolving as we plan.

---

## The idea

While building the predecessor ([ESP MA Endpoint / `sendspin-xiao`](#lineage--cross-links) — a fixed,
mains-powered Music Assistant audio endpoint), the device was tested with headphones — which
sparked this: *a small, battery-powered, pocket-sized handheld* that connects to the home Music
Assistant server over WiFi and streams to **wired headphones**, with a touchscreen for now-playing
and controls.

It is a **Sendspin client** at heart (same protocol/ecosystem as the predecessor) but a major step
up in role and polish:

- The predecessor was a **player only** (received audio, played it in sync).
- This is a **player + controller + metadata + artwork** device — it plays audio *and* controls
  playback *and* shows track info + album art — plus (ambitiously) a **library browser** built on
  Music Assistant's own API.

Above all, it must **feel like a polished product**, not a dev-board hack. That goal drives the
AMOLED display, the minimalist enclosure, and the custom-PCB endgame.

## Product archetype

A **WiFi-streaming DAP** (digital audio player) in the lineage of FiiO / HiBy / Shanling players
and the iPod Classic — but the music lives on your Music Assistant server, streamed over WiFi,
rather than stored locally.

## Key features (target)

- Pocket-sized, battery-powered (**10+ hour** runtime target).
- **AMOLED** touchscreen — minimalist glass slab, no face buttons (maybe just power).
- Streams from Music Assistant over WiFi (Sendspin protocol) to **wired headphones**.
- **Now-playing** display: title / artist / album / album art / scrubbing progress.
- **Transport control**: play / pause / next / previous / volume / mute / shuffle / repeat,
  plus room/group switching.
- **Library browse / search / queue** (stretch — via Music Assistant's API; see Software below).
- **Three ways to control it:**
  1. Touchscreen
  2. **Wired-headphone inline remote** (play/vol/skip from the cable — no need to take it out of
     your pocket)
  3. **IMU wake-on-pickup** (it knows when you lift it)
- Screen sleep + **touch lockout** + **hold-to-unlock** (kills pocket mis-taps, saves the most
  power).
- **Haptic** feedback. **USB-C** charge / data / **USB-DAC mode**. **CNC-aluminum** body (v2).

## Hardware direction

| Area | Direction |
|---|---|
| **MCU** | ESP32-S3 (8 MB PSRAM, 16 MB flash) — enough for LVGL framebuffers + a deep audio buffer. |
| **Display** | AMOLED (premium look). Burn-in mitigated by screen-sleep + dimming + periodic pixel-shift. Uses a QSPI display driver. |
| **Audio** | Our own I²S DAC + headphone amp (PCM510x-class line-out + small stereo HP amp like TPA6132, *or* an integrated codec — revisit at PCB time). Predecessor's PCM510x code carries over. |
| **Jack** | 4-conductor **TRRS** so we can read the headphone inline remote (see Control scheme). |
| **Power** | LiPo + PMIC (likely AXP2101). Target 10+ h via screen-sleep + WiFi power-save + deep buffer. Battery is the dominant size driver. |
| **Sensors** | QMI8658 6-axis IMU (wake-on-pickup). Haptic motor. |
| **Build** | **Prototype on an AMOLED dev board** (e.g. Waveshare ESP32-S3-Touch-AMOLED-1.8, 368×448) + DAC + TRRS breakouts → then a **custom PCB** → resin-printed v1 enclosure → CNC-aluminum v2. |

## Control scheme — headphone inline remote

A distinctive feature: control the device from the buttons on the headphone cable, so the
minimalist slab needs no face buttons.

- Use a **TRRS jack** (CTIA: Tip = L, Ring1 = R, Ring2 = GND, Sleeve = MIC).
- The cable's buttons sit between the MIC line and GND at standard resistances
  (center/play-pause ≈ 0 Ω, Vol+ ≈ 240 Ω, Vol- ≈ 470 Ω).
- A bias pull-up from Vref to the MIC line turns those into distinct voltages; an ESP32-S3 **ADC**
  reads the node and firmware decodes the bands (with debounce) into button events.
- **Robustness:** a *"learn my remote"* calibration handles CTIA/OMTP and Apple/Android resistor
  variance; ESD diodes on the jack; jack-detect so sense is only active when plugged; graceful
  fallback to touch-only for plain (no-remote) headphones.

## Software architecture

Built incrementally, but architected for the full vision from the start.

**Tier 1 — Sendspin-native (the protocol gives this almost for free via the `sendspin-cpp` SDK):**
- `player` — receive + play synced audio (deep buffer; sync precision is *not* critical since this
  is a solo headphone device).
- `metadata` — now-playing text + progress.
- `artwork` — album / artist images on screen.
- `controller` — transport commands: play/pause/stop/next/previous/volume/mute/shuffle/repeat/
  group-switch. (This is the *complete* Sendspin controller command set — it is transport only.)

**Tier 2 — Library browse / search / queue (a separate, bigger lift):**
- **Not** part of Sendspin — requires integrating **Music Assistant's own WebSocket API** as a
  second protocol client (auth, library queries, search, queue management).
- Roughly doubles firmware + UI work (scrollable lists, on-screen search keyboard, pagination).
- Needs its own feasibility research for ESP32 (large JSON responses).

**Build order:** spike (prove LVGL + WiFi + I²S audio + the roles coexist on the dev rig, and the
headphone-remote decode works) → Tier 1 working → custom PCB → Tier 2 → enclosure.

## Power / battery strategy

The predecessor's stability recipe (permanent `WIFI_PS_NONE`, max TX power) was a *mains-device*
tuning and is **abandoned here** — this is a **solo** headphone device, so µs-level multi-room sync
doesn't matter; only **glitch-free playback + battery life** do. Levers:

- **Display sleep in pocket** (woken by IMU) — kills the backlight/panel draw, the single biggest
  consumer.
- **WiFi power-save** (modem-sleep / DTIM) — viable because we don't need tight sync.
- **Deep audio buffer** — buffer several seconds so WiFi can sleep between bursts and wake to
  refill; this resolves the power-save-vs-dropout tension.

Rough math: naive always-on ≈ 350–400 mA (a 1000 mAh cell ≈ 2.5 h); with the levers ≈ 120–180 mA
(a 2000 mAh cell ≈ 10–12 h).

## Open questions / still planning

- Final display size + exact AMOLED board for prototyping.
- DAC + headphone-amp part selection (discrete PCM510x + amp vs. integrated codec).
- Music Assistant API feasibility on ESP32 for Tier 2.
- Enclosure design language; power-button / sleep-wake behaviour.
- USB-DAC mode scope.
- *(more to come — planning ongoing)*

## Lineage & cross-links

Builds on the predecessor project. This repo has read access to the predecessor's code, project
memory, and past sessions (configured in `.claude/settings.local.json`).

| Resource | Path |
|---|---|
| Working firmware code (predecessor) | `c:\Users\Jess_\DropBox\Personal\ESP MA Endpoint` |
| Predecessor project memory | `…\ESP-MA-Endpoint\memory` |
| Past Claude sessions (predecessor) | `c:\Users\Jess_\.claude\projects\c--Users-Jess--DropBox-Personal-ESP-MA-Endpoint` |

Predecessor (`sendspin-xiao`, shipped **v0.2.14**, 2026-05-01) highlights to draw on: native
ESP-IDF Sendspin **player**, captive-portal WiFi provisioning, OTA, PCM5102A I²S sink, Kalman
time-sync tuning, NVS persistence, `/status` metrics — and the hard-won lesson that audio stutter
was a **WiFi** problem, not a timing one.

## Git workflow

- Work on `feat/*` branches, merge to `main` with `--no-ff`.
- Tag releases only on `main`.
- Remote: <https://github.com/Jesstr8803/MA-ESPortable>

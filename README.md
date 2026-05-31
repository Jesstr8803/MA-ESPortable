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
| **Platform** | **ESP / ESP-IDF** (decided). Keeps the official `sendspin-cpp` SDK + predecessor code, and the MCU battery/instant-on advantage. Linux SBC rejected (would gut battery + instant-on). |
| **MCU** | **ESP32-S3** by default (8 MB PSRAM, 16 MB flash — enough for LVGL framebuffers + a deep audio buffer). **Escalate to ESP32-P4 + C6** only if the spike proves the S3 can't drive the GUI/Tier-2 smoothly. The spike *is* the S3-vs-P4 test. |
| **Display** | **Waveshare ESP32-S3-AMOLED-1.91 (touch)** — 240×536 AMOLED bar (QSPI). Premium look; burn-in mitigated by screen-sleep + dimming + periodic pixel-shift. See Reproducibility below. |
| **Audio** | Our own I²S DAC + headphone amp (PCM510x-class line-out + small stereo HP amp like TPA6132, *or* an integrated codec — revisit at PCB time). Predecessor's PCM510x code carries over. |
| **Jack** | 4-conductor **TRRS** so we can read the headphone inline remote (see Control scheme). |
| **Power** | LiPo + PMIC (likely AXP2101). Target 10+ h via screen-sleep + WiFi power-save + deep buffer. Battery is the dominant size driver. |
| **Sensors** | QMI8658 6-axis IMU (wake-on-pickup). Haptic motor. |
| **Build** | **Prototype on the Waveshare ESP32-S3-AMOLED-1.91 (touch)** + DAC + TRRS breakouts → then a **custom carrier/shield PCB** on its 20-pin headers → resin-printed v1 enclosure → CNC-aluminum v2. |

## Reproducibility & hardware architecture

A core goal: **anyone should be able to build one.** Off-the-shelf parts where possible,
3D-print files provided, and any custom PCB orderable by anyone (JLCPCB Gerbers + BOM) and/or
sold assembled on Tindie. Same open-hardware ethos as the predecessor (which shipped a WebSerial
web-flasher on GitHub Pages).

**Architecture: a carrier / shield PCB on an off-the-shelf Waveshare AMOLED display board.**
- The **Waveshare board** provides the hard parts: ESP32-S3 module, WiFi antenna/RF, AMOLED panel
  (QSPI) + touch, PMIC, IMU. (No RF/antenna/FPC layout for us to get right.)
- Our **carrier PCB** holds only the analog section: I²S DAC + headphone amp + TRRS jack +
  headphone-remote ADC-sense + haptic driver + jack-detect. It mates to the display board's
  exposed header / castellated pads.
- **Reproduction = "buy Waveshare board + order the carrier from JLCPCB (or buy it assembled on
  Tindie) + print the STLs + assemble."**

**Display board (chosen): Waveshare ESP32-S3-AMOLED-1.91, touch version** — a slim **240×536
AMOLED bar**. It exposes **2× 20-pin headers (~27 GPIO, 8 ADC, 2 I²C, SPI)**, so the carrier
mounts as a clean shield with plenty of headroom (I²S ×3, ADC remote-sense, haptic, jack-detect,
fuel gauge, power button). Onboard: ESP32-S3R8 (8 MB PSRAM, 16 MB flash), QMI8658 IMU, USB-C, and
an MX1.25 LiPo charge/discharge header. The tall-narrow shape suits both a vertical now-playing
layout and scrolling library lists.
⚠️ **Order the *touch* variant** (it also ships non-touch). With 8 MB **octal** PSRAM, GPIO35/36/37
are reserved for PSRAM — route I²S to other exposed pins via the GPIO matrix. *(Earlier candidate:
ESP32-S3-Touch-AMOLED-1.8 (368×448, only 7 GPIO) — now a fallback.)*

**PCB design-for-manufacture:** hand-solderable (larger SMD, no fine-pitch QFN) so DIYers can
build, *plus* a JLCPCB-Assembly BOM/CPL for pre-assembled / Tindie batches. Prefer LCSC "Basic"
parts that are also stocked at DigiKey/Mouser. Version the PCB on the silkscreen and document the
PCB-rev ↔ firmware interlock.

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

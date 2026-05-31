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

Triaged into what makes it *work*, what makes it *polished*, and *stretch*. The first working
version's polish level is decided by **what the spike teaches** (can the S3 carry LVGL + WiFi +
audio + a taste of the UI). All four "optional" features below are **confirmed in scope.**

**🟢 MVP — first light (the minimum useful device):**
- Play synced audio from MA (Sendspin `player`; FLAC/PCM, deep buffer) → wired headphones.
- Now-playing screen (Sendspin `metadata`): title / artist / album / progress.
- Basic transport (Sendspin `controller`): play / pause / next / prev / volume / mute.
- Touch UI (LVGL on the 240×536 bar); WiFi connect; NVS persistence (volume, settings, last server).

**🔵 v1 — feels like a product:**
- Album art (Sendspin `artwork`); shuffle / repeat; room/group switch.
- **Headphone inline-remote** control (TRRS + ADC decode + "learn my remote") — *in scope*.
- **IMU wake-on-pickup**; screen sleep / touch-lockout / hold-to-unlock; **haptic** feedback.
- Battery + charging indicator; settings screen (name, brightness, sleep timeout, static-delay,
  remote calibration); boot splash; dark theme.
- Captive-portal WiFi provisioning; **OTA** over WiFi; pop/mute suppression; `/status`; multi-server
  handling. *(Most of this is proven predecessor code we carry over.)*

**🟣 Stretch / committed-but-later:**
- **Library browse / search / queue** (Tier 2 — MA WebSocket API) — *in scope*; can't come first
  (must play before you can browse-and-play), but it's a committed goal, not a maybe.
- **USB-DAC mode** (act as a PC sound card) — *in scope*.
- **Web flasher** (GitHub Pages WebSerial) for toolchain-free install + distribution — *in scope*.
- 24-bit audio path; EQ/DSP; sleep timer; multiple saved WiFi networks.

Physical: minimalist AMOLED glass slab, no face buttons (maybe just power); resin-printed v1 →
CNC-aluminum v2 (u.FL external antenna makes the metal body viable).

## Hardware direction

| Area | Direction |
|---|---|
| **Platform** | **ESP / ESP-IDF** (decided). Keeps the official `sendspin-cpp` SDK + predecessor code, and the MCU battery/instant-on advantage. Linux SBC rejected (would gut battery + instant-on). |
| **MCU** | **ESP32-S3** by default (8 MB PSRAM, 16 MB flash — enough for LVGL framebuffers + a deep audio buffer). **Escalate to ESP32-P4 + C6** only if the spike proves the S3 can't drive the GUI/Tier-2 smoothly. The spike *is* the S3-vs-P4 test. |
| **Display** | **Waveshare ESP32-S3-AMOLED-1.91 (touch)** — 240×536 AMOLED bar (QSPI). Premium look; burn-in mitigated by screen-sleep + dimming + periodic pixel-shift. See Reproducibility below. |
| **Audio** | **Cirrus Logic CS43131** — single-chip DAC + ground-centered headphone amp (130 dB DR, −115 dB THD+N, 32-bit/384 kHz, 2 Vrms into 600 Ω). One chip = whole output stage; covers IEMs → demanding over-ears. In stock on LCSC (C1554754 / C1554759) → JLCPCB-assemblable. 5×5 QFN (needs assembly, not hand-solder). |
| **Jack** | 4-conductor **TRRS** so we can read the headphone inline remote (see Control scheme). |
| **Power** | Onboard charging (board's MX1.25 header). **Battery: deferred** — principle is an *off-the-shelf swappable cell* (not a glued pouch) that fits behind the board; thickness budget → capacity → runtime. Stretch runtime via screen-sleep + WiFi power-save + deep buffer. |
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
- **Haptics ("Taptic-style"):** a **TI DRV2605L** driver (LCSC C527464; 100+ built-in effects +
  closed-loop auto-resonance with overdrive/braking) driving an **LRA** (not a buzzy ERM). Gives
  crisp designed taps for the buttonless touch UI. *The LRA must be rigidly bonded to the chassis*
  — half the feel is mechanical (an enclosure constraint). Won't match Apple's bespoke Taptic
  Engine, but uses the same LRA + waveform-engine architecture.
- **Reproduction = "buy Waveshare board + order the carrier from JLCPCB (or buy it assembled on
  Tindie) + print the STLs + assemble."**

**Display board (chosen): Waveshare ESP32-S3-AMOLED-1.91, touch version** — a slim **240×536
AMOLED bar**. It exposes **2× 20-pin headers (~27 GPIO, 8 ADC, 2 I²C, SPI)**, so the carrier
mounts as a clean shield with plenty of headroom (I²S ×3, ADC remote-sense, haptic, jack-detect,
fuel gauge, power button). Onboard: ESP32-S3R8 (8 MB PSRAM, 16 MB flash), QMI8658 IMU, USB-C, and
an MX1.25 LiPo charge/discharge header. The tall-narrow shape suits both a vertical now-playing
layout and scrolling library lists.
⚠️ **Order the *touch* variant, ideally with headers** (the "-M" pre-soldered-header version) so the
carrier shield mates directly. With 8 MB **octal** PSRAM, GPIO35/36/37 are reserved for PSRAM —
route I²S to other exposed pins via the GPIO matrix. *(Earlier candidate: ESP32-S3-Touch-AMOLED-1.8
(368×448, only 7 GPIO) — now a fallback.)*

Key spec details (for firmware + enclosure):
- Display driver **RM67162** (QSPI), touch **FT3168** (I²C), IMU **QMI8658** (I²C) — the ICs our
  LVGL/touch/motion bring-up targets.
- 240×536, 350 cd/m², 65K color. SoC ESP32-S3R8 (240 MHz, 16 MB flash / 8 MB octal PSRAM).
- **Ceramic chip antenna + u.FL connector** — the u.FL lets us run an external antenna inside a
  **metal (CNC) enclosure**, which would otherwise block the chip antenna.
- Touch board size **59.6 × 28.5 mm**; MX1.25 LiPo header; microSD slot; USB-C.

**PCB design-for-manufacture:** hand-solderable (larger SMD, no fine-pitch QFN) so DIYers can
build, *plus* a JLCPCB-Assembly BOM/CPL for pre-assembled / Tindie batches. Prefer LCSC "Basic"
parts that are also stocked at DigiKey/Mouser. Version the PCB on the silkscreen and document the
PCB-rev ↔ firmware interlock.

## Audio chain

Quality is a priority (the device is for good wired headphones), within the LCSC/JLCPCB
reproducibility constraint.

- **DAC + amp: Cirrus Logic CS43131** (single chip). 32-bit/384 kHz, 130 dB DR, −115 dB THD+N,
  integrated **ground-centered Class-H headphone amp** delivering 2 Vrms into 600 Ω. It's the chip
  in premium USB-C "dongle DACs" — so it's a proven match for expensive headphones, and a single
  chip is the *entire* output stage (no separate amp). Ground-centered output = no signal-path
  coupling caps (clean for IEMs) + low output impedance; 2 Vrms into 600 Ω = enough for
  high-impedance over-ears. **In stock on LCSC** (C1554754 / C1554759, ~$3.5–4) → JLCPCB-assemblable.
  0.4 mm-pitch QFN-40, so it relies on JLCPCB assembly (rest of the board stays hand-solderable).
- **Clean power:** a dedicated ultra-low-noise LDO (LT3042-class) on an isolated analog island
  feeding the CS43131, star-grounded away from the ESP32/WiFi switching noise. (Layout discipline
  matters as much as the chip.)
- **High-res:** the chip supports up to 32-bit/384 kHz, future-proofing past what MA currently
  serves (the predecessor was 16-bit only).
- **USB-DAC mode:** routed through the ESP32-S3's native USB (USB Audio Class device) → I²S →
  CS43131. One audio path, two sources (WiFi/Sendspin or USB).
- **MCLK:** supplied from the ESP32-S3 I²S MCLK pin. Driver: an I²C init register sequence (not
  zero-driver like the predecessor's PCM5102A, but bounded and one-time).
- **Bluetooth:** dropped — it undercuts the wired-quality goal and the S3 can't do A2DP anyway.

## Power tree

```
USB-C 5V ──► [board's charger] ── charges cell + power-path ──► board system (S3 + AMOLED + 3V3)
CELL ──[DW01A + dual-MOSFET protection]── VBAT ──┬──► board MX1.25 battery input
 (off-the-shelf, carrier holder)                 ├──► MAX17048 fuel gauge (I²C → accurate %)
                                                 ├──► LT3042 ultra-low-noise LDO ──► 1.8V analog
                                                 │       island ──► CS43131  ★audio-critical★
                                                 └──► DRV2605L haptics (VBAT/3V3)
Power button ──► short = deep-sleep / long = latch-off      IMU motion-int ──► wake
```

- **One charger — the board's.** It already charges the cell and runs a power-path, so the device
  **plays while charging**. We add no second charger (two on one cell is trouble).
- **Cell on the carrier**, through a **DW01A + dual-MOSFET protection** stage (LCSC C700964) so *any*
  off-the-shelf cell is safe — then feeds the board's battery input and the carrier rails.
- **Fuel gauge: MAX17048** (LCSC C2682616) — accurate battery % over I²C (ModelGauge, no sense
  resistor, 3 µA).
- **Audio rail is isolated:** CS43131 is fed from **VBAT → LT3042 ultra-low-noise LDO** (LCSC C666568)
  on its **own analog ground island**, star-tied to system ground, *away* from the board's WiFi/
  switching noise — **not** off the board's 3.3 V. This is where audio cleanliness is won or lost.
- **Power-off: both modes.** Short-press → S3 **deep-sleep** (~tens of µA, instant wake via button or
  IMU motion). Long-press → **true hardware latch-off** (soft-latch load switch; zero drain for
  storage). The MCU holds the latch enable and releases it on long-press.

## UI design principles

The panel is tiny — **19.8 × 44.3 mm** (smaller than a stick of gum). The UI leans into that:

- **Glanceable + remote-first.** The headphone remote handles play/skip/volume without looking; the
  IMU wakes the screen; touch is for *glances* and light navigation, not constant poking.
- **Big, sparse touch targets.** A fingertip is ~8–10 mm — half the screen width per button — so
  layouts stay simple with large hit areas. No dense UIs.
- **Browse over search.** A full keyboard on a 20 mm screen is miserable to type on, so we lean on
  list browsing and minimize typing (rethink search input if it's really needed).
- **Orientation: portrait primary** (fits the tall bar + scrolling lists). Landscape is an optional
  *fixed* setting (a now-playing "media widget" layout), **not** dynamic auto-rotate (saves the
  runtime-rotation cost and avoids designing every screen twice).
- Pure-black AMOLED theme (pixels off = power saved). Prototype: `ui-prototype/index.html`.

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

**Design specifics:**
- **Divider:** Vref 3.3 V, **Rbias ≈ 2.2 kΩ** → bands: center ~0 V, Vol+ ~0.33 V, Vol− ~0.58 V,
  no-press ~1.65 V (with mic) / 3.3 V (open). RC filter (~1 kΩ + 100 nF) into the ADC for
  debounce. Drop Rbias to ~1 kΩ if Vol+/Vol− need wider separation.
- ⚠️ **Must use an ADC1 pin (GPIO1–10)** — the S3's **ADC2 is unusable while WiFi is on**, and WiFi
  is always on. Pick a free ADC1 channel off the 1.91 header at pinout time.
- **ESD array** on the jack; keep L/R ESD caps <10 pF so they don't roll off treble.
- **CTIA only** (modern standard); OMTP just won't sense (calibration detects + warns). No
  auto-switch IC — not worth it for a personal device.
- **Jack-detect contact → GPIO** gives **auto-pause on unplug** / resume on re-plug, "headphones
  connected" UI state, and power savings (sense only when plugged).
- **Firmware:** oversample (~16–64 avg) → classify to calibrated band midpoints → ~30 ms debounce →
  events, with press-and-hold repeat for volume and an optional long-press mapping (seek/next).

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
- **Not** part of Sendspin — integrate **Music Assistant's own WebSocket API** (port **8095**) as a
  second protocol client. Reference spec: the official `music-assistant-client` (controllers:
  `music`, `players`, `player_queues`, `auth`) + auto-generated docs at `http://<ma-ip>:8095/api-docs`.
- **Playback loop:** browse via the MA WS API → pick media → `play_media` targeting *our own* player →
  MA streams it back via Sendspin. (Could also control other rooms.)
- **Feasibility verdict (researched):** *feasible on the S3 as a paged "lite browse,"* and it's the
  **heaviest software in the project** + the #1 reason the spike might tip us to the **P4**. Key facts:
  pagination via limit/offset (default 500; we use ~25–50/page); browse is a URI tree
  (`library://artist/1`); **auth required since MA schema v28** (long-lived token).
- **Scope it lite** (80/20): search + playlists + recently-played + paged artists/albums with lazy
  thumbnails — **not** a mirror of the full desktop library tree.
- **Risks & mitigations:** album-art thumbnails while scrolling = the perf killer → lazy-load visible
  rows only, small thumbs, LRU cache in PSRAM/SD, prefetch. **Two simultaneous WebSockets** (Sendspin +
  MA) + LVGL + audio = the P4-escalation driver → the **spike must stress-test exactly this**. Auth
  token onboarding → **paste it into the captive-portal web page** (never type a long token on the
  touch keyboard). Independent reconnect per socket; Tier-1 keeps working if MA-control drops.

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

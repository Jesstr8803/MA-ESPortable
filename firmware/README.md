# MA-ESPortable — firmware

ESP-IDF firmware for the Waveshare **ESP32-S3-AMOLED-1.91 (touch)** board. Most of it runs on the
**bare board** — the carrier (audio/gauge) is only needed for audio output and is added later.

> Status: **scaffold that BUILDS.** Verified to compile clean on ESP-IDF v5.5.4 (esp32s3) with the
> full dependency tree resolved (`sendspin-cpp` 0.6.1, SH8601 2.0.1, LVGL 8.4, esp_lcd_touch,
> esp_websocket_client, mdns). Pin map, WiFi/boot skeleton, and an LVGL UI foundation (theme +
> screen manager + Now Playing) are in place and compiling. Milestone TODOs below are next.
> See `architecture.md` (tasks/cores/state machine + predecessor reuse) and `tier2-ma-api.md`.

## Requirements
- **ESP-IDF v5.5+** (required by `sendspin-cpp`)
- Target: `esp32s3`

## Build / flash
```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```
First build pulls managed components (see `main/idf_component.yml`): `sendspin-cpp`, the SH8601 AMOLED
driver, LVGL, FT3168 touch, mDNS.

## Layout
```
firmware/
  CMakeLists.txt            top-level project
  partitions.csv            dual-OTA, 16 MB
  sdkconfig.defaults        S3R8 + octal PSRAM, big TCP windows, custom partitions
  main/
    CMakeLists.txt
    idf_component.yml        managed dependencies
    board_pins.h            VERIFIED pin map (single source of truth)
    main.cpp                boot flow + milestone TODOs
    wifi_provisioning.{h,cpp}  STA + SoftAP captive portal (skeleton)
```

## Milestones (build order)
All but the last run on the bare board.

1. **Bring-up** — SH8601 QSPI panel + LVGL + FT3168 touch; boot splash.
2. **UI** — port the screens from `../ui-prototype/` to LVGL.
3. **WiFi** — STA from NVS creds; SoftAP captive-portal provisioning; on-LAN config page.
4. **IMU** — QMI8658 wake-on-pickup / orientation; screen-sleep + lock model.
5. **Music Assistant** — Sendspin client (player + metadata + controller + artwork).
6. **The spike** — prove LVGL + WiFi + I²S audio (+ a taste of Tier-2 scrolling/thumbnails) coexist.
   This is the **ESP32-S3 vs ESP32-P4** decision point.
7. **Audio (carrier)** — I²S → CS43131; deep buffer; time-sync gate. *(needs carrier or a temp I²S DAC breakout)*
8. **Tier-2** — Music Assistant WS API browse/search/queue.

Carried-over patterns from the predecessor (`sendspin-xiao`): captive-portal provisioning, OTA,
`/status` metrics, NVS persistence, the deep-buffer + (here) WiFi power-save approach.

## Notes
- microSD is **unused** (assets in flash, art streams to PSRAM).
- WiFi power-save is enabled at runtime (battery device); the deep audio buffer absorbs the jitter —
  unlike the predecessor's mains-tuned `WIFI_PS_NONE`.

# MA-ESPortable — UI prototype

A browser mock of the device screen, used to design the UI **before** porting to LVGL on the ESP32.

- **Open** `index.html` in any browser (double-click — no server needed).
- The device panel is the **real resolution: 240 × 536 px** (ESP32-S3-AMOLED-1.91).
- **Navigable:** Now Playing → **Library (2×2 tile menu: Artists / Albums top, Playlists / Queue bottom)** → list → Album → tap a track to play. Big tiles = easy finger targets. Mini-player bar returns to Now Playing; lists/albums have a back ‹; lock screen = press & hold the ring.
- **Swipe** (touch or mouse-drag): up = into Library, down/right = back. Coarse swipes suit the tiny screen better than hunting for small chevrons.
- **On-screen volume:** Now Playing has a volume slider (tall hit zone — tap anywhere or drag) so headphones *without* inline controls can still set volume.
- **Marquee:** titles that don't fit auto-scroll (try a long track/playlist name). On-device, LVGL does this natively via `LV_LABEL_LONG_SCROLL` — no custom code.
- **Jump-to** buttons (panel) jump straight to Now Playing / Library / Queue / Lock for quick testing.
- **States** (panel): **Boot** splash, **Connecting** to Music Assistant, **Charging** screen, **Settings** (scrollable list — name/brightness/sleep/sync/calibrate-remote/haptics toggle/Wi-Fi/MA/OTA/about), **Setup mode** (SoftAP). Settings is also reachable via the gear in the Library header.
- **Orientation** toggles Portrait (240×536, all screens) vs Landscape (536×240, now-playing "media widget"). Portrait is the recommended primary (tall device + scrolling lists); landscape shines for now-playing. Prefer a fixed orientation chosen at setup over dynamic auto-rotate (perf + 2× design).
- (Search was dropped for v1 — browse-first + search-from-phone; voice search reserved for the future.)
- **Touch sizing:** targets are enlarged for fingers (rows ~76 px ≈ 6 mm, play ~80 px, transport hit-areas 60 px), tiny secondary icons removed (queue/lock reached via swipe/tabs/timeout). The **Touch check** panel shows a draggable **⌀9 mm fingertip** circle — drag it over the UI; anything smaller than the circle is fiddly. Recommended target ≈9 mm (≈109 px on this 308-PPI panel). The real mitigation is leaning on **swipes** (no precision needed) over precise taps.
- **View** modes:
  - *Design 2×* / *1× (px)* — work at native pixels (what matters for the LVGL port).
  - *Physical 1:1* — scales to this monitor (~225 PPI, 2560×1600) so it renders at the true ~19.8 × 44.3 mm size. Because the real panel (~308 PPI) is denser than the monitor, true size looks *small* — that's correct.
- **Calibration:** the real display is hard to hit exactly because of OS scaling / `devicePixelRatio`. Adjust the slider until the on-screen bar measures **50 mm** with a real ruler; Physical 1:1 is then accurate.

## Notes for porting to LVGL
- Each view is one `.screen .view` block built from simple boxes → maps to LVGL screens/containers.
- Pure-black background (`#000`) = true-black on AMOLED (pixels off = power saved).
- Tall 240×536 bar favors vertical layouts (now-playing stack; scrolling lists).
- These screens are mockups (static data); wire to Sendspin metadata (Tier 1) and the MA WS API (Tier 2) during the firmware port.

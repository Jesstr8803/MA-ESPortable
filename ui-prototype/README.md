# MA-ESPortable — UI prototype

A browser mock of the device screen, used to design the UI **before** porting to LVGL on the ESP32.

- **Open** `index.html` in any browser (double-click — no server needed).
- The device panel is the **real resolution: 240 × 536 px** (ESP32-S3-AMOLED-1.91).
- **Screen** buttons switch views: Now Playing, Browse, Search, Lock.
- **View** modes:
  - *Design 2×* / *1× (px)* — work at native pixels (what matters for the LVGL port).
  - *Physical 1:1* — scales to this monitor (~225 PPI, 2560×1600) so it renders at the true ~19.8 × 44.3 mm size. Because the real panel (~308 PPI) is denser than the monitor, true size looks *small* — that's correct.
- **Calibration:** the real display is hard to hit exactly because of OS scaling / `devicePixelRatio`. Adjust the slider until the on-screen bar measures **50 mm** with a real ruler; Physical 1:1 is then accurate.

## Notes for porting to LVGL
- Each view is one `.screen .view` block built from simple boxes → maps to LVGL screens/containers.
- Pure-black background (`#000`) = true-black on AMOLED (pixels off = power saved).
- Tall 240×536 bar favors vertical layouts (now-playing stack; scrolling lists).
- These screens are mockups (static data); wire to Sendspin metadata (Tier 1) and the MA WS API (Tier 2) during the firmware port.

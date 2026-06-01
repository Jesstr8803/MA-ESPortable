# ESP32-S3-AMOLED-1.91 — GPIO cross-reference checklist

Every ESP32-S3 GPIO vs. what the Waveshare **demo source** says it does, with a column to confirm
against the **schematic / board in hand**. Demo source:
[waveshareteam/ESP32-S3-AMOLED-1.91](https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91)
(`02_Example/Arduino/`: `03_LVGL_V8_Test.ino`, `02_I2C_QMI8658/i2c_bsp.cpp`,
`04_SD_Card/sd_card_bsp.cpp`, `01_ADC_Test/adc_bsp.cpp`).

> ⚠️ Demo files are inconsistent — treat as strong-but-unconfirmed until checked against the board.

| GPIO | Demo says | Category | Free for carrier? | ADC1? | Confirmed? |
|---|---|---|---|---|---|
| 0 | BOOT button | Reserved (our button) | no | — | [ ] |
| 1 | Battery sense | Used (ADC) | no | yes | [ ] |
| 2 | — | FREE | yes | yes | [ ] |
| 3 | — | FREE | yes | yes | [ ] |
| 4 | — | FREE | yes | yes | [ ] |
| 5 | AMOLED D3 | Used (display) | no | yes | [ ] |
| 6 | AMOLED CS | Used (display) | no | yes | [ ] |
| 7 | AMOLED D1 | Used (display) | no | yes | [ ] |
| 8 | microSD D0/MISO | Used (SD) † | no* | yes | [ ] |
| 9 | microSD CLK/CS | Used (SD) † | no* | yes | [ ] |
| 10 | — | FREE | yes | yes | [ ] |
| 11 | — | FREE | yes | no | [ ] |
| 12 | — | FREE | yes | no | [ ] |
| 13 | — | FREE | yes | no | [ ] |
| 14 | — | FREE | yes | no | [ ] |
| 15 | — | FREE | yes | no | [ ] |
| 16 | — | FREE | yes | no | [ ] |
| 17 | AMOLED RST | Used (display) | no | no | [ ] |
| 18 | AMOLED D0 | Used (display) | no | no | [ ] |
| 19 | USB D− | Reserved (USB) | no | no | [ ] |
| 20 | USB D+ | Reserved (USB) | no | no | [ ] |
| 21 | — | FREE | yes | no | [ ] |
| 22–25 | (do not exist on S3) | N/A | — | — | — |
| 26–32 | SPI flash | Reserved | no | no | [ ] |
| 33–37 | Octal PSRAM | Reserved | no | no | [ ] |
| 38 | — | FREE | yes | no | [ ] |
| 39 | I²C SCL | Used (touch+IMU) | shared ‡ | no | [ ] |
| 40 | I²C SDA | Used (touch+IMU) | shared ‡ | no | [ ] |
| 41 | — | FREE | yes | no | [ ] |
| 42 | microSD CMD/MOSI | Used (SD) † | no* | no | [ ] |
| 43 | UART0 TX | Reserved (console) | no | no | [ ] |
| 44 | UART0 RX | Reserved (console) | no | no | [ ] |
| 45 | — | FREE | yes | no | [ ] |
| 46 | — | FREE | yes | no | [ ] |
| 47 | AMOLED PCLK | Used (display) | no | no | [ ] |
| 48 | AMOLED D2 | Used (display) | no | no | [ ] |

**Legend / notes**
- **†** SD pins (8, 9, 42). **\*** = freed if microSD is left unpopulated (we don't need it — audio is
  over WiFi). Dropping SD frees **8 & 9 (both ADC1)** + 42.
- **‡** I²C (39/40) is *shared*, not blocked — the carrier's CS43131 / DRV2605L / MAX17048 join this
  same bus (that's the plan).

**Free GPIOs (demo):** 2, 3, 4, 10, 11, 12, 13, 14, 15, 16, 21, 38, 41, 45, 46  (+ 8, 9, 42 if SD unused).
**ADC1-capable free:** 2, 3, 4, 10  (+ 8, 9 if SD unused).

## Specifically verify against the schematic
- [ ] **SD vs display overlap** — the SD demo's SPI mode uses CLK=47, which is also the LCD PCLK. How is SD actually wired (SDMMC-only? muxed?).
- [ ] **QMI8658 bus** — example folder is `02_I2C_QMI8658` (I²C, addr 0x6A/0x6B) but `qmi8658c.h` has `#define QMI8658_USE_SPI`. Confirm it's on the I²C bus (39/40).
- [ ] **Touch INT/RST and IMU INT** pins — not present in the demo files pulled.
- [ ] **Header breakout** — confirm each "FREE" GPIO is actually exposed on the 2×20 headers.
- [ ] **Battery sense (GPIO1)** — confirm it's the LiPo divider, not just a test pad.

## Tentative carrier assignment (fill in once confirmed)
| Carrier signal | Proposed GPIO | Confirmed GPIO |
|---|---|---|
| I²S MCLK | 41 | |
| I²S BCLK | 42 (if SD dropped) / 38 | |
| I²S LRCK | 45 | |
| I²S SDOUT → DAC | 46 | |
| I²C SDA / SCL (shared) | 40 / 39 | |
| Remote-sense ADC (ADC1) | 2 | |
| Jack-detect | 16 | |
| DRV2605L EN | 38 | |
| MAX17048 ALRT | 21 | |
| Future mic ADC (ADC1) | 3 | |

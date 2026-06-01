# ESP32-S3-AMOLED-1.91 — GPIO cross-reference

**VERIFIED from the board schematic** (`hardware/reference/ESP32-S3-AMOLED-1.91.pdf`, legend table),
extracted with coordinates so column assignments are reliable. This supersedes the earlier
demo-source guesses.

| GPIO | Schematic function | Category | Free for carrier? | ADC1? |
|---|---|---|---|---|
| 0 | BOOT button | Reserved (our button) | no | — |
| 1 | BAT_ADC (battery sense) | Used | no | yes |
| 2 | — | **FREE** | yes | **yes** |
| 3 | — | **FREE** | yes | **yes** |
| 4 | — | **FREE** | yes | **yes** |
| 5 | AMOLED DB1 | Used (display) | no | yes |
| 6 | AMOLED CS | Used (display) | no | yes |
| 7 | AMOLED DCX_RS | Used (display) | no | yes |
| 8 | AMOLED SDO **+ SD_MISO** | Used (display/SD muxed) | no | yes |
| 9 | AMOLED TE **+ SD_CS** | Used (display/SD muxed) | no | yes |
| 10 | — | **FREE** | yes | **yes** |
| 11 | — | **FREE** | yes | no |
| 12 | — | **FREE** | yes | no |
| 13 | — | **FREE** | yes | no |
| 14 | — | **FREE** | yes | no |
| 15 | — | **FREE** | yes | no |
| 16 | — | **FREE** | yes | no |
| 17 | AMOLED RESET | Used (display) | no | no |
| 18 | AMOLED RD_SDI | Used (display) | no | no |
| 19 | USB D− | Reserved (USB) | no | no |
| 20 | USB D+ | Reserved (USB) | no | no |
| 21 | — | **FREE** | yes | no |
| 22–25 | (do not exist on S3) | N/A | — | — |
| 26, 33–37 | SPI flash + octal PSRAM | Reserved | no | no |
| 38 | — | **FREE** | yes | no |
| 39 | TP_SCL **+ IMU_SCL** | Used (shared I²C ‡) | shared | no |
| 40 | TP_SDA **+ IMU_SDA** | Used (shared I²C ‡) | shared | no |
| 41 | AMOLED TP_INT (touch interrupt) | Used | no | no |
| 42 | SD_MOSI | Used (SD) † | (no*) | no |
| 43 | U0TXD | Reserved (console) | no | no |
| 44 | U0RXD | Reserved (console) | no | no |
| 45 | IMU_INT1 | Used (IMU) | no | no |
| 46 | IMU_INT2 | Used (IMU) | no | no |
| 47 | AMOLED WRX_SCL **+ SD_CLK** | Used (display/SD muxed) | no | no |
| 48 | AMOLED DB0 | Used (display) | no | no |

**Legend / notes**
- **‡** I²C bus = **SDA 40 / SCL 39** (note the order), shared by the touch (FT3168) and IMU
  (QMI8658). Our carrier's CS43131 / DRV2605L / MAX17048 join this same bus — that's the plan.
- **IMU is on I²C** (39/40) with interrupts on 45/46. (The demo's `QMI8658_USE_SPI` define is a
  red herring; the schematic wires it I²C.)
- **†/*** SD is **muxed with the display** (SD_MISO=8, SD_CS=9, SD_CLK=47 are shared AMOLED pins;
  only SD_MOSI=42 is SD-exclusive). So **not** using microSD frees **only GPIO42** — not 8/9/47.

**Genuinely FREE GPIOs:** **2, 3, 4, 10, 11, 12, 13, 14, 15, 16, 21, 38** (+ **42** if microSD unused).
**ADC1-capable free (for remote-sense):** **2, 3, 4, 10**.
That's 12 free pins — plenty for I²S(4) + remote-sense ADC(1) + jack-detect(1) + haptic-EN(1) +
gauge-ALRT(1) = 8.

## Still to confirm on the physical board
- [ ] Each FREE GPIO is actually broken out on the 2×20 headers (vs. only on internal pads).
- [ ] The physical header pin positions (fill the map below from the wiki pinout image / silkscreen).

## Physical header map — VERIFIED from the board

40-pin "Pico-style" header (numbered 1–40). Confirmed: **every genuinely-free GPIO is broken out**,
including all 4 free ADC1 pins and the I²C bus.

| Pin | Signal | | Pin | Signal |
|---|---|---|---|---|
| 1 | GPIO43 (U0TX) | | 21 | GPIO34 (PSRAM) |
| 2 | GPIO44 (U0RX) | | 22 | GPIO35 (PSRAM) |
| 3 | GND | | 23 | GND |
| 4 | GPIO0 (BOOT) | | 24 | GPIO36 (PSRAM) |
| 5 | GPIO1 (BAT_ADC) | | 25 | GPIO37 (PSRAM) |
| **6** | **GPIO2 — FREE (ADC1)** | | **26** | **GPIO38 — FREE** |
| **7** | **GPIO3 — FREE (ADC1)** | | 27 | GPIO39 (**I²C SCL**) |
| 8 | GND | | 28 | GND |
| **9** | **GPIO11 — FREE** | | 29 | GPIO40 (**I²C SDA**) |
| **10** | **GPIO12 — FREE** | | 30 | CHIP_UP (charge status?) |
| **11** | **GPIO13 — FREE** | | **31** | **GPIO4 — FREE (ADC1)** |
| **12** | **GPIO14 — FREE** | | **32** | **GPIO10 — FREE (ADC1)** |
| 13 | GND | | 33 | GND |
| **14** | **GPIO15 — FREE** | | **34** | **GPIO16 — FREE** |
| 15 | GPIO19 (USB) | | 35 | GPIO17 (AMOLED RST) |
| 16 | GPIO20 (USB) | | 36 | 3V3 |
| **17** | **GPIO21 — FREE** | | 37 | 3V3_EN |
| 18 | GND | | 38 | GND |
| 19 | GPIO26 (PSRAM/flash) | | 39 | VSYS (system rail, post-charger) |
| 20 | GPIO33 (PSRAM) | | 40 | VBUS (5 V from USB) |

**Power available on header:** 3V3 (pin 36), **VSYS** (pin 39 — post-charger battery rail, likely the
cleanest VBAT tap for the LT3042 audio LDO → carrier may not need separate battery wiring), VBUS/5 V
(pin 40), GND ×8.
**Note:** GPIO42 (SD_MOSI) is *not* on the header (irrelevant — SD unused). CHIP_UP (pin 30) is
likely a charge-status line — possibly useful for the charging-screen UI (confirm from schematic).

## Carrier → header pin assignment (FINAL pins, header-confirmed)

| Carrier signal | GPIO | Header pin |
|---|---|---|
| I²S MCLK | 11 | 9 |
| I²S BCLK | 12 | 10 |
| I²S LRCK | 13 | 11 |
| I²S SDOUT → DAC | 14 | 12 |
| I²C SDA / SCL (shared) | 40 / 39 | 29 / 27 |
| Remote-sense ADC (ADC1) | 2 | 6 |
| Future mic ADC (ADC1) | 3 | 7 |
| Jack-detect | 15 | 14 |
| DRV2605L EN | 16 | 34 |
| MAX17048 ALRT | 21 | 17 |
| Spare (ADC1) | 4, 10 | 31, 32 |
| Spare | 38 | 26 |
| Power | 3V3 / VSYS / GND | 36 / 39 / many |

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

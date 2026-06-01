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

## Physical 2×20 header map — TO FILL IN FROM THE BOARD

> The header pin *positions* (which pad each GPIO sits on) are only in Waveshare's pinout **image** /
> schematic, which I couldn't fetch. Fill these from the board silkscreen or the wiki pinout diagram.
> The board is "Pico-header compatible" (two 1×20 rows, 2.54 mm pitch).

**Header 1 (left row)**

| Hdr pin | Silkscreen / GPIO | Hdr pin | Silkscreen / GPIO |
|---|---|---|---|
| 1 |  | 2 |  |
| 3 |  | 4 |  |
| 5 |  | 6 |  |
| 7 |  | 8 |  |
| 9 |  | 10 |  |
| 11 |  | 12 |  |
| 13 |  | 14 |  |
| 15 |  | 16 |  |
| 17 |  | 18 |  |
| 19 |  | 20 |  |

**Header 2 (right row)**

| Hdr pin | Silkscreen / GPIO | Hdr pin | Silkscreen / GPIO |
|---|---|---|---|
| 1 |  | 2 |  |
| 3 |  | 4 |  |
| 5 |  | 6 |  |
| 7 |  | 8 |  |
| 9 |  | 10 |  |
| 11 |  | 12 |  |
| 13 |  | 14 |  |
| 15 |  | 16 |  |
| 17 |  | 18 |  |
| 19 |  | 20 |  |

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

# MA-ESPortable — Carrier board design

A carrier/shield PCB that mounts on the **Waveshare ESP32-S3-AMOLED-1.91 (touch)** via its 2×20
headers, and also holds the battery. It adds the audio output stage, haptics, fuel gauge, battery
protection, and the headphone jack + inline-remote sense.

> Status: **in progress.** This is the working electrical design doc — BOM, nets, pin map, layout.
> Exact ESP32 GPIO numbers are marked *(confirm from board)* until verified against the board's
> demo pin map / schematic.

## Design decisions driving the BOM

- **DAC + headphone amp: Cirrus CS43131** (130 dB DR, ground-centered Class-H, drives IEMs→600 Ω).
- It runs on **1.8 V logic**, but the ESP32-S3 GPIO is **3.3 V** → the I²S and I²C lines need
  level translation:
  - **SN74AXC4T245** — 4-bit level shifter for I²S (MCLK/BCLK/LRCK/SDIN), 3.3 V ↔ 1.8 V.
  - **PCA9306** — I²C translator for the CS43131's SDA/SCL only (DRV2605L + MAX17048 stay on the
    native 3.3 V bus).
- **Ground-centered output = the headphone return (TRRS ring-2) is *real system ground*** (charge
  pump gives a negative rail; L/R swing around true GND). No DC-blocking caps in the L/R path, and
  it's compatible with the system-ground-referenced mic-sense divider.

## Bill of materials — ICs

| Ref | Part | Function | Pkg | LCSC |
|---|---|---|---|---|
| U1 | **CS43131** | DAC + ground-centered headphone amp | QFN-40 5×5 | C1554754 / C1554759 |
| U2 | **DRV2605L** | Haptic driver (LRA), I²C | VSSOP-10 | C527464 |
| U3 | **MAX17048** | Battery fuel gauge (ModelGauge), I²C | — | C2682616 |
| U4 | **LT3042** | Ultra-low-noise LDO → 1.8 V audio rail | DFN | C666568 |
| U5 | **DW01A** | 1-cell Li-ion protection | SOT-23-6 | C700964 |
| Q1 | **FS8205A** | Dual N-MOSFET for protection | SOT-23-6 | *(confirm)* |
| U6 | **SN74AXC4T245** | 4-bit level shifter (I²S) | — | *(confirm)* |
| U7 | **PCA9306** | I²C voltage translator | — | *(confirm)* |
| U8 | *(DNP)* MEMS mic | Future voice (reserved footprint) | — | — |

## Bill of materials — passives & connectors

- **CS43131:** FLYP/FLYN flying caps **1 µF ×2**; charge-pump output cap **1 µF**; per-rail decoupling
  **100 nF** on each supply (VA/VD/VL/VHP) + **1–10 µF** bulk; optional series ferrite + small cap on
  HPOUTL/R for EMI.
- **LT3042:** **R_set = 18 kΩ** (0.1 % for accuracy) → 1.8 V; SET cap **4.7 µF** (ultra-low noise);
  C_in **1–10 µF**, C_out **10 µF** low-ESR; EN pull-up.
- **DRV2605L:** VDD **100 nF + 1 µF**; OUT± to LRA; EN to GPIO or pulled high.
- **MAX17048:** VDD **1 µF**; CELL sense to VBAT (optional RC); ALRT → GPIO (optional low-batt int).
- **Protection (DW01A + Q1):** per datasheet — ~**100–470 Ω** + **100 nF** on VDD; FETs in the cell
  B− path.
- **Remote-sense:** **R_bias 2.2 kΩ** (MIC→3.3 V); RC filter **1 kΩ + 100 nF** into the ADC.
- **Reserved mic tap (DNP):** AC-couple **0.1 µF** + preamp footprint → ADC1.
- **ESD:** low-capacitance ESD array on the TRRS lines (**<10 pF** on L/R so treble isn't rolled off).
- **Level shifters:** 100 nF per rail (3.3 V and 1.8 V sides).
- **I²C pull-ups:** rely on the board's existing bus pull-ups; only add 4.7 kΩ if the bus is weak.
- **Connectors:** TRRS jack (4-pole, **switched/detect**); LRA pads/connector; battery holder/JST;
  2×20 female headers to the board.

## Power tree (rails)

```
CELL ──[DW01A + Q1 protection]── VBAT ──┬──► board MX1.25 battery input (board charges + power-paths)
                                        ├──► MAX17048 CELL sense
                                        └──► LT3042 (R_set 18k) ──► 1.8V_A (clean) ──► CS43131 rails
3V3 (from board header) ──► remote-sense bias, level-shifter HIGH side, DRV2605L, MAX17048, PCA9306 hi
1.8V_A ──► CS43131 (VA/VD/VL), level-shifter LOW side, PCA9306 low side
```
- CS43131 audio rail is its **own analog island** off the LT3042, **star-grounded** to system GND,
  away from the ESP32/WiFi/QSPI-display switching noise.
- The level shifters' 1.8 V side is fed from the same 1.8 V_A (so DAC + its logic share one clean
  domain).

## Net connections by subsystem

**Audio (I²S):** ESP32 [MCLK,BCLK,LRCK,SDOUT] → **U6 SN74AXC4T245** → CS43131 [MCLK,SCLK,LRCK,SDIN].
**Audio (control):** 3.3 V I²C bus → **U7 PCA9306** → CS43131 [SDA,SCL] @1.8 V.
**Audio out:** CS43131 HPOUTL→TRRS Tip(L), HPOUTR→TRRS Ring1(R), HP return/GND→TRRS Ring2 = system GND.
**Haptic:** 3.3 V I²C → DRV2605L; DRV2605L OUT± → LRA.
**Fuel gauge:** 3.3 V I²C → MAX17048; CELL→VBAT; ALRT→GPIO *(optional)*.
**Battery:** CELL → DW01A/Q1 → VBAT (to board + LT3042).
**TRRS jack:** L/R/GND as above; **MIC line** → R_bias 2.2 k to 3.3 V + RC (1 k/100 nF) → ADC1; plus
DNP AC-tap for future voice; ESD array on all four lines; **detect switch** → GPIO.

## Board pin usage — from the official demo source

Read from [waveshareteam/ESP32-S3-AMOLED-1.91](https://github.com/waveshareteam/ESP32-S3-AMOLED-1.91)
`02_Example/Arduino/` (`03_LVGL_V8_Test.ino`, `02_I2C_QMI8658/i2c_bsp.cpp`, `01_ADC_Test/adc_bsp.cpp`).
Display driver **SH8601-class** QSPI.

> ⚠️ **Treat as strong-but-unconfirmed.** Waveshare's example files are inconsistent between demos, so
> these are the best source values but the **final authority is the board schematic / the board in
> hand**. Confirm every number (and that each pin is broken out on the 2×20 header) before fab.

**Used by the board — do not reuse:**
- **AMOLED QSPI:** CS=**6**, PCLK=**47**, D0=**18**, D1=**7**, D2=**48**, D3=**5**, RST=**17** (no backlight pin)
- **I²C (touch FT3168 + IMU QMI8658), I2C_NUM_0:** SDA=**40**, SCL=**39**
- **Battery voltage sense:** **GPIO1** (ADC1_CH0) — the ADC demo reads this *(confirm it's the LiPo divider)*
- **Reserved by silicon:** **26–37** (SPI flash + octal PSRAM), **0** (BOOT/our button), **19/20** (USB), **43/44** (UART0)

**Free ADC1-capable (GPIO1–10; ADC2 unusable with WiFi):** **2, 3, 4, 8, 9, 10** (1=batt, 5/6/7=LCD).
**Free digital:** **13, 14, 15, 16, 21, 38, 41, 42, 45, 46** (+ any unused ADC1 pins).
Plenty of headroom. *(All subject to the header-breakout + schematic confirmation above.)*

## ESP32 header pins — carrier assignment *(tentative)*

| Signal | GPIO | Constraint / notes |
|---|---|---|
| I²S MCLK | 41 | mandatory; via U6 level shifter |
| I²S BCLK | 42 | via U6 |
| I²S LRCK | 45 | via U6 |
| I²S SDOUT (→DAC SDIN) | 46 | via U6 |
| I²C SDA / SCL | **40 / 39** | board's shared bus; +DRV2605L/MAX17048; +PCA9306→CS43131 |
| **Remote-sense ADC** | **2** (ADC1_CH1) | **must be ADC1** (ADC2 dead w/ WiFi) |
| Jack-detect | 16 | TRRS detect switch → auto-pause on unplug |
| DRV2605L EN | 38 | optional (or tie high) |
| MAX17048 ALRT | 21 | optional low-battery interrupt |
| Future mic ADC | 3 (ADC1_CH2) | reserved (DNP) |
| 3V3 / GND | power | from header |
| VBAT | carrier-internal | cell → protection (not from header) |

**Battery gauge note:** the board already senses battery voltage on **GPIO1** — so a basic %-from-voltage
readout is possible *without* the MAX17048. We keep MAX17048 for accurate ModelGauge % under load;
dropping it (use GPIO1) is a valid parts-saving if accuracy isn't critical. *(Confirm GPIO1 is the LiPo
divider, not just a test pad.)*

## Layout guidance

- **Two ground domains, single-point (star) tie:** quiet **analog GND** (CS43131, LT3042, HP return,
  flying caps) vs **digital/system GND** (ESP32 header, level shifters, I²C, haptic). Join at one
  point near the LT3042/CS43131 ground.
- Keep the **LT3042 + CS43131 + flying caps** tight together; short, wide analog traces.
- Route **I²S away from** the audio output and analog rail; level shifters near the header.
- **TRRS jack**: HP L/R short and matched; mic-sense tap off a *quiet* ground point; ESD array right
  at the jack.
- Keep the **DAC/analog section away** from the ESP32 antenna/WiFi and the QSPI display traces.
- LRA mounting is mechanical (chassis-bonded) — see enclosure; only the drive pads are on the PCB.

## Open items

- Confirm exact free GPIOs (and a free ADC1 pin) from the board demo/schematic.
- Confirm LCSC part numbers for SN74AXC4T245, PCA9306, FS8205A, ESD array, TRRS jack, LRA.
- Decide LRA part (size vs enclosure) and battery cell (deferred).
- 2-layer vs 4-layer (lean 4-layer for clean ground near WiFi; revisit on cost).
- Schematic capture → Gerbers + JLCPCB assembly BOM/CPL.

# MA-ESPortable — Carrier board design

A carrier/shield PCB that mounts on the **Waveshare ESP32-S3-AMOLED-1.91 (touch)** via its 2×20
headers, and also holds the battery. It adds the audio output stage, fuel gauge, battery
protection, and the headphone jack + inline-remote sense.

> Status: **in progress.** This is the working electrical design doc — BOM, nets, pin map, layout.
> Exact ESP32 GPIO numbers are marked *(confirm from board)* until verified against the board's
> demo pin map / schematic.

## Build philosophy: design to full spec, populate selectively

The PCB is designed and routed for **every** feature; parts we don't need for a given build are
left as **unpopulated footprints (DNP)**. A bare board costs the same regardless, and a basic build
can be upgraded to full spec later with just a soldering iron — no respin. Optional/DNP candidates:
- **DW01A + FS8205A** protection — omit if using a protected pouch (populate for a bare cell)
- **MAX17048** fuel gauge — omit and use the board's GPIO1 battery ADC for a basic % readout
- **MEMS mic + headset-mic AC tap** — future voice; footprints only
- Anything else marked *(opt)* / *(DNP)* in the BOM

*(Haptics dropped entirely — not even a footprint: no LRA on LCSC breaks reproducibility, draws
battery for pure polish, adds an enclosure mounting constraint.)*

## Design decisions driving the BOM

- **DAC + headphone amp: Cirrus CS43131** (130 dB DR, ground-centered Class-H, drives IEMs→600 Ω).
- It runs on **1.8 V logic**, but the ESP32-S3 GPIO is **3.3 V** → the I²S and I²C lines need
  level translation:
  - **SN74AXC4T245** — 4-bit level shifter for I²S (MCLK/BCLK/LRCK/SDIN), 3.3 V ↔ 1.8 V.
  - **PCA9306** — I²C translator for the CS43131's SDA/SCL only (MAX17048 stays on the
    native 3.3 V bus).
- **Ground-centered output = the headphone return (TRRS ring-2) is *real system ground*** (charge
  pump gives a negative rail; L/R swing around true GND). No DC-blocking caps in the L/R path, and
  it's compatible with the system-ground-referenced mic-sense divider.

## Bill of materials — ICs

| Ref | Part | Function | Pkg | LCSC |
|---|---|---|---|---|
| U1 | **CS43131** | DAC + ground-centered headphone amp | QFN-40 5×5 | C1554754 / C1554759 |
| U3 | **MAX17048** | Battery fuel gauge (ModelGauge), I²C | — | C2682616 |
| U4 | **LT3042** | Ultra-low-noise LDO → 1.8 V audio rail | DFN | C666568 |
| U5 | *(opt)* DW01A | 1-cell protection — **omit if pouch is protected** | SOT-23-6 | C700964 |
| Q1 | *(opt)* FS8205A | Dual N-MOSFET for protection (with U5) | SOT-23-6 | *(confirm)* |
| U6 | **SN74AXC4T245** | 4-bit level shifter (I²S) | — | *(confirm)* |
| U7 | **PCA9306** | I²C voltage translator | — | *(confirm)* |
| U8 | *(DNP)* MEMS mic | Future voice (reserved footprint) | — | — |

## Bill of materials — passives & connectors

- **CS43131** (verified from datasheet Typical Connection Diagram, p10):
  - **VCP (charge pump) = battery 3.0–5.25 V direct** → feed from **VSYS** (header pin 39), *not* 1.8 V.
  - **VA / VL / VD = +1.8 V** ← from the LT3042.
  - Flying caps **2.2 µF** (FLYP/FLYN nodes), VCP_FILT± **2.2 µF**, VCP reservoir **4.7 µF**,
    **FILT± = 15 µF**, decoupling **0.1 µF** on VL/VD/VA.
  - **ADR pin → GND** sets I²C address **0x30** (last two bits 00). ✅
  - Headphone out: **HPOUTA/B direct to TRRS L/R**, **HPREFA/B = ground ref** (ground-centered, no DC
    blocking caps). MCLK external on XTI/MCLK ← our I²S MCLK.
- **LT3042:** **R_set = 18 kΩ** (0.1 % for accuracy) → 1.8 V; SET cap **4.7 µF** (ultra-low noise);
  C_in **1–10 µF**, C_out **10 µF** low-ESR; EN pull-up.
- **MAX17048:** VDD **1 µF**; CELL sense to VBAT (optional RC); ALRT → GPIO (optional low-batt int).
- **Protection (DW01A + Q1):** per datasheet — ~**100–470 Ω** + **100 nF** on VDD; FETs in the cell
  B− path.
- **Remote-sense:** **R_bias 2.2 kΩ** (MIC→3.3 V); RC filter **1 kΩ + 100 nF** into the ADC.
- **Reserved mic tap (DNP):** AC-couple **0.1 µF** + preamp footprint → ADC1.
- **ESD:** low-capacitance ESD array on the TRRS lines (**<10 pF** on L/R so treble isn't rolled off).
- **Level shifters:** 100 nF per rail (3.3 V and 1.8 V sides).
- **I²C pull-ups:** rely on the board's existing bus pull-ups; only add 4.7 kΩ if the bus is weak.
- **Connectors:** TRRS jack (4-pole, **switched/detect**); battery JST (to board); 2×20 female
  headers to the board.

## Power tree (rails)

```
VSYS (header pin 39, raw battery rail) ──┬──► LT3042 (R_set 18k) ──► 1.8V_A (clean) ──► CS43131 VA/VD/VL
                                         └──► CS43131 VCP (charge pump, wants 3.0–5.25V direct)
3V3 (header pin 36) ──► remote-sense bias, level-shifter HIGH side, MAX17048, PCA9306 hi side
1.8V_A ──► level-shifter LOW side, PCA9306 low side
(Battery itself plugs into the BOARD's MX1.25 input; board has the charger + power-path → VSYS.)
```
- CS43131 audio rail is its **own analog island** off the LT3042, **star-grounded** to system GND,
  away from the ESP32/WiFi/QSPI-display switching noise.
- The level shifters' 1.8 V side is fed from the same 1.8 V_A (so DAC + its logic share one clean
  domain).

## Net connections by subsystem

**Audio (I²S):** ESP32 [MCLK,BCLK,LRCK,SDOUT] → **U6 SN74AXC4T245** → CS43131 [MCLK,SCLK,LRCK,SDIN].
**Audio (control):** 3.3 V I²C bus → **U7 PCA9306** → CS43131 [SDA,SCL] @1.8 V.
**Audio out:** CS43131 HPOUTL→TRRS Tip(L), HPOUTR→TRRS Ring1(R), HP return/GND→TRRS Ring2 = system GND.
**Fuel gauge:** 3.3 V I²C → MAX17048; CELL→VBAT; ALRT→GPIO *(optional)*.
**Battery:** CELL → DW01A/Q1 → VBAT (to board + LT3042).
**TRRS jack:** L/R/GND as above; **MIC line** → R_bias 2.2 k to 3.3 V + RC (1 k/100 nF) → ADC1; plus
DNP AC-tap for future voice; ESD array on all four lines; **detect switch** → GPIO.

## Board pin usage — VERIFIED from the schematic

From `hardware/reference/ESP32-S3-AMOLED-1.91.pdf` (legend table, coordinate-extracted). Full table:
`hardware/pinout-crossref.md`.

**Used by the board — do not reuse:**
- **AMOLED (QSPI):** CS=6, DB1=5, DCX_RS=7, SDO=8, TE=9, RST=17, RD_SDI=18, WRX_SCL=47, DB0=48
- **I²C (touch FT3168 + IMU QMI8658):** **SDA=40, SCL=39** (note order). IMU interrupts: 45/46.
- **microSD (muxed with display):** SD_MISO=8, SD_CS=9, SD_CLK=47 (shared AMOLED pins), SD_MOSI=42
- **Battery sense:** GPIO1 (ADC1_CH0)
- **Reserved by silicon:** 26/33–37 (flash+PSRAM), 0 (BOOT/our button), 19/20 (USB), 43/44 (UART0)

**Genuinely FREE:** **2, 3, 4, 10, 11, 12, 13, 14, 15, 16, 21, 38** (all broken out on the header).
**Free ADC1 (for remote-sense): 2, 3, 4, 10.** 12 free pins — ample.

**microSD: unused.** UI assets compile into the 16 MB flash; album art streams live from MA (cached
in PSRAM); no local music. SD is muxed with the display bus and SD_MOSI (42) isn't on the header
anyway — firmware simply doesn't init it. One less thing.

## ESP32 header pins — carrier assignment *(FINAL, header-confirmed)*

Header positions verified from the board's 40-pin map (see `pinout-crossref.md`).

| Signal | GPIO | Header pin | Notes |
|---|---|---|---|
| I²S MCLK | 11 | 9 | mandatory; via U6 level shifter |
| I²S BCLK | 12 | 10 | via U6 |
| I²S LRCK | 13 | 11 | via U6 |
| I²S SDOUT (→DAC SDIN) | 14 | 12 | via U6 |
| I²C SDA / SCL | **40 / 39** | 29 / 27 | board's shared bus; +MAX17048; +PCA9306→CS43131 |
| **Remote-sense ADC** | **2** (ADC1) | 6 | **must be ADC1** (ADC2 dead w/ WiFi) |
| Future mic ADC | 3 (ADC1) | 7 | reserved (DNP) |
| Jack-detect | 15 | 14 | TRRS detect switch → auto-pause on unplug |
| MAX17048 ALRT | 21 | 17 | optional low-battery interrupt |
| spare (ADC1) | 4, 10 | 31, 32 | headroom |
| spare | 16, 38 | 34, 26 | headroom |
| Power | 3V3 / VSYS / GND | 36 / 39 / many | VSYS = post-charger rail |

**Power source CONFIRMED:** the board's "switching of power" is an auto power-path — BAT via an
**APM2307 P-FET** OR USB-VCC via a **1N4148** (OR-ing/gate ref, not main current path) → **VSYS**. So
VSYS (pin 39) is the raw system rail: battery (3.0–4.2 V) on battery, USB-derived when plugged — always
> the LT3042's ~2.15 V min for 1.8 V out. **Carrier draws VSYS + 3V3 + GND from the header**; the
LT3042's ~79 dB PSRR scrubs charger/USB noise. No separate battery wire on the carrier.

**Protection likely NOT needed on carrier:** battery plugs into the board's MX1.25 (board has the
PL4054 charger). Most off-the-shelf LiPo pouches with a connector **already include a protection PCB**,
so we can drop the DW01A + FS8205A from the carrier BOM — *just buy a protected pouch* (confirm at
purchase). Keep them only if using a bare/unprotected cell.

## I²C bus — address map (no collisions)

The carrier's 3 chips share the board's I²C bus (SDA 40 / SCL 39) with the existing touch + IMU:

| Device | 7-bit addr | Notes |
|---|---|---|
| FT3168 touch | 0x38 | existing (FocalTech standard — confirm in touch demo) |
| QMI8658 IMU | 0x6A / 0x6B | existing (from demo + datasheet) |
| CS43131 DAC | 0x30 (0x31 if AD0 high) | ours |
| MAX17048 gauge | **0x36 (fixed)** | ours — no address pins |

All distinct → **no collisions**. The two fixed addresses (0x5A, 0x36) are unoccupied by anything else.
Bus loading: 5 devices total — the board's existing pull-ups are likely fine; add stronger pull-ups
only if the extended bus is marginal.

**Battery gauge note:** the board senses battery voltage on **GPIO1** — so a basic %-from-voltage
readout is possible *without* the MAX17048. We keep MAX17048 for accurate ModelGauge % under load;
dropping it (use GPIO1) is a valid parts-saving if accuracy isn't critical.

## Layout guidance

- **Two ground domains, single-point (star) tie:** quiet **analog GND** (CS43131, LT3042, HP return,
  flying caps) vs **digital/system GND** (ESP32 header, level shifters, I²C). Join at one
  point near the LT3042/CS43131 ground.
- Keep the **LT3042 + CS43131 + flying caps** tight together; short, wide analog traces.
- Route **I²S away from** the audio output and analog rail; level shifters near the header.
- **TRRS jack**: HP L/R short and matched; mic-sense tap off a *quiet* ground point; ESD array right
  at the jack.
- Keep the **DAC/analog section away** from the ESP32 antenna/WiFi and the QSPI display traces.

## Open items

- Confirm exact free GPIOs (and a free ADC1 pin) from the board demo/schematic.
- Confirm LCSC part numbers for SN74AXC4T245, PCA9306, FS8205A, ESD array, TRRS jack.
- Decide battery cell (deferred).
- 2-layer vs 4-layer (lean 4-layer for clean ground near WiFi; revisit on cost).
- Schematic capture → Gerbers + JLCPCB assembly BOM/CPL.

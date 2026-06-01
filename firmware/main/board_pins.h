// board_pins.h — MA-ESPortable / Waveshare ESP32-S3-AMOLED-1.91 (touch)
//
// Pin map VERIFIED from the board schematic + 40-pin header map.
// See hardware/pinout-crossref.md and hardware/carrier-design.md for the full table.
// Display driver: SH8601-class AMOLED, QSPI. Touch: FT3168 (I2C). IMU: QMI8658 (I2C).
#pragma once

// ---------------------------------------------------------------------------
// On-board peripherals (fixed by the Waveshare board — do not reassign)
// ---------------------------------------------------------------------------

// AMOLED display (QSPI)
#define PIN_LCD_CS        6
#define PIN_LCD_DB1       5    // QSPI data
#define PIN_LCD_DCX_RS    7
#define PIN_LCD_SDO       8    // (muxed w/ SD_MISO — SD unused)
#define PIN_LCD_TE        9    // (muxed w/ SD_CS — SD unused)
#define PIN_LCD_RST       17
#define PIN_LCD_RD_SDI    18
#define PIN_LCD_WRX_SCL   47   // QSPI clock (muxed w/ SD_CLK — SD unused)
#define PIN_LCD_DB0       48
#define LCD_H_RES         536
#define LCD_V_RES         240

// Shared I2C bus (touch FT3168 + IMU QMI8658 + carrier chips)
#define PIN_I2C_SDA       40   // header pin 29
#define PIN_I2C_SCL       39   // header pin 27
#define I2C_PORT          I2C_NUM_0

// Touch (FT3168) — interrupt on dedicated pin, shares the I2C bus above
#define PIN_TOUCH_INT     41
#define I2C_ADDR_TOUCH    0x38

// IMU (QMI8658) — on the I2C bus, interrupts dedicated
#define PIN_IMU_INT1      45
#define PIN_IMU_INT2      46
#define I2C_ADDR_IMU      0x6A   // (0x6B alt)

// Battery voltage ADC (board's divider)
#define PIN_BAT_ADC       1      // ADC1_CH0

// Button / power
#define PIN_BOOT_BTN      0      // short=sleep/lock, long=deep-sleep off, very-long=reboot

// ---------------------------------------------------------------------------
// Carrier board (mounts on the 2x20 header) — our additions
// "Full-spec board, populate selectively": some are DNP in a given build.
// ---------------------------------------------------------------------------

// I2S to CS43131 DAC (through SN74AXC4T245 level shifter, 3.3V<->1.8V)
#define PIN_I2S_MCLK      11   // header pin 9
#define PIN_I2S_BCLK      12   // header pin 10
#define PIN_I2S_LRCK      13   // header pin 11
#define PIN_I2S_DOUT      14   // header pin 12  -> DAC SDIN

// I2C addresses of carrier chips (share PIN_I2C_SDA/SCL via PCA9306 for the DAC)
#define I2C_ADDR_DAC      0x30   // CS43131 (0x31 if AD0 high)
#define I2C_ADDR_HAPTIC   0x5A   // DRV2605L (fixed)
#define I2C_ADDR_GAUGE    0x36   // MAX17048 (fixed)

// Analog front-end / control
#define PIN_REMOTE_ADC    2      // ADC1_CH1  — headphone inline-remote sense (header pin 6)
#define PIN_MIC_ADC       3      // ADC1_CH2  — future voice (DNP) (header pin 7)
#define PIN_JACK_DETECT   15     // TRRS detect switch -> auto-pause (header pin 14)
#define PIN_HAPTIC_EN     16     // DRV2605L EN (header pin 34)
#define PIN_GAUGE_ALRT    21     // MAX17048 ALRT, low-batt int (header pin 17)

// Spare, broken out, free for future use: GPIO4 (pin31, ADC1), GPIO10 (pin32, ADC1), GPIO38 (pin26)

// Power rails from the header: 3V3 (pin36), VSYS (pin39, raw system rail), GND.
// Carrier's LT3042 taps VSYS -> clean 1.8V analog rail for the CS43131.

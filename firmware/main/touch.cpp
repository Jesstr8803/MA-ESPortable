// touch.cpp — FT3168 capacitive touch (I2C) for MA-ESPortable.
// FT3168 logic ported from Waveshare's ESP32-S3-AMOLED-1.91 demo (touch_bsp.c).
// The controller reports coords in the panel's NATIVE landscape (536x240); our
// LVGL UI is portrait (240x536, rotated manually in display.cpp), so we map the
// raw coords through the SAME 90 deg rotation before handing them to LVGL.
#include "touch.h"
#include "board_pins.h"     // PIN_I2C_SDA/SCL, I2C_PORT
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "touch";
#define FT3168_ADDR   0x38
#define NATIVE_W      536    // panel native landscape
#define NATIVE_H      240

// Axis flips to align touch with our portrait UI — set from corner-testing.
#define TOUCH_FLIP_X  0
#define TOUCH_FLIP_Y  0

// NOTE: I2C_master_Init was already called in display.cpp (shared bus). If not,
// we install it here defensively.
static bool s_i2c_ready = false;

static esp_err_t i2c_rd(uint8_t reg, uint8_t *buf, size_t len) {
    return i2c_master_write_read_device(I2C_PORT, FT3168_ADDR, &reg, 1, buf, len, pdMS_TO_TICKS(100));
}
static esp_err_t i2c_wr(uint8_t reg, uint8_t val) {
    uint8_t b[2] = { reg, val };
    return i2c_master_write_to_device(I2C_PORT, FT3168_ADDR, b, 2, pdMS_TO_TICKS(100));
}

void touch_init(void) {
    // The display already set up I2C_NUM_0 (touch + IMU + carrier share it).
    // Just put the FT3168 into normal mode.
    if (i2c_wr(0x00, 0x00) == ESP_OK) {
        s_i2c_ready = true;
        ESP_LOGI(TAG, "FT3168 init ok");
    } else {
        ESP_LOGW(TAG, "FT3168 init failed (I2C). Touch disabled.");
    }
}

// Read one touch point in PORTRAIT (240x536) coords. Returns true if pressed.
bool touch_read(uint16_t *x, uint16_t *y) {
    if (!s_i2c_ready) return false;
    uint8_t ntouch = 0;
    if (i2c_rd(0x02, &ntouch, 1) != ESP_OK || ntouch == 0) return false;

    uint8_t buf[4];
    if (i2c_rd(0x03, buf, 4) != ESP_OK) return false;

    // Raw native coords (same decode as the demo).
    uint16_t ny = (((uint16_t)buf[0] & 0x0f) << 8) | buf[1];
    uint16_t nx = (((uint16_t)buf[2] & 0x0f) << 8) | buf[3];
    if (nx > NATIVE_W) nx = NATIVE_W;
    if (ny > NATIVE_H) ny = NATIVE_H;

    // Map native-landscape -> our portrait (matches display.cpp's 90deg CCW
    // pixel rotation: portrait_x = native_y ; portrait_y = NATIVE_W-1 - native_x).
    // (If taps land mirrored/rotated wrong, flip these — see note in display.cpp.)
    // Measured on hardware: nx spans the WIDE axis (0..536), ny spans the
    // NARROW axis (0..240). So portrait X = ny, portrait Y = nx. Per-axis flips
    // below from corner-testing.
    uint16_t px = ny;
    uint16_t py = nx;
#if TOUCH_FLIP_X
    px = (NATIVE_H - 1) - px;
#endif
#if TOUCH_FLIP_Y
    py = (NATIVE_W - 1) - py;
#endif
    if (px >= NATIVE_H) px = NATIVE_H - 1;   // portrait width  = 240 (=NATIVE_H)
    if (py >= NATIVE_W) py = NATIVE_W - 1;   // portrait height = 536 (=NATIVE_W)
    *x = px;
    *y = py;
    return true;
}
